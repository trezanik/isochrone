/**
 * @file        src/app/ImGuiWkspTopology.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiWkspTopology.h"
#include "app/ImGuiWkspSettings.h"
#include "app/ImGuiWorkspace.h"
#include "app/Workspace.h"
#include "app/Application.h"
#include "app/AppConfigDefs.h"
#include "app/AppImGui.h"  // drawclient IDs
#include "app/TConverter.h"
#include "app/Command.h"
#include "app/event/AppEvent.h"

#include "engine/resources/ResourceCache.h"
#include "engine/services/event/EngineEvent.h"

#include "core/services/config/Config.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/memory/Memory.h"
#include "core/services/log/Log.h"
#include "core/util/net/net.h"
#include "core/util/net/net_structs.h"
#include "core/util/time.h"
#include "core/TConverter.h"
#include "core/error.h"

#include "imgui/dear_imgui/imgui_internal.h"
#include "imgui/event/ImGuiEvent.h"
#include "imgui/CustomImGui.h"
#include "imgui/ImNodeGraph.h"
#include "imgui/ImNodeGraphPin.h"
#include "imgui/ImNodeGraphLink.h"
#include "imgui/BaseNode.h"
#include "imgui/TConverter.h"

#include <algorithm>
#include <sstream>


namespace trezanik {
namespace app {


constexpr char  popupname_hardware[] = "Hardware";
constexpr char  popupname_service_group[] = "Service Group";
constexpr char  popupname_service_selector[] = "Service Selector";
constexpr char  popupname_service[] = "Service";


/**
 * Nasty hack type
 * 
 * For the printf-style format to function as desired, we need to have different
 * types for AddPropertyRow to distinguish between printing with no precision,
 * limited to 2, etc.
 * 
 * Absent of adding a parameter to the function, which is an option, we define
 * this one-off type that's added statically and replaced within each loop
 * invocation, resulting in minimal overhead.
 */
struct PinPosition
{
	float* x;
	float* y;
};


ImGuiWkspTopology::ImGuiWkspTopology(
	GuiInteractions& gui_interactions,
	ImGuiWorkspace* wksp
)
: IImGui(gui_interactions)
, my_evtmgr(*core::ServiceLocator::EventDispatcher())
, my_wksp(wksp)
, my_wksp_data(&wksp->my_wksp_data)
, my_workspace(wksp->GetWorkspace())
, my_context_node(nullptr)
, my_context_pin(nullptr)
, my_context_link(nullptr)
, my_open_service_selector_popup(false)
, my_service_selector_radio_value(0)
, my_selector_index_service(-1)
, my_selector_index_service_group(-1)
, my_hide_empty_fields(false)
, my_open_hardware_popup(false)
, my_draw_hardware_popup(false)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		// immediate binding, nothing variable
		my_nodegraph.ContextPopUpContent(
			std::bind(&ImGuiWkspTopology::ContextPopup, this, std::placeholders::_1)
		);

		my_window_label = "###Topology";
		my_window_label += my_workspace->GetID().GetCanonical();

		Populate();
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiWkspTopology::~ImGuiWkspTopology()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		// _gui_interactions.mutex is already locked! ImGuiWorkspace controls it, we're just a minion

		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


template<>
int
ImGuiWkspTopology::AddNode(
	std::shared_ptr<workspace_node> node
)
{
	using namespace trezanik::core;
	using namespace trezanik::imgui;

	/*
	 * Called by two routes:
	 * 1) Initial load, via our Populate method
	 *  Nodes already exist, this method is being called in order to create the
	 *  nodegraph objects. Don't amend the list.
	 * 2) Dynamic addition
	 *  New node via topology context menu/nodelist new, etc. - the node object
	 *  passed in is created immediately prior to this call and is untracked
	 * 
	 * This as a set we could ignore duplicates, but vector with custom sorting
	 * means this now needs handling.
	 * 
	 * Potential refactor for this to be dynamic only (2), and the parent
	 * ImGuiWorkspace only does the loading (1)
	 */
	bool  wkspdata_push_back = true;

	for ( auto& n : my_wksp_data->nodes )
	{
		if ( n == node )
		{
			wkspdata_push_back = false;
			break;
		}
	}

	auto sptr = my_nodegraph.CreateNode<IsochroneNode>(
		ImVec2(node->graph.position.x, node->graph.position.y), node->id, my_wksp, node
	);

	if ( sptr == nullptr )
	{
		TZK_LOG(LogLevel::Error, "Failed to create new NodeGraph IsochroneNode");
		return ErrFAILED;
	}

	/*
	 * Track this graph node shared_ptr, all graph node operations done with these
	 * 
	 * Do this now so nodegraph update events have an associated node, otherwise
	 * it'll warn that the node isn't found
	 */
	my_nodes.push_back(sptr);

	/*
	 * Ensure the graph node style is populated even if we're using the default
	 * styles; this allows the combo box/whatever to display the active style,
	 * which is more desired than a blank box entry - when the list does actually
	 * contain the style in use.
	 * We make sure not to save this, if it's the default however.
	 */
	if ( node->graph.style.empty() )
	{
		node->graph.style = reserved_style_system.c_str();
	}
	
	sptr->SetStyle(GetNodeStyle(node->graph.style.c_str()));


	if ( node->graph.size.y != 0 && node->graph.size.x != 0 )
	{
		sptr->SetStaticSize(node->graph.size);
	}


	AddNodePins(sptr, node->graph.pins);
	

	if ( wkspdata_push_back )
	{
		// New node, add a timestamp unless it was already explicitly set
		if ( node->added == 0 )
		{
			node->added = core::aux::get_secs_since_epoch();
		}

		// track the node
		my_wksp_data->nodes.push_back(node);
		/*
		 * Sort after every insertion, runtime amendments require immediate update.
		 * Could use a set but we want the sort method to be adjustable itself, and
		 * don't want to store multiple collections for each!
		 */
		SortNodes();
	}

	return ErrNONE;
}


void
ImGuiWkspTopology::AddNodePins(
	std::shared_ptr<IsochroneNode> sptr,
	std::vector<pin>& pins
)
{
	using namespace trezanik::core;

	for ( trezanik::app::pin& pin : pins )
	{
		/*
		 * We really shouldn't encounter this if we enforce validation within
		 * loading in Workspace, but safety check
		 */
		if ( pin.type == PinType::Server && pin.svc_grp == nullptr && pin.svc == nullptr )
		{
			TZK_LOG(LogLevel::Error, "Pin has no service/group, will be omitted");
			continue;
		}

		if ( pin.style.empty() )
		{
			// default to standard (connector) style
			pin.style = reserved_style_connector;

			// detect service like we do for dynamic creations
			if ( pin.svc_grp != nullptr )
			{
				pin.style = reserved_style_service_group;
			}
			else if ( pin.svc != nullptr )
			{
				switch ( pin.svc->protocol_num )
				{
				case IPProto::icmp: pin.style = reserved_style_service_icmp; break;
				case IPProto::tcp:  pin.style = reserved_style_service_tcp; break;
				case IPProto::udp:  pin.style = reserved_style_service_udp; break;
				default:
					break;
				}
			}
			else if ( pin.type == PinType::Client )
			{
				pin.style = reserved_style_client;
			}
		}

		switch ( pin.type )
		{
		case PinType::Server:
			{
				sptr->AddServerPin(
					ImVec2(pin.pos.x, pin.pos.y), pin.id,
					GetPinStyle(pin.style.c_str()),
					pin.svc_grp, pin.svc,
					sptr.get(), &my_nodegraph
				);
			}
			break;
		case PinType::Client:
			{
				sptr->AddClientPin(
					ImVec2(pin.pos.x, pin.pos.y), pin.id,
					GetPinStyle(pin.style.c_str()),
					sptr.get(), &my_nodegraph
				);
			}
			break;
		case PinType::Connector:
			{
				sptr->AddConnectorPin(
					ImVec2(pin.pos.x, pin.pos.y), pin.id,
					GetPinStyle(pin.style.c_str()),
					sptr.get(), &my_nodegraph
				);
			}
			break;
		default:
			TZK_LOG_FORMAT(LogLevel::Warning, "Invalid/incomplete pin flow for %s", pin.id.GetCanonical());
			break;
		}
	}
}


int
ImGuiWkspTopology::AddNodeStyle(
	const char* name,
	std::shared_ptr<trezanik::imgui::NodeStyle> style
)
{
	using namespace trezanik::core;

	/*
	 * Iterate the vector and locate the name, which must be unique in the set.
	 * As noted in class documentation, this cannot be a direct map/set
	 */
	for ( auto& s : my_wksp_data->node_styles )
	{
		if ( STR_compare(s.first.c_str(), name, true) == 0 )
		{
			TZK_LOG(LogLevel::Error, "Node style already exists");
			return EEXIST;
		}
	}

	if ( IsReservedStyleName(name) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Reserved name prefix '%s' denied", name);
		return EACCES;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Added new node style: '%s'", name);

	my_wksp_data->node_styles.emplace_back(std::make_pair<>(name, style));

	return ErrNONE;
}


int
ImGuiWkspTopology::AddPinStyle(
	const char* name,
	std::shared_ptr<trezanik::imgui::PinStyle> style
)
{
	using namespace trezanik::core;

	for ( auto& s : my_wksp_data->pin_styles )
	{
		if ( STR_compare(s.first.c_str(), name, true) == 0 )
		{
			TZK_LOG(LogLevel::Error, "Pin style already exists");
			return EEXIST;
		}
	}

	if ( IsReservedStyleName(name) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Reserved name prefix '%s' denied", name);
		return EACCES;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Added new pin style: '%s'", name);

	my_wksp_data->pin_styles.emplace_back(std::make_pair<>(name, style));

	return ErrNONE;
}


template<>
int
ImGuiWkspTopology::AddPropertyRow<ImVec2>(
	PropertyRowType type,
	const char* label,
	ImVec2* value,
	bool TZK_UNUSED(hide_if_empty)
)
{
	bool  modified = false;
	bool  read_only = (type == PropertyRowType::FloatInputReadOnly);
	float  step = 0.f;
	float  step_fast = 0.f;
	// for sizes and positions, make them appear to be whole integers, but are really floats
	char   format[] = "%.0f";

	ImGui::PushID(value);
	ImGui::TableNextColumn();
	ImGui::Text("%s", label);
	ImGui::TableNextColumn();
	if ( value != nullptr )
	{
		ImGui::SetNextItemWidth(50.f);
		ImGui::InputFloat("##x", &value->x, step, step_fast, format, read_only ? ImGuiInputTextFlags_ReadOnly : 0);
		if ( ImGui::IsItemEdited() )
			modified = true;
		ImGui::SameLine();
		ImGui::SetNextItemWidth(50.f);
		ImGui::InputFloat("##y", &value->y, step, step_fast, format, read_only ? ImGuiInputTextFlags_ReadOnly : 0);
		if ( ImGui::IsItemEdited() )
			modified = true;
	}
	else
	{
		ImGui::Text("");
		ImGui::SameLine();
		ImGui::Text("");
	}
	ImGui::PopID();

	return modified;
}


template<>
int
ImGuiWkspTopology::AddPropertyRow<PinPosition>(
	PropertyRowType type,
	const char* label,
	PinPosition* value,
	bool TZK_UNUSED(hide_if_empty)
)
{
	// this should be identical to ImVec2, with a different format specifier

	bool  modified = false;
	bool  read_only = (type == PropertyRowType::FloatInputReadOnly);
	float  step = 0.f;
	float  step_fast = 0.f;
	// pin positions should have a precision of 2 when displayed
	char   format[] = "%.2f";

	ImGui::PushID(value);
	ImGui::TableNextColumn();
	ImGui::Text("%s", label);
	ImGui::TableNextColumn();
	if ( value != nullptr )
	{
		ImGui::SetNextItemWidth(50.f);
		ImGui::InputFloat("##x", value->x, step, step_fast, format, read_only ? ImGuiInputTextFlags_ReadOnly : 0);
		if ( ImGui::IsItemEdited() )
			modified = true;
		ImGui::SameLine();
		ImGui::SetNextItemWidth(50.f);
		ImGui::InputFloat("##y", value->y, step, step_fast, format, read_only ? ImGuiInputTextFlags_ReadOnly : 0);
		if ( ImGui::IsItemEdited() )
			modified = true;
	}
	else
	{
		ImGui::Text("");
		ImGui::SameLine();
		ImGui::Text("");
	}
	ImGui::PopID();

	return modified;
}



template<>
int
ImGuiWkspTopology::AddPropertyRow<trezanik::core::UUID>(
	PropertyRowType type,
	const char* label,
	trezanik::core::UUID* value,
	bool TZK_UNUSED(hide_if_empty)
)
{
	ImGui::PushID(value);
	ImGui::TableNextColumn();
	ImGui::Text("%s", label);
	ImGui::TableNextColumn();

	if ( type == PropertyRowType::TextInputReadOnly )
	{
		ImGui::InputText(
			"##uuid",
			(char*)value->GetCanonical(),  // can be char*, flags make this ready only
			trezanik::core::uuid_buffer_size,
			ImGuiInputTextFlags_ReadOnly
		);
	}
	else // PropertyRowType::TextReadOnly
	{
		ImGui::Text("%s", value->GetCanonical());
	}

	ImGui::PopID();
	return 0; // never modifiable
}


template<>
int
ImGuiWkspTopology::AddPropertyRow<std::string>(
	PropertyRowType type,
	const char* label,
	std::string* value,
	bool hide_if_empty
)
{
	bool  modified = false;

	if ( hide_if_empty && (value == nullptr || value->empty()) )
	{
		return 0;
	}
	if ( value == nullptr )
	{
		// warn
		return 0;
	}

	// use the pointer address as 'unique' ID; shouldn't ever re-use vars
	ImGui::PushID(value);
	ImGui::TableNextColumn();
	ImGui::Text("%s", label);
	ImGui::TableNextColumn();

	if ( type == PropertyRowType::NodeStyle || type == PropertyRowType::PinStyle )
	{
		int  index = (type == PropertyRowType::NodeStyle) ? IndexFromNodeStyle(value) : IndexFromPinStyle(value);

		if ( index == -1 )
			index = 0;

#if 0  // old api - uses c_str dedicated vector, needs updating on change
		int  new_index = -1;
		if ( ImGui::Combo(IdLabelString(id).c_str(), &index, my_node_styles_cstr.data(), (int)my_node_styles_cstr.size()) )
		{
			if ( new_index != index )
			{
				const char* new_val = StyleFromIndex(new_index);
				if ( new_val == nullptr )
					value->clear();
				else
					value->assign(new_val);
				modified = true;
			}
		}
#else  // new api
		if ( ImGui::BeginCombo("##combo", value->c_str(), 0) )
		{
			int  pos = -1;
			// new api enables raw, non-cached usage

			auto style_loop = [&pos,&index,&modified,&value](auto& e) {
				const bool  is_selected = (++pos == index);

				if ( e.first.empty() )
				{
					TZK_DEBUG_BREAK;
				}
				else
				{
					if ( ImGui::Selectable(e.first.c_str(), is_selected) )
					{
						value->assign(e.first);
						modified = true;
					}
					if ( is_selected )
					{
						ImGui::SetItemDefaultFocus();
					}
				}
			};

			if ( type == PropertyRowType::NodeStyle )
			{
				for ( auto& e : my_wksp_data->node_styles )
				{
					style_loop(e);
				}
			}
			else
			{
				for ( auto& e : my_wksp_data->pin_styles )
				{
					style_loop(e);
				}
			}

			ImGui::EndCombo();
		}
#endif
	}
	else if ( type == PropertyRowType::TextInput )
	{
		modified = ImGui::InputText("##text", value);
	}
	else if ( type == PropertyRowType::TextInputReadOnly )
	{
		modified = ImGui::InputText("##text", value, ImGuiInputTextFlags_ReadOnly);
	}
	else if ( type == PropertyRowType::TextMultilineInput )
	{
		modified = ImGui::InputTextMultiline("##text", value);
	}
	else if ( type == PropertyRowType::TextMultilineInputReadOnly )
	{
		modified = ImGui::InputTextMultiline("##text", value, ImVec2(0,0), ImGuiInputTextFlags_ReadOnly);
	}
	else if ( type == PropertyRowType::TextReadOnly )
	{
#if 0  // Code Disabled: Not a fan how this looks, and is very much setting dependent
		ImGui::PushFont(_gui_interactions.font_fixed_width);
		ImGui::Text("%s", value->c_str());
		ImGui::PopFont();
#else
		ImGui::Text("%s", value->c_str());
#endif
	}
	else
	{
		// always have an element to maintain columns+rows positioning
		ImGui::Text("(null)");
	}

	ImGui::PopID();
	return modified;
}


int
ImGuiWkspTopology::AddWorkspacePinFromNodeGraphPin(
	std::shared_ptr<trezanik::imgui::Pin> ng_pin
)
{
	using namespace trezanik::core;

	auto  sp = std::dynamic_pointer_cast<ServerPin>(ng_pin);
	auto  cp = std::dynamic_pointer_cast<ClientPin>(ng_pin);
	auto  tp = std::dynamic_pointer_cast<ConnectorPin>(ng_pin);
	PinType  type = PinType::Invalid;
	ImVec2   pos;

	if ( sp != nullptr )
	{
		type = PinType::Server;
	}
	else if ( cp != nullptr )
	{
		type = PinType::Client;
	}
	else if ( tp != nullptr )
	{
		type = PinType::Connector;
	}
	else
	{
		TZK_LOG_FORMAT(LogLevel::Error, 
			"Failed to dynamic_cast Pin %s to a ServerPin, ClientPin, or ConnectorPin; proceeding will result in an invalid pin",
			ng_pin->GetID().GetCanonical()
		);
		return EFAULT;
	}

	pos.x = ng_pin->GetRelativePosition().x;
	pos.y = ng_pin->GetRelativePosition().y;

	trezanik::app::pin  retval(ng_pin->GetID(), pos, type);
	
	if ( sp != nullptr )
	{
		if ( sp->IsServiceGroup() )
			retval.svc_grp = sp->GetServiceGroup();
		else
			retval.svc = sp->GetService();
	}

	/*
	 * Not a fan of this; to get the name of the style, we have to look up the
	 * pointer and match it to the styles held in the workspace. Storing the
	 * name would be nicer, but we have nowhere to store it within the pin
	 * style and don't want to add it there
	 */
	auto  pinstyle = GetPinStyle(ng_pin->GetStyle());
	retval.style = pinstyle.empty() ? reserved_style_connector : pinstyle;

	auto  iscn = dynamic_cast<IsochroneNode*>(ng_pin->GetAttachedNode());
	if ( iscn == nullptr )
	{
		return EFAULT;
	}

	iscn->GetWorkspaceNode()->graph.pins.push_back(retval);
	return ErrNONE;
}


void
ImGuiWkspTopology::BreakLink(
	trezanik::imgui::Pin* pin
)
{
	using namespace trezanik::core;

	for ( auto& l : pin->GetLinks() )
	{
		trezanik::imgui::Pin*  srcpin = l->Source().get();
		trezanik::imgui::Pin*  tgtpin = l->Target().get();
		bool  pin_is_src = (srcpin == pin);
		bool  pin_is_tgt = (tgtpin == pin);

		if ( pin_is_src || pin_is_tgt )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Breaking link %s via pin %s",
				l->GetID().GetCanonical(), pin->GetID().GetCanonical()
			);

			my_nodegraph.RemoveLink(l);
			return;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Failed to find link for pin %s",
		pin->GetID().GetCanonical()
	);
}


void
ImGuiWkspTopology::BreakLink(
	const trezanik::core::UUID& id
)
{
	using namespace trezanik::core;

	for ( auto& ngl : my_nodegraph.GetLinks() )
	{
		if ( ngl->GetID() == id )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Breaking link %s", ngl->GetID().GetCanonical());

			// and the nodegraph - must return, iterator broken
			my_nodegraph.RemoveLink(ngl);
			return;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Failed to find link with id %s", id.GetCanonical());
}


ImVec2
ImGuiWkspTopology::ContextCalcNodePinPosition()
{
	using namespace trezanik::core;

	const ImVec2&  pos = my_context_node->GetPosition();
	const ImVec2&  size = my_context_node->GetSize();
	/*
	 * Do a simple distance check to decide on a winner. This isn't perfect,
	 * but is definitely good enough outside of a proper algorithm
	 * This *should* always be within the rectangle confines, or right on
	 * its edge - never outside.
	 */
	float   xf = (my_context_cursor_pos.x - pos.x) / size.x;
	float   yf = (my_context_cursor_pos.y - pos.y) / size.y;
	ImVec2  pinpos(0, 0);

	if ( xf < 0.f || yf < 0.f || xf > 1.f || yf > 1.f )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Offset calculation error: xf=%.4f, yf=%.4f; using default", xf, yf);
		TZK_DEBUG_BREAK;
		return pinpos;
	}

	if ( xf <= 0.5f ) // top, left or bottom
	{
		if ( yf <= 0.5f ) // top or left
		{
			if ( yf <= xf ) // top
			{
				pinpos.x = xf;
				pinpos.y = 0.f;
			}
			else // left
			{
				pinpos.x = 0.f;
				pinpos.y = yf;
			}
		}
		else // bottom or left
		{
			if ( xf < (1.f - yf) ) // left
			{
				pinpos.x = 0.f;
				pinpos.y = yf;
			}
			else // bottom
			{
				pinpos.x = xf;
				pinpos.y = 1.f;
			}
		}
	}
	else // top, right or bottom
	{
		if ( yf <= 0.5f ) // top or right
		{
			if ( (1.f - yf) > xf ) // top
			{
				pinpos.x = xf;
				pinpos.y = 0.f;
			}
			else // right
			{
				pinpos.x = 1.f;
				pinpos.y = yf;
			}
		}
		else // bottom or right
		{
			if ( (1.f - yf) > (1.f - xf) ) // right
			{
				pinpos.x = 1.f;
				pinpos.y = yf;
			}
			else // bottom
			{
				pinpos.x = xf;
				pinpos.y = 1.f;
			}
		}
	}

	return pinpos;
}


void
ImGuiWkspTopology::ContextPopup(
	trezanik::imgui::context_popup& cp
)
{
	using namespace trezanik::core;

	bool  close_popup = false;

	my_context_cursor_pos = cp.position;

	/*
	 * Six different menus based on selection:
	 * 1) Single node, no pin. Node operations displayed
	 * 2) Single node, pin. Pin operations displayed
	 * 3) Multiple nodes (no pin). Multi-node operations displayed
	 * 4) No node, no pin. Canvas operations displayed
	 * 5) As per 1 and 2, but overrides all others if a hovered node
	 * 6) Link, overrides all others. Link operations displayed
	 * 
	 * Remember that the hovered node is only populated if it's *not* already in
	 * the selected node listing.
	 * 
	 * This drawing capability is confined to the context menu popup only; only
	 * provide things like MenuItems and Buttons for interaction.
	 */
	if ( cp.hovered_link != nullptr )
	{
		my_context_link = cp.hovered_link;

		close_popup = DrawContextPopupLinkSelect(my_context_link);
	}
	else if ( cp.hovered_node != nullptr )
	{
		my_context_node = cp.hovered_node;
		my_context_pin = cp.pin;

		if ( my_context_pin != nullptr )
		{
			close_popup = DrawContextPopupPinSelect(my_context_node, my_context_pin);
		}
		else
		{
			auto  node = dynamic_cast<IsochroneNode*>(my_context_node);
			if ( node != nullptr )
			{
				close_popup = DrawContextPopupNodeSelect(node);
			}
		}
	}
	else if ( cp.nodes.size() > 1 )
	{
		my_context_node = nullptr;
		my_context_pin = nullptr;

		if ( cp.hovered_node == nullptr )
		{
			// if any selected node is not hovered, do not display its context menu
			close_popup = DrawContextPopupNoSelect();
		}
		else
		{
			close_popup = DrawContextPopupMultiSelect(cp.nodes);
		}
	}
	else if ( cp.nodes.size() == 1 )
	{
		my_context_node = *cp.nodes.begin();
		my_context_pin = cp.pin;

		if ( my_context_pin != nullptr )
		{
			close_popup = DrawContextPopupPinSelect(my_context_node, my_context_pin);
		}
		else if ( cp.hovered_node == nullptr )
		{
			// if the selected node is not hovered, do not display its context menu
			close_popup = DrawContextPopupNoSelect();
		}
		else
		{
			auto  node = dynamic_cast<IsochroneNode*>(my_context_node);
			if ( node != nullptr )
			{
				close_popup = DrawContextPopupNodeSelect(node);
			}
		}
	}
	else
	{
		my_context_node = nullptr;
		my_context_pin = nullptr;

		if ( cp.pin != nullptr )
		{
			my_context_pin = cp.pin;
			close_popup = DrawContextPopupPinSelect(my_context_node, my_context_pin);
		}
		else
		{
			close_popup = DrawContextPopupNoSelect();
		}
	}

	if ( close_popup )
	{
		TZK_LOG(LogLevel::Trace, "Closing current popup");
		ImGui::CloseCurrentPopup();
	}
}


std::shared_ptr<imgui::Link>
ImGuiWkspTopology::CreateLink(
	std::shared_ptr<trezanik::imgui::Pin> source,
	std::shared_ptr<trezanik::imgui::Pin> target
)
{
	using namespace trezanik::core;

	UUID  uuid;
	uuid.Generate();

	auto  link = std::make_shared<app::link>(
		uuid, source->GetID(), target->GetID()
	);

	my_wksp_data->links.insert(link);

	TZK_LOG_FORMAT(LogLevel::Debug, "Created link %s, between %s and %s", 
		uuid.GetCanonical(),
		source->GetID().GetCanonical(), target->GetID().GetCanonical()
	);

	auto  retval = my_nodegraph.CreateLink<imgui::Link>(
		link->id, source, target,
		&my_nodegraph,
		&link->text, &link->offset, &link->method
	);
	return retval;
}


void
ImGuiWkspTopology::Draw()
{
	using namespace trezanik::core;

	if ( ImGui::BeginChild(my_window_label.c_str()) )
	{
		/*
		 * Draws Canvas
		 * Updates Selected nodes and Deletes those pending destruction
		 * Iterates nodes
		 * node.Update()
		 *   Draws self, and iterates pins [in+out independently]
		 *     pin.Update()
		 *       Draws self
		 * Hover/selection handling determined in each Update call
		 */
		my_nodegraph.Update();
		my_selected_nodes = my_nodegraph.GetSelectedNodes();

		/*
		 * Use this opportunity to update the pin tooltip text. We can do it
		 * every frame, but that's very wasteful. Instead, we detect if a pin is
		 * hovered, and update the text, so it'll be ready and shown the frame
		 * after the first hover (and repeated until no longer hovered)
		 */
		imgui::Pin*  hovered_pin = my_nodegraph.GetHoveredPin();
		if ( hovered_pin != nullptr )
		{
			UpdatePinTooltip(hovered_pin);
		}

		ImGui::EndChild();
	}

	if ( my_open_service_selector_popup )
	{
		ImGui::OpenPopup(popupname_service_selector);
		my_open_service_selector_popup = false;

		ImVec2  popup_size(_gui_interactions.app_rect.Max.x / 2, _gui_interactions.app_rect.Max.y / 2);
		ImVec2  popup_size_min(200.f, 150.f);
		ImVec2  popup_size_max(-1.f, -1.f);

		// some arbritrary values here for now
		if ( popup_size.x > 1024.f ) popup_size.x = 1024.f;
		if ( popup_size.y > 1024.f ) popup_size.y = 1024.f;
		if ( popup_size.x < 250.f ) popup_size.x = 250.f;
		if ( popup_size.y < 250.f ) popup_size.y = 250.f;

		/// @bug these don't seem to be working
		ImGui::SetNextWindowSize(popup_size, ImGuiCond_Appearing);
		ImGui::SetNextWindowSizeConstraints(popup_size_min, popup_size_max);
	}
	if ( DrawServiceSelector() != 0 && my_context_node != nullptr )
	{
		/*
		 * User has confirmed selection from service selector.
		 * Presently, this is only invoked from adding an input pin to a selected
		 * node, so we'll proceed on this basis.
		 * Can imagine other needs for service selection, so should consider this
		 * for future works
		 */

		auto  pinpos = ContextCalcNodePinPosition();
		auto  inode = dynamic_cast<IsochroneNode*>(my_context_node);

		if ( inode == nullptr )
		{
			TZK_LOG(LogLevel::Error, "Failed to cast context node to an IsochroneNode");
			my_context_node = nullptr;
			ImGui::EndChild();
			return;
		}

		if ( my_selector_service != nullptr )
		{
			/*
			 * note: we can call imgui::PinStyle::whatever(), but they won't be
			 * modifiable as they'll be new objects, not the shared_ptr to the
			 * styles being held (which are the ones offered for dynamic edits)
			 */
			std::shared_ptr<imgui::PinStyle>  style = GetPinStyle(reserved_style_connector.c_str());

			switch ( my_selector_service->protocol_num )
			{
			case IPProto::tcp:
				style = GetPinStyle(reserved_style_service_tcp.c_str());
				break;
			case IPProto::udp:
				style = GetPinStyle(reserved_style_service_udp.c_str());
				break;
			case IPProto::icmp:
				style = GetPinStyle(reserved_style_service_icmp.c_str());
				break;
			default:
				break;
			}
			// create in nodegraph
			auto  spin = inode->AddServerPin(
				pinpos, style,
				nullptr, my_selector_service,
				my_context_node, &my_nodegraph
			);
			TZK_LOG_FORMAT(LogLevel::Debug, "Created %s pin %s at (%f,%f)", "server", spin->GetID().GetCanonical(), pinpos.x, pinpos.y);
			// track in workspace data
			AddWorkspacePinFromNodeGraphPin(spin);
		}
		else if ( my_selector_service_group != nullptr )
		{
			// create in nodegraph
			auto  spin = inode->AddServerPin(
				pinpos,
				GetPinStyle(reserved_style_service_group.c_str()),
				my_selector_service_group, nullptr,
				my_context_node, &my_nodegraph
			);
			TZK_LOG_FORMAT(LogLevel::Debug, "Created %s pin %s at (%f,%f)", "server", spin->GetID().GetCanonical(), pinpos.x, pinpos.y);
			// track in workspace data
			AddWorkspacePinFromNodeGraphPin(spin);
		}
		else
		{
			TZK_LOG(LogLevel::Warning, "No selected service or service group after a modal confirmation");
		}

		my_context_node = nullptr;
	}
	
	if ( my_open_hardware_popup )
	{
		/// @todo check for sysinfo component, otherwise ignore

		ImGui::OpenPopup(popupname_hardware);
		my_open_hardware_popup = false;
		my_draw_hardware_popup = true;
	}
	if ( my_draw_hardware_popup )
	{
		// micro-optimization possible
		IsochroneNode*  node = dynamic_cast<IsochroneNode*>(my_context_node == nullptr ? my_selected_nodes[0].get() : my_context_node);
		if ( node != nullptr )
		{
			// content region of workspace, not the full application client area!
			ImVec2  cr = ImGui::GetContentRegionAvail() * 0.75f;
			ImGui::SetNextWindowSize(cr, ImGuiCond_Appearing);
			DrawHardwareDialog(node);
		}
		else
		{
			my_draw_hardware_popup = false;
			ImGui::ClosePopupToLevel(0, false);
		}
	}
}


bool
ImGuiWkspTopology::DrawContextPopupLinkSelect(
	trezanik::imgui::Link* link
)
{
	using namespace trezanik::core;

	bool  retval = false;

	ImGui::Text("Link Context Menu");
	ImGui::Separator();


	// disable for alpha, not yet implemented - use propview
	ImGui::BeginDisabled();
	if ( ImGui::Button("Set Text") )
	{
		/// @todo add link text dialog
	}
	ImGui::EndDisabled();

	ImGui::Separator();

	float  button_width = ImGui::GetContentRegionAvail().x;

	ImGui::SetNextWindowSize(ImVec2(button_width, 0));
	if ( ImGui::Button("Delete Link") )
	{
		TZK_LOG(LogLevel::Trace, "Deleting link");

		BreakLink(link->GetID());
		retval = true;
	}

	return retval;
}


bool
ImGuiWkspTopology::DrawContextPopupMultiSelect(
	std::vector<trezanik::imgui::BaseNode*> nodes
)
{
	ImGui::Text("Graph Context Menu");
	ImGui::Separator();

	if ( ImGui::Button("Delete All Nodes") )
	{
		// confirm deletions - list node names/ids
		for ( auto& n : nodes )
			my_nodegraph.DeleteNode(n);

		return true;
	}

	return false;
}


bool
ImGuiWkspTopology::DrawContextPopupNoSelect()
{
	using namespace trezanik::core;

	ImGui::Text("Graph Context Menu");
	ImGui::Separator();

	if ( ImGui::Button("New Node") )
	{
		TZK_LOG(LogLevel::Debug, "Creating new node");

		auto  node = std::make_shared<workspace_node>();
		node->graph.position = my_context_cursor_pos;
		node->graph.size.x = TZK_DEFAULT_NEWNODE_WIDTH;
		node->graph.size.y = TZK_DEFAULT_NEWNODE_HEIGHT;
		node->name = newnode_name;
		node->id.Generate();

		// DispatchEvent :: NodeCreated -> one handler for dup checking
		AddNode(node);

		return true;
	}
	
	return false;
}


bool
ImGuiWkspTopology::DrawContextPopupNodeSelect(
	IsochroneNode* node
)
{
	using namespace trezanik::core;

	bool  retval = false;

	// nice to have its name/identifier here for assurance
	ImGui::Text("Node Context Menu");
	ImGui::Separator();


	// if component - scanners/tools
	if ( ImGui::BeginMenu("Recon") )
	{
		if ( ImGui::MenuItem("nmap") )
		{

		}
		ImGui::EndMenu();
	}
	if ( ImGui::BeginMenu("Attack") )
	{
		ImGui::MenuItem("metasploit");
		ImGui::EndMenu();
	}
	ImGui::Separator();
	if ( ImGui::BeginMenu("Deploy") )
	{
		ImGui::MenuItem("Deploy GOD");
		ImGui::EndMenu();
	}
	// if ( ConnectToGOD() - GOD_discovered )
	/*
	 * 3 display states:
	 * GOD : Unknown (never deployed/unable to communicate)
	 * GOD : Exists (not running)
	 * GOD : Established (Active TCP session)
	 */
	if ( ImGui::BeginMenu("GOD") )
	{
		if ( ImGui::MenuItem("Acquire Prefetch") )
		{
			TZK_LOG(LogLevel::Warning, "Not implemented");
			retval = true;
		}
		if ( ImGui::MenuItem("Acquire AMCache") )
		{
			TZK_LOG(LogLevel::Warning, "Not implemented");
		}
		if ( ImGui::MenuItem("Identify Anomalies") )
		{
			TZK_LOG(LogLevel::Warning, "Not implemented");
			retval = true;
		}
		if ( ImGui::MenuItem("Get Autostarts") )
		{
			TZK_LOG(LogLevel::Warning, "Not implemented");
			retval = true;
		}
		if ( ImGui::MenuItem("Get Browsing History") )
		{
			// prompt for user

			TZK_LOG(LogLevel::Warning, "Not implemented");
			retval = true;
		}
		if ( ImGui::MenuItem("Get Local Users") )
		{
			// get all profiles

			TZK_LOG(LogLevel::Warning, "Not implemented");
			retval = true;
		}
		if ( ImGui::MenuItem("Get Logged On Users") )
		{
			// get all logon sessions

			TZK_LOG(LogLevel::Warning, "Not implemented");
			retval = true;
		}
		ImGui::EndMenu();
	}

	ImGui::Separator();

	if ( ImGui::BeginMenu("Pins") )
	{
		if ( ImGui::MenuItem("Add Server (inbound)") )
		{
			TZK_LOG(LogLevel::Trace, "Adding server pin");

			my_open_service_selector_popup = true;
			retval = true;
		}
		if ( ImGui::MenuItem("Add Client (outbound)") )
		{
			TZK_LOG(LogLevel::Trace, "Adding client pin");

			auto  pos = ContextCalcNodePinPosition();
			// add to nodegraph and pin objects
			auto  cpin = node->AddClientPin(
				pos,
				GetPinStyle(reserved_style_client.c_str()),
				my_context_node, &my_nodegraph
			);
			TZK_LOG_FORMAT(LogLevel::Debug, "Created %s pin %s at (%f,%f)", "client", cpin->GetID().GetCanonical(), pos.x, pos.y);
			// track in workspace data
			AddWorkspacePinFromNodeGraphPin(cpin);
			retval = true;
		}
		ImGui::Separator();
		if ( ImGui::MenuItem("Add Generic Connector") )
		{
			TZK_LOG(LogLevel::Trace, "Adding connector pin");

			auto  pos = ContextCalcNodePinPosition();
			// add to nodegraph and pin objects
			auto  cpin = node->AddConnectorPin(
				pos,
				GetPinStyle(reserved_style_connector.c_str()),
				my_context_node, &my_nodegraph
			);
			TZK_LOG_FORMAT(LogLevel::Debug, "Created %s pin %s at (%f,%f)", "connector", cpin->GetID().GetCanonical(), pos.x, pos.y);
			// track in workspace data
			AddWorkspacePinFromNodeGraphPin(cpin);
			retval = true;
		}
		ImGui::EndMenu();
	}

	ImGui::Separator();
	if ( ImGui::MenuItem("Edit Hardware") )
	{
		my_open_hardware_popup = true;
	}
	ImGui::Separator();

	if ( ImGui::BeginMenu("Set Style") )
	{
		for ( auto& style : my_wksp_data->node_styles )
		{
			bool  is_current = false;

			if ( node->GetStyle() == style.second )
			{
				is_current = true;
				ImGui::BeginDisabled();
			}

			if ( ImGui::MenuItem(style.first.c_str()) )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Assigning style %s to node %s",
					style.first.c_str(), node->GetID().GetCanonical()
				);

				node->SetStyle(style.second);
				retval = true;
				break;
			}

			if ( is_current )
			{
				ImGui::EndDisabled();
			}
		}

		ImGui::EndMenu();
	}

	ImGui::Separator();

#if 0  // Code Disabled: example and visual usage ///////////////// needs updating for new evtmgmt
	if ( ImGui::StyledButton("Styled Button", ImVec2(100.f, 0), 
		IM_COL32(0, 180, 0, 255), IM_COL32(0, 200, 0, 255), IM_COL32(0, 255, 0, 255)
	))
	{
		auto  extevt = std::make_unique<AppEvent_UIButtonPress>("Styled Button", "operation.being.performed");
		my_evtmgr.PushEvent(std::move(extevt));
		retval = true;
	}
#endif
	if ( ImGui::Button("Log All Properties") )
	{
		TZK_LOG_FORMAT(LogLevel::Mandatory, "Dumping Node Properties: %s", node->Dump().str().c_str());
		retval = true;
	}

	ImGui::Separator();

	float  button_width = ImGui::GetContentRegionAvail().x;

	ImGui::SetNextWindowSize(ImVec2(button_width, 0));
	if ( ImGui::Button("Delete Node") )
	{
		TZK_LOG(LogLevel::Trace, "Deleting node");

		node->Close(); // optional call
		/*
		 * trigger removal from our state and the workspace data. Must be a
		 * consistent route as the nodegraph handler is unaware of the topology
		 * and can only issue an event dispatch; this makes it the same setup
		 * regardless of GUI delete (this) or keyboard delete (nodegraph)
		 */
		imgui::EventData::node_graph_update  nu{ imgui::NodeGraphUpdate::NodeDeleting, &my_nodegraph };
		nu.opt.node_uuid = node->ID();
		ServiceLocator::EventDispatcher()->DispatchEvent(imgui::uuid_nodegraph_update, nu);

		retval = true;
	}

	return retval;
}


bool
ImGuiWkspTopology::DrawContextPopupPinSelect(
	trezanik::imgui::BaseNode* TZK_UNUSED(node),
	trezanik::imgui::Pin* pin
)
{
	using namespace trezanik::core;

	/*
	 * Warning:
	 * The node will be the selected node (if any node is at all).
	 * This is not necessarily the node that the pin is attached to! Perfectly
	 * valid to have a node selected, but then right click another pin.
	 *
	 * Node is strictly speaking redundant here; might be useful in future?
	 */

	bool  retval = false;
	trezanik::imgui::BaseNode*  pin_node = pin->GetAttachedNode();

	ImGui::Text("Pin Context Menu");
	ImGui::Separator();

	// can dynamic cast to the pin type and display additional data - ServerPin

	if ( pin->IsConnected() )
	{
		if ( pin->GetLinks().size() == 1 )
		{
			if ( ImGui::Button("Break Link") )
			{
				BreakLink(pin);
				retval = true;
			}
		}
		else
		{
			/*
			 * If you want to break a specific link when there are multiple
			 * connections, you should be right-clicking the link instead and
			 * doing it within the dedicated context menu
			 */
			if ( ImGui::Button("Break All Links") )
			{
				// convoluted, but we have to avoid breaking iterators
				std::vector<trezanik::core::UUID>  break_links;
				for ( auto& l : pin->GetLinks() )
				{
					break_links.emplace_back(l->GetID());
				}
				for ( auto& id : break_links )
				{
					BreakLink(id);
				}

				retval = true;
			}
		}
	}

	if ( ImGui::BeginMenu("Set Style") )
	{
		for ( auto& style : my_wksp_data->pin_styles )
		{
			bool  is_current = false;

			if ( pin->GetStyle() == style.second )
			{
				is_current = true;
				ImGui::BeginDisabled();
			}

			if ( ImGui::MenuItem(style.first.c_str()) )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Assigning style %s to pin %s",
					style.first.c_str(), pin->GetID().GetCanonical()
				);

				pin->SetStyle(style.second);
				retval = true;
				break;
			}

			if ( is_current )
			{
				ImGui::EndDisabled();
			}
		}

		ImGui::EndMenu();
	}

	ImGui::Separator();

	float  button_width = ImGui::GetContentRegionAvail().x;

	ImGui::SetNextWindowSize(ImVec2(button_width, 0));
	if ( ImGui::Button("Delete Pin") )
	{
		TZK_LOG(LogLevel::Trace, "Deleting pin");

		// break all links first to avoid remnants
		// convoluted, but we have to avoid breaking iterators
		std::vector<trezanik::core::UUID>  break_links;
		for ( auto& l : pin->GetLinks() )
		{
			break_links.emplace_back(l->GetID());
		}
		for ( auto& id : break_links )
		{
			BreakLink(id);
		}

		pin_node->RemovePin(pin->GetID());
		retval = true;
	}

	return retval;
}


void
ImGuiWkspTopology::DrawHardwareDialog(
	IsochroneNode* node
)
{
	using namespace trezanik::core;

	if ( node == nullptr || !ImGui::BeginPopupModal(popupname_hardware, &my_draw_hardware_popup) )
	{
		my_draw_hardware_popup = false;
		return;
	}

	auto  wnode = node->GetWorkspaceNode();
	if ( !wnode->has_component(cth_cmpt_sysinfo) )
	{
#if 0  // Code Disabled: create the component if it doesn't have one already. Auto-deleted on save if empty
		TZK_LOG_FORMAT(LogLevel::Warning, "Hardware Dialog attempted opening for node (%s) without a system info component", node->GetID().GetCanonical());
		my_draw_hardware_popup = false;
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		return;
#else
		auto  sysinf = std::make_unique<node_component_systeminfo>();
		wnode->components.push_back(std::move(sysinf));
#endif
	}

	auto  nc_sysinf = dynamic_cast<node_component_systeminfo*>(wnode->get_component(cth_cmpt_sysinfo));
	if ( nc_sysinf == nullptr )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Hardware Dialog attempted opening for node (%s) without a system info component", node->GetID().GetCanonical());
		my_draw_hardware_popup = false;
		ImGui::CloseCurrentPopup();
		ImGui::EndPopup();
		return;
	}

	node_component_systeminfo::system&  sysinf = nc_sysinf->system_info;

	ImGui::PushID(this);

	ImGui::Text("Node : %s", node->GetWorkspaceNode()->name.c_str());
	ImGui::Spacing();

	my_wksp->DrawHardwareEditor(sysinf);

	ImGui::PopID();

	if ( ImGui::Button("Close") )
	{
		my_draw_hardware_popup = false;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}


void
ImGuiWkspTopology::DrawPropertyView()
{
	using namespace trezanik::core;

	auto  AddSeparatorRow = [](const char* txt)
	{
		ImGui::TableNextColumn();
		ImGui::SeparatorText(txt);
		ImGui::TableNextRow();
	};

	if ( ImGui::CollapsingHeader("Workspace Properties", ImGuiTreeNodeFlags_None) )
	{
		static ImVec2  outer_size = { 0.f, 0.f };
		int  row_count = 0;

		ImGuiTableFlags  table_flags = ImGuiTableFlags_Resizable
			| ImGuiTableFlags_NoSavedSettings
			| ImGuiTableFlags_RowBg
			| ImGuiTableFlags_SizingStretchProp
			| ImGuiTableFlags_ScrollY
			| ImGuiTableFlags_HighlightHoveredColumn;

		if ( ImGui::BeginTable("workspaceprops##", 2, table_flags, outer_size) )
		{
			ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
			ImGui::TableSetupColumn("Property", col_flags, 0.3f);
			ImGui::TableSetupColumn("Value", col_flags, 0.7f);
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();
			row_count += 2;

			AddSeparatorRow("Configuration");
			row_count += 2;

			ImGui::TableNextColumn();
			ImGui::Text("Default Link Method");
			ImGui::TableNextColumn();
			ImGui::RadioButton("Direct", &my_nodegraph.settings.link_default_method, static_cast<int>(imgui::LinkMethod::Direct));
			ImGui::RadioButton("Cubic Bezier", &my_nodegraph.settings.link_default_method, static_cast<int>(imgui::LinkMethod::CubicBezier));
			ImGui::RadioButton("Quadratic Bezier", &my_nodegraph.settings.link_default_method, static_cast<int>(imgui::LinkMethod::QuadraticBezier));
			ImGui::RadioButton("Multi-line Auto", &my_nodegraph.settings.link_default_method, static_cast<int>(imgui::LinkMethod::MultiLineAuto));
			ImGui::RadioButton("Multi-line Hybrid", &my_nodegraph.settings.link_default_method, static_cast<int>(imgui::LinkMethod::MultiLineHybrid));
			row_count += 5;

			AddSeparatorRow("Links");
			row_count += 1;

			for ( auto& l : my_nodegraph.GetLinks() )
			{
				ImGui::PushID(l.get());

				ImU32  oc = IM_COL32(166, 169, 74, 255);
				ImGui::PushStyleColor(ImGuiCol_Text, oc);
				ImGui::TableNextColumn();
				ImGui::Text("ID");
				ImGui::TableNextColumn();
				ImGui::PushFont(_gui_interactions.font_fixed_width, _gui_interactions.font_fixed_width_size);
				ImGui::Text("%s", l->GetID().GetCanonical());
				ImGui::PopFont();
				ImGui::PopStyleColor();
				
				ImGui::TableNextColumn();
				ImGui::Text("Text");
				ImGui::TableNextColumn();
				ImGui::InputText("##Text", l->GetText());

				ImGui::TableNextColumn();
				ImGui::Text("Text Offset");
				ImGui::TableNextColumn();
				ImGui::InputFloat2("##TextOffset", &l->GetTextOffset()->x);

				ImGui::TableNextColumn();
				ImGui::Text("Source Pin");
				ImGui::TableNextColumn();
				ImGui::Text("%s", l->Source()->GetID().GetCanonical());

				ImGui::TableNextColumn();
				ImGui::Text("Target Pin");
				ImGui::TableNextColumn();
				ImGui::Text("%s", l->Target()->GetID().GetCanonical());

				ImGui::TableNextColumn();
				ImGui::Text("Source Node");
				ImGui::TableNextColumn();
				//ImGui::Text("%s", l->Source()->GetAttachedNode()->GetName()->c_str());
				ImGui::Text("%s", l->Source()->GetAttachedNode()->GetID().GetCanonical());

				ImGui::TableNextColumn();
				ImGui::Text("Target Node");
				ImGui::TableNextColumn();
				//ImGui::Text("%s", l->Target()->GetAttachedNode()->GetName()->c_str());
				ImGui::Text("%s", l->Target()->GetAttachedNode()->GetID().GetCanonical());

				ImGui::TableNextColumn();
				ImGui::Text("Method");
				ImGui::TableNextColumn();
				int*   rval = reinterpret_cast<int*>(l->GetMethod());
				ImGui::RadioButton("Direct", rval, static_cast<int>(imgui::LinkMethod::Direct));
				ImGui::RadioButton("Cubic Bezier", rval, static_cast<int>(imgui::LinkMethod::CubicBezier));
				ImGui::RadioButton("Quadratic Bezier", rval, static_cast<int>(imgui::LinkMethod::QuadraticBezier));
				ImGui::RadioButton("Multi-line Auto", rval, static_cast<int>(imgui::LinkMethod::MultiLineAuto));
				ImGui::RadioButton("Multi-line Hybrid", rval, static_cast<int>(imgui::LinkMethod::MultiLineHybrid));

				row_count += 12;

				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndTable();
		}

		// correct size for the next frame
		outer_size.y = ImGui::GetFrameHeight() + row_count * (ImGui::GetTextLineHeight() + ImGui::GetStyle().CellPadding.y * 2.0f);

	} // workspace properties

	if ( ImGui::CollapsingHeader("Node Properties", ImGuiTreeNodeFlags_None) )
	{
		/*
		 * Shared between all nodes/workspaces.
		 * Each frame, the content is calculated allowing us to then determine
		 * the size needing to be given, which will take effect on the next frame.
		 *
		 * If we don't specify this, the table consumes all the available content
		 * region - which I wouldn't mind, but it pushes the other collapsing
		 * headers out of view in verticality, which looks ridiculous when there
		 * can frequently only be one row.
		 * Easy to then miss all the child headers since the next one is heavily
		 * cut-off and looks more like a horizontal scrollbar
		 */
		static ImVec2  outer_size = { 0.f, 0.f };
		int  row_count = 0;

		ImGuiTableFlags  table_flags = ImGuiTableFlags_Resizable
			| ImGuiTableFlags_NoSavedSettings
			| ImGuiTableFlags_RowBg
			| ImGuiTableFlags_SizingStretchProp
			| ImGuiTableFlags_ScrollY
			| ImGuiTableFlags_HighlightHoveredColumn;

		if ( ImGui::BeginTable("nodeprops##", 2, table_flags, outer_size) )
		{
			ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
			ImGui::TableSetupColumn("Property", col_flags, 0.3f);
			ImGui::TableSetupColumn("Value", col_flags, 0.7f);
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();
			/*
			 * each 'property row' is responsible for moving onto its own row
			 * with each invocation.
			 * This is to allow us to dynamically insert a trailing element that
			 * can indicate an invalid item, or add undo/redo/other buttons in
			 * future.
			 * 
			 * So this comes later: ImGui::TableNextColumn();
			 */
			row_count = 3; // 1 row plus 2 spacing

			if ( my_selected_nodes.empty() )
			{
				/*
				 * Keep drawing the table, but don't populate any data (since
				 * there are multiple nodes selected).
				 * This will keep positions of buttons and areas of focus in
				 * the same spot, which can be less jarring for users
				 */

				ImGui::TableNextColumn();
				ImGui::Text("Nodes Selected");
				ImGui::TableNextColumn();
				ImGui::Text("None");
				row_count += 1;
			}
			else if ( my_selected_nodes.size() > 1 )
			{
				ImGui::TableNextColumn();
				ImGui::Text("Nodes Selected");
				ImGui::TableNextColumn();
				ImGui::Text("%zu", my_selected_nodes.size());
				row_count += 1;
			}
			else
			{
				// properties for selected node
				auto  node = std::dynamic_pointer_cast<IsochroneNode>(my_selected_nodes[0]);
				auto  gn = node->GetGraphNode();

				/*
				 * I really don't want to make this an incrementable that *every* one
				 * needs to invoke, but I'm also aligned with hardcoded counts being
				 * highly error-prone and ugly.
				 * We'll do it for now, hopefully imgui will have a table sizing fix
				 * by the time we need to revisit.
				 */
				row_count += 5; // yes, counted by hand

				AddPropertyRow(PropertyRowType::TextReadOnly, "ID", gn->id);
				if ( AddPropertyRow(PropertyRowType::TextMultilineInput, "Data", &gn->datastr) )
				{
					// not a graphnode property! anyone to notify?
				}
				if ( AddPropertyRow(PropertyRowType::FloatInput, "Position", &gn->position) )
				{
					node->SetPosition(gn->position);
				}
				if ( AddPropertyRow(PropertyRowType::FloatInput, "Size", &gn->size) )
				{
					ImVec2  requested_size = gn->size;

					if ( requested_size.x < imgui::node_minimum_width )
					{
						requested_size.x = imgui::node_minimum_width;
					}
					if ( requested_size.y < imgui::node_minimum_height )
					{
						requested_size.y = imgui::node_minimum_height;
					}

					node->SetStaticSize(requested_size);
				}
				if ( AddPropertyRow(PropertyRowType::NodeStyle, "Style", &gn->style) )
				{
					TZK_LOG_FORMAT(LogLevel::Trace, "Setting new node style: %s", gn->style.c_str());
					node->SetStyle(GetNodeStyle(gn->style.c_str()));
				}

				DrawPropertyView_Pins(row_count, gn->pins, node);

				auto  gnc_sysinf = dynamic_cast<node_component_systeminfo*>(node->GetWorkspaceNode()->get_component(cth_cmpt_sysinfo));
				if ( gnc_sysinf != nullptr )
				{
					DrawPropertyView_SystemInfo(row_count, gnc_sysinf->system_info);
				}

				
#if 0  // multi-sys prior code
					AddSeparatorRow("Elements");
					ImGui::SameLine();
					ImGui::HelpMarker("Free-form elements; NO validation is performed");
					// we can validate, easy enough to code. Maybe for future once this mess is split up..

					static std::string  delete_entry;
					static std::string  str_newhost;
					static std::string  str_newip;
					static std::string  str_newiprange;
					static std::string  str_newsubnet;

					auto  disp_list = [&row_count, this](
						std::vector<std::string>& vec, std::string& newstr, const char* disp_label,
						const char* label_delbutton, const char* label_iteminput, const char* label_addbutton
					)
					{
						row_count++;

						for ( auto& e : vec )
						{
							row_count++;

							ImGui::PushID(row_count); // hey, an actual use for this
							AddPropertyRow(PropertyRowType::TextInput, disp_label, &e);
							ImGui::SameLine();
							if ( ImGui::SmallButton(label_delbutton) )
							{
								delete_entry = e;
							}
							ImGui::PopID();
						}
						if ( !delete_entry.empty() )
						{
							auto  res = std::find(vec.begin(), vec.end(), delete_entry);
							if ( (res != vec.end()) )
							{
								TZK_LOG_FORMAT(LogLevel::Trace, "Erasing %s: %s", disp_label, delete_entry.c_str());
								vec.erase(res);
							}
							delete_entry.clear();
						}
						
						ImGui::TableNextColumn();
						// nothing in this column as the text hint displays equivalent
						ImGui::TableNextColumn();
						ImGui::InputTextWithHint(label_iteminput, disp_label, &newstr);
						ImGui::SameLine();
						if ( ImGui::SmallButton(label_addbutton) && !newstr.empty() )
						{
							// prevent duplicates
							auto  res = std::find(vec.begin(), vec.end(), newstr);
							if ( res == vec.end() )
							{
								TZK_LOG_FORMAT(LogLevel::Trace, "Adding %s: %s", disp_label, newstr.c_str());
								vec.emplace_back(newstr);
								newstr.clear();
							}
						}
					};

					disp_list(mnode->graph.hostnames, str_newhost, "Hostname", "Delete##Host", "##new_host", "Add Host");
					disp_list(mnode->graph.ips, str_newip, "IP", "Delete##IP", "##new_ip", "Add IP");
					disp_list(mnode->graph.ip_ranges, str_newiprange, "IP Range", "Delete##IPRange", "##new_iprange", "Add IP Range");
					disp_list(mnode->graph.subnets, str_newsubnet, "Subnet", "Delete##Subnet", "##new_subnet", "Add Subnet");
#endif
				
			}

			ImGui::EndTable();
		}

		// correct size for the next frame
		outer_size.y = ImGui::GetFrameHeight() + row_count * (ImGui::GetTextLineHeight() + ImGui::GetStyle().CellPadding.y * 2.0f);

	} // node properties
	
	// if sysinfo component
	if ( ImGui::CollapsingHeader("System Information", ImGuiTreeNodeFlags_None) )
	{
		// same comments apply as Node Properties above!
		static ImVec2  outer_size = { 0.f, 0.f };
		int  row_count = 4; // checkbox and button, +2 spacing

		ImGuiTableFlags  table_flags = ImGuiTableFlags_Resizable
			| ImGuiTableFlags_NoSavedSettings
			| ImGuiTableFlags_RowBg
			| ImGuiTableFlags_SizingStretchProp
			| ImGuiTableFlags_ScrollY
			| ImGuiTableFlags_HighlightHoveredColumn;

		if ( my_selected_nodes.size() == 1 && ImGui::SmallButton("Edit Hardware") )
		{
			my_open_hardware_popup = true;
		}

		if ( ImGui::BeginTable("nodehw##", 2, table_flags, outer_size) )
		{
			ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch;
			ImGui::TableSetupColumn("", col_flags, 0.3f);
			ImGui::TableSetupColumn("", col_flags, 0.7f);
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();

			row_count += 2;

			if ( my_selected_nodes.size() == 1 )
			{
				auto  node = std::dynamic_pointer_cast<IsochroneNode>(my_selected_nodes[0]);
				auto  gnc_sysinf = dynamic_cast<node_component_systeminfo*>(node->GetWorkspaceNode()->get_component(cth_cmpt_sysinfo));
				
				if ( gnc_sysinf != nullptr )
				{
					DrawPropertyView_SystemInfo(row_count, gnc_sysinf->system_info);
				}
			}
			else
			{
				ImGui::TableNextColumn();
				ImGui::Text("Nodes Selected");
				ImGui::TableNextColumn();
				if ( my_selected_nodes.empty() )
					ImGui::Text("%s", "None");
				else
					ImGui::Text("%zu", my_selected_nodes.size());
			}

			ImGui::EndTable();
		}

		// correct size for the next frame
		outer_size.y = ImGui::GetFrameHeight() + row_count * (ImGui::GetTextLineHeight() + ImGui::GetStyle().CellPadding.y * 2.0f);

	} // systeminfo

	/*
	 * Lambdas for the styles (nodes and pins), to reduce the LoC and make
	 * it a bit clearer to read
	 */
	auto colouredit4 = [](auto& colour, std::string& label)
	{
		ImVec4  f4 = ImGui::ColorConvertU32ToFloat4(colour);
		if ( ImGui::ColorEdit4(label.c_str(), &f4.x, ImGuiColorEditFlags_None) )
		{
			colour = ImGui::ColorConvertFloat4ToU32(f4);
		}
	};
	auto comboshape = [](imgui::PinSocketShape& shape, std::string& label)
	{
		/// @todo grab these from external so this doesn't need touching on amendments
		const char* strs[] = { "Circle", "Square", "Diamond", "Hexagon" };
		int   selected_num = imgui::TConverter<imgui::PinSocketShape>::ToUint8(shape) - 1;
		bool  selected = false;
		int   num = 0;

		if ( ImGui::BeginCombo(label.c_str(), selected_num != -1 ? strs[selected_num] : "") )
		{
			for ( auto& str : strs )
			{
				selected = (num == selected_num);
				if ( ImGui::Selectable(str, &selected) )
				{
					selected_num = num;
					shape = imgui::TConverter<imgui::PinSocketShape>::FromUint8((uint8_t)selected_num + 1); // +1 as 0=Invalid
				}
				num++;
			}
			ImGui::EndCombo();
		}
	};
	auto inputfloat = [](float* f, std::string& label)
	{
		ImGui::InputFloat(label.c_str(), f, 0.f, 0.f, "%.1f", ImGuiInputTextFlags_None);
	};
#if 0  // Code Disabled: unused, keep available
	auto inputfloat2 = [](float* f2, std::string& label)
	{
		ImGui::InputFloat2(label.c_str(), f2, "%.1f", ImGuiInputTextFlags_None);
	};
#endif
	auto inputfloat4 = [](float* f4, std::string& label)
	{
		ImGui::InputFloat4(label.c_str(), f4, "%.1f", ImGuiInputTextFlags_None);
	};
#if 0  // Code Disabled: unused, keep available
	auto inputint = [](auto& i, std::string& label)
	{
		ImGui::InputInt(label.c_str(), &i, 1, 100, ImGuiInputTextFlags_None);
	};
#endif
	if ( ImGui::CollapsingHeader("Node Styles") )
	{
		auto&  ns = my_wksp_data->node_styles;
		static std::string  style_name = "New Style Name";

		/// @todo callback, on focus auto-hide. Default alpha would be good too
		ImGui::InputText("##NewStyleName", &style_name);
		ImGui::SameLine();
		if ( ImGui::Button("Add") && !style_name.empty() )
		{
			auto  new_style = trezanik::imgui::NodeStyle::standard();
			AddNodeStyle(style_name.c_str(), new_style);
			style_name = "New Style Name";
		}

		for ( auto& n : ns )
		{
			ImGui::Separator();
			/*
			 * Renaming is possible, due to update flows it'd be desired to have
			 * this as a dedicated prompt rather than immediate. Allows invocation
			 * of workspace verification and command issuance for rollback.
			 */
			ImGui::Text("%s", n.first.c_str());

			if ( !IsReservedStyleName(n.first.c_str()) )
			{
				ImGui::SameLine();
				std::string  lbl_delete = "Delete##" + n.first;
				if ( ImGui::Button(lbl_delete.c_str()) )
				{
					RemoveNodeStyle(n.first.c_str());
				}
			}

			std::string  lbl_a = "Background##" + n.first;
			std::string  lbl_b = "Border##" + n.first;
			std::string  lbl_c = "Border Selected##" + n.first;
			std::string  lbl_d = "Border Selected Thickness##" + n.first;
			std::string  lbl_e = "Border Thickness##" + n.first;
			std::string  lbl_f = "Header Background##" + n.first;
			std::string  lbl_g = "Header Title##" + n.first;
			std::string  lbl_h = "Margin (Header:LTRB)##" + n.first;
			std::string  lbl_i = "Margin (Body:LTRB)##" + n.first;
			std::string  lbl_j = "Radius##" + n.first;

			colouredit4(n.second->bg, lbl_a);
			colouredit4(n.second->border_colour, lbl_b);
			colouredit4(n.second->border_selected_colour, lbl_c);
			inputfloat(&n.second->border_selected_thickness, lbl_d);
			inputfloat(&n.second->border_thickness, lbl_e);
			colouredit4(n.second->header_bg, lbl_f);
			colouredit4(n.second->header_title_colour, lbl_g);
			inputfloat4(&n.second->margin_header.x, lbl_h);
			inputfloat4(&n.second->margin.x, lbl_i);
			inputfloat(&n.second->radius, lbl_j);
		}
	} // node styles

	if ( ImGui::CollapsingHeader("Pin Styles") )
	{
		auto  ps = my_wksp_data->pin_styles;
		static std::string  style_name = "New Style Name";

		/// @todo callback, on focus auto-hide. Default alpha would be good too
		ImGui::InputText("##NewPinStyleName", &style_name);
		ImGui::SameLine();
		if ( ImGui::Button("Add") && !style_name.empty() )
		{
			auto  new_style = trezanik::imgui::PinStyle::connector();
			AddPinStyle(style_name.c_str(), new_style);
			style_name = "New Style Name";
		}

		for ( auto& s : ps )
		{
			ImGui::Separator();

			ImGui::Text("%s", s.first.c_str());

			if ( !IsReservedStyleName(s.first.c_str()) )
			{
				ImGui::SameLine();
				std::string  lbl_delete = "Delete##" + s.first;
				if ( ImGui::Button(lbl_delete.c_str()) )
				{
					RemovePinStyle(s.first.c_str());
				}

				std::string  lbl_a = "Display##" + s.first;
				std::string  lbl_b = "Image##" + s.first;

				bool  shape_selected = s.second->display == imgui::PinStyleDisplay::Shape;
				bool  image_selected = s.second->display == imgui::PinStyleDisplay::Image;
				const char* strs[] = { "Shape", "Image" };

				if ( ImGui::BeginCombo(lbl_a.c_str(), shape_selected ? strs[0] : strs[1]) )
				{
					if ( ImGui::Selectable("Shape", &shape_selected) )
					{
						image_selected = false;
						s.second->display = imgui::PinStyleDisplay::Shape;
					}
					if ( ImGui::Selectable("Image", &image_selected) )
					{
						shape_selected = false;
						s.second->display = imgui::PinStyleDisplay::Image;
					}
					ImGui::EndCombo();
				}

				ImGui::InputText(lbl_b.c_str(), &s.second->filename);
			}

			std::string  lbl_c = "Link Drag Thickness##" + s.first;
			std::string  lbl_d = "Link Hover Extra Thickness##" + s.first;
			std::string  lbl_e = "Link Selected Thickness##" + s.first;
			std::string  lbl_f = "Link Thickness##" + s.first;
			std::string  lbl_g = "Socket Colour##" + s.first;
			std::string  lbl_h = "Socket Connected Radius##" + s.first;
			std::string  lbl_i = "Socket Hovered Radius##" + s.first;
			std::string  lbl_j = "Socket Radius##" + s.first;
			std::string  lbl_k = "Socket Shape##" + s.first;
			std::string  lbl_l = "Socket Thickness##" + s.first;

			inputfloat(&s.second->link_dragged_thickness, lbl_c);
			inputfloat(&s.second->link_hovered_extra_thickness, lbl_d);
			inputfloat(&s.second->link_selected_thickness, lbl_e);
			inputfloat(&s.second->link_thickness, lbl_f);
			colouredit4(s.second->socket_colour, lbl_g);
			inputfloat(&s.second->socket_connected_radius, lbl_h);
			inputfloat(&s.second->socket_hovered_radius, lbl_i);
			inputfloat(&s.second->socket_radius, lbl_j);
			comboshape(s.second->socket_shape, lbl_k);
			inputfloat(&s.second->socket_thickness, lbl_l);
		}

	} // pin styles
}


void
ImGuiWkspTopology::DrawPropertyView_Pins(
	int& row_count,
	std::vector<pin>& pins,
	std::shared_ptr<IsochroneNode> node
)
{
	using namespace trezanik::core;

	static PinPosition  pp;
	int  pc = 0;

	for ( auto& p : pins )
	{
		pc++;
		ImU32  ec = IM_COL32(186, 189, 94, 255);
		ImU32  oc = IM_COL32(166, 169, 74, 255);

		row_count += 5;

		ImGui::Separator();
		ImGui::PushID(p.id.GetCanonical());
		ImGui::PushStyleColor(ImGuiCol_Text, pc % 2 == 0 ? ec : oc);
		AddPropertyRow(PropertyRowType::TextReadOnly, "Pin.ID", &p.id);
		ImGui::PopStyleColor();

		if ( AddPropertyRow(PropertyRowType::TextInput, "Pin.Name", &p.name) )
		{

		}
		pp.x = &p.pos.x;
		pp.y = &p.pos.y;
		if ( AddPropertyRow(PropertyRowType::FloatInput, "Pin.RelativePosition", &pp) )
		{
			auto  impin = node->GetPin(p.id);

			if ( impin != nullptr )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Setting new pin relative position: %g,%g", p.pos.x, p.pos.y);
				impin->SetRelativePosition(ImVec2(p.pos.x, p.pos.y));
			}
		}
		if ( !IsValidRelativePosition(p.pos.x, p.pos.y) )
		{
			row_count++;
			ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid relative format");
		}
		/// @todo local enum lookup map, present raw type value? Save converting every frame needlessly
		std::string  stype = TConverter<PinType>::ToString(p.type);
		AddPropertyRow(PropertyRowType::TextReadOnly, "Pin.Type", &stype);
		if ( AddPropertyRow(PropertyRowType::PinStyle, "Pin.Style", &p.style) )
		{
			auto  impin = node->GetPin(p.id);

			if ( impin != nullptr )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Setting new pin style: %s", p.style.c_str());
				impin->SetStyle(GetPinStyle(p.style.c_str()));
			}
		}

		/*
		 * Get the imgui pin to access connections, links, etc.
		 * We still need to use the graph_node pin as only that
		 * has the service/group details, so property output is
		 * obtained from a combination of the two
		 */
		auto  ipin = node->GetPin(p.id);

		if ( p.type == PinType::Server )
		{
			if ( p.svc_grp != nullptr )
			{
				AddPropertyRow(PropertyRowType::TextReadOnly, "Pin.Service Group", &p.svc_grp->name);
				AddPropertyRow(PropertyRowType::TextInput, "Pin.ServiceComment", &p.svc_grp->comment);
			}
			else if ( p.svc != nullptr )
			{
				AddPropertyRow(PropertyRowType::TextReadOnly, "Pin.Service", &p.svc->name);
				AddPropertyRow(PropertyRowType::TextInput, "Pin.ServiceComment", &p.svc->comment);
			}
			std::string  numc = std::to_string(ipin->NumConnections());
			AddPropertyRow(PropertyRowType::TextReadOnly, "Pin.Connections", &numc);
		}
		else if ( p.type == PinType::Client )
		{
			std::string  numc = std::to_string(ipin->NumConnections());
			AddPropertyRow(PropertyRowType::TextReadOnly, "Pin.Connections", &numc);
		}
		else if ( p.type == PinType::Connector )
		{
			std::string  conn = std::to_string(ipin->IsConnected());
			AddPropertyRow(PropertyRowType::TextReadOnly, "Pin.Connected", &conn);
		}
		else
		{
			// should be unreachable
		}

		ImGui::PopID();
	}
}


void
ImGuiWkspTopology::DrawPropertyView_SystemInfo(
	int& row_count,
	node_component_systeminfo::system& sysinf
)
{
	// duplicate
	auto  AddSeparatorRow = [](const char* txt)
	{
		ImGui::TableNextColumn();
		ImGui::SeparatorText(txt);
		ImGui::TableNextRow();
	};

	// remember, all IdLabels need to be unique. we could push+pop after each row...
	int  labelid = 0;
	/// @todo pass labelid into AddPropRow, have it inc, push and pop within the method

	for ( auto& elem : sysinf.cpus )
	{
		row_count += 3;

		AddSeparatorRow("CPU");
		ImGui::PushID(++labelid);
		if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, my_hide_empty_fields) )
		{
		}
		ImGui::PopID();
	}
	for ( auto& elem : sysinf.dimms )
	{
		row_count += 5;

		AddSeparatorRow("DIMM");
		ImGui::PushID(++labelid);
		if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Capacity", &elem.capacity, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Slot", &elem.slot, my_hide_empty_fields) )
		{
		}
		ImGui::PopID();
	}
	for ( auto& elem : sysinf.disks )
	{
		row_count += 4;

		AddSeparatorRow("Disk");
		ImGui::PushID(++labelid);
		if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Capacity", &elem.capacity, my_hide_empty_fields) )
		{
		}
		ImGui::PopID();
	}
	for ( auto& elem : sysinf.gpus )
	{
		row_count += 3;

		AddSeparatorRow("GPU");
		ImGui::PushID(++labelid);
		if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, my_hide_empty_fields) )
		{
		}
		ImGui::PopID();
	}
	for ( auto& elem : sysinf.psus )
	{
		row_count += 4;

		AddSeparatorRow("PSU");
		ImGui::PushID(++labelid);
		if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Wattage", &elem.wattage, my_hide_empty_fields) )
		{
		}
		ImGui::PopID();
	}
	for ( auto& elem : sysinf.mobo )
	{
		row_count += 4;

		AddSeparatorRow("Motherboard");
		ImGui::PushID(++labelid);
		if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "BIOS", &elem.bios, my_hide_empty_fields) )
		{
		}
		ImGui::PopID();
	}
	for ( auto& elem : sysinf.os )
	{
		row_count += 4;

		AddSeparatorRow("Operating System");
		ImGui::PushID(++labelid);
		if ( AddPropertyRow(PropertyRowType::TextInput, "Architecture", &elem.arch, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Kernel", &elem.kernel, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Name", &elem.name, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Version", &elem.version, my_hide_empty_fields) )
		{
		}
		ImGui::PopID();
	}
	for ( auto& intf : sysinf.interfaces )
	{
		row_count += 3;

		AddSeparatorRow("Interface");
		ImGui::PushID(++labelid);
		if ( AddPropertyRow(PropertyRowType::TextInput, "Alias", &intf.alias, my_hide_empty_fields) )
		{
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "MAC", &intf.mac, my_hide_empty_fields) )
		{
			trezanik::core::aux::mac_address  macaddr;
			intf.valid_mac = trezanik::core::aux::string_to_macaddr(intf.mac.c_str(), macaddr) == 1;
		}
		if ( !intf.valid_mac && !(my_hide_empty_fields && intf.mac.empty()) )
		{
			row_count++;
			ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
		}
		if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &intf.model, my_hide_empty_fields) )
		{
		}

		for ( auto& ia : intf.addresses )
		{
			row_count += 3;

			AddSeparatorRow("Address");
			ImGui::PushID(++labelid);

			if ( AddPropertyRow(PropertyRowType::TextInput, "IPv4", &ia.address, my_hide_empty_fields) )
			{
				trezanik::core::aux::ip_address  ipaddr;
				ia.valid_address = trezanik::core::aux::string_to_ipaddr(ia.address.c_str(), ipaddr) > 0;
			}
			if ( !ia.valid_address && !(my_hide_empty_fields && ia.address.empty()) )
			{
				row_count++;
				ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
			}
			/*if ( AddPropertyRow(IdLabel::IPv6, "IPv6", &ia.str_ipv6) )
			{
			}*/
			if ( AddPropertyRow(PropertyRowType::TextInput, "Subnet Mask", &ia.mask, my_hide_empty_fields) )
			{
				trezanik::core::aux::ip_address  ipaddr;
				ia.valid_mask = trezanik::core::aux::string_to_ipaddr(ia.mask.c_str(), ipaddr) > 0;
			}
			if ( !ia.valid_mask && !(my_hide_empty_fields && ia.mask.empty()) )
			{
				row_count++;
				ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
			}
			if ( AddPropertyRow(PropertyRowType::TextInput, "Default Gateway", &ia.gateway, my_hide_empty_fields) )
			{
				trezanik::core::aux::ip_address  ipaddr;
				ia.valid_gateway = trezanik::core::aux::string_to_ipaddr(ia.gateway.c_str(), ipaddr) > 0;
			}
			if ( !ia.valid_gateway && !(my_hide_empty_fields && ia.gateway.empty()) )
			{
				row_count++;
				ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
			}
			ImGui::PopID();
		}
		for ( auto& in : intf.nameservers )
		{
			row_count += 1;

			AddSeparatorRow("Nameserver");
			ImGui::PushID(++labelid);
			if ( AddPropertyRow(PropertyRowType::TextInput, "Nameserver", &in.nameserver, my_hide_empty_fields) )
			{
				trezanik::core::aux::ip_address  ipaddr;
				in.valid_nameserver = trezanik::core::aux::string_to_ipaddr(in.nameserver.c_str(), ipaddr) > 0;
			}
			if ( !in.valid_nameserver && !(my_hide_empty_fields && in.nameserver.empty()) )
			{
				row_count++;
				ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
			}
			ImGui::PopID();
		}
		ImGui::PopID();
	}
}


int
ImGuiWkspTopology::DrawServiceSelector()
{
	using namespace trezanik::core;

	int  retval = 0;

	if ( !ImGui::BeginPopupModal(popupname_service_selector) )
	{
		return retval;
	}

	const int  radioval_service = 0;
	const int  radioval_servicegroup = 1;

	/// @bug this isn't working either, can't seem to set a minimum size
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(200, 150));

	// [Group] Radio selector for type
	{
		ImGui::RadioButton("Service", &my_service_selector_radio_value, radioval_service);
		ImGui::SameLine();
		ImGui::RadioButton("Service Group", &my_service_selector_radio_value, radioval_servicegroup);
		ImGui::Separator();
	}
	// [Group] Double table pane (these could easily be listboxes too!
	{
		ImGuiTableFlags  table_flags = ImGuiTableFlags_NoSavedSettings
			| ImGuiTableFlags_RowBg
			| ImGuiTableFlags_SizingStretchProp
			| ImGuiTableFlags_ScrollX
			| ImGuiTableFlags_ScrollY;
		ImVec2  avail = ImGui::GetContentRegionAvail();

		// w = half, with 5px each side
		// h = 75% (can we just do whatever remains after buttons too?)
		ImVec2  outer_size((avail.x * 0.5f) - 10.f, ImGui::GetContentRegionAvail().y * 0.75f);

		if ( ImGui::BeginTable("selector-left##", 1, table_flags, outer_size) )
		{
			ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch;
			ImGui::TableSetupColumn(my_service_selector_radio_value == radioval_service ? "Service" : "Service Group", col_flags, 1.f);
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();
			ImGui::TableNextColumn();// mandatory, otherwise GetCurrentWindow()->SkipItems = true as we have ScrollX/Y, resulting in child window

			int   pos = -1;

			if ( my_service_selector_radio_value == radioval_service )
			{
				for ( auto& svc : my_wksp_data->services )
				{
					const bool  is_selected = (++pos == my_selector_index_service);

					if ( ImGui::Selectable(svc->name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns) )
					{
						if ( my_selector_index_service == pos )
						{
							TZK_LOG_FORMAT(LogLevel::Trace, "Unselected %s: %d (%s)", "Service", pos, svc->name.c_str());
							my_selector_index_service = -1;
						}
						else
						{
							TZK_LOG_FORMAT(LogLevel::Trace, "Selected %s: %d (%s)", "Service", pos, svc->name.c_str());
							my_selector_service = svc;
							my_selector_index_service = pos;
						}
					}
					if ( is_selected )
					{
						ImGui::SetItemDefaultFocus();
					}
					ImGui::TableNextColumn();
				}
			}
			else
			{
				for ( auto& svcg : my_wksp_data->service_groups )
				{
					const bool  is_selected = (++pos == my_selector_index_service_group);

					if ( ImGui::Selectable(svcg->name.c_str(), is_selected, ImGuiSelectableFlags_SpanAllColumns) )
					{
						if ( my_selector_index_service_group == pos )
						{
							TZK_LOG_FORMAT(LogLevel::Trace, "Unselected %s: %d (%s)", "Service Group", pos, svcg->name.c_str());
							my_selector_index_service_group = -1;
						}
						else
						{
							TZK_LOG_FORMAT(LogLevel::Trace, "Selected %s: %d (%s)", "Service Group", pos, svcg->name.c_str());
							my_selector_service_group = svcg;
							my_selector_index_service_group = pos;
						}
					}
					if ( is_selected )
					{
						ImGui::SetItemDefaultFocus();
					}
					ImGui::TableNextColumn();
				}
			}

			ImGui::EndTable();
		}

		ImGui::SameLine();

		if ( my_service_selector_radio_value == radioval_service )
		{
			if ( ImGui::BeginTable("selector-right##", 2, table_flags, outer_size) )
			{
				ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
				ImGui::TableSetupColumn("Property", col_flags, 0.4f);
				ImGui::TableSetupColumn("Value", col_flags, 0.6f);
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				// list all details
				if ( my_selector_service != nullptr )
				{
					ImGui::Text("Comment");
					ImGui::TableNextColumn();
					ImGui::Text("%s", my_selector_service->comment.c_str());
					ImGui::TableNextColumn();

					ImGui::Text("Protocol");
					ImGui::TableNextColumn();
					ImGui::Text("%s", my_selector_service->protocol.c_str());
					ImGui::TableNextColumn();

					if ( my_selector_service->protocol_num == IPProto::icmp )
					{
						ImGui::Text("ICMP Type");
						ImGui::TableNextColumn();
						ImGui::Text("%d", my_selector_service->icmp_type);
						ImGui::TableNextColumn();

						ImGui::Text("ICMP Code");
						ImGui::TableNextColumn();
						ImGui::Text("%d", my_selector_service->icmp_code);
						ImGui::TableNextColumn();
					}
					else
					{
						ImGui::Text("Port Number");
						ImGui::TableNextColumn();
						ImGui::Text("%d", my_selector_service->port_num);
						ImGui::TableNextColumn();

						ImGui::Text("Port Number High");
						ImGui::TableNextColumn();
						ImGui::Text("%d", my_selector_service->port_num_high);
						ImGui::TableNextColumn();
					}
				}

				ImGui::EndTable();
			}
		}
		else
		{
			if ( ImGui::BeginTable("selector-right##", 1, table_flags, outer_size) )
			{
				ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
				ImGui::TableSetupColumn("Member Services", col_flags, 1.f);
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				if ( my_selector_service_group != nullptr )
				{
					// loop all services
					for ( auto& svc : my_selector_service_group->services )
					{
						ImGui::Selectable(svc.c_str());

						ImGui::TableNextColumn();
					}
				}

				ImGui::EndTable();
			}
		}
	} // double table


	ImGui::Separator();

	bool  confirm_disabled = my_service_selector_radio_value == radioval_service ? 
			my_selector_service == nullptr : my_selector_service_group == nullptr;

	if ( confirm_disabled )
	{
		ImGui::BeginDisabled();
	}
	if ( ImGui::Button("Confirm") )
	{
		ImGui::CloseCurrentPopup();
		TZK_LOG(LogLevel::Trace, "Closing Service Selector (confirmed)");
		retval = 1;
		// ensuring only one item is valid
		if ( my_service_selector_radio_value == radioval_service )
		{
			my_selector_service_group = nullptr;
			TZK_LOG_FORMAT(LogLevel::Debug, "Confirmed %s %s", "Service", my_selector_service->name.c_str());
		}
		else
		{
			my_selector_service = nullptr;
			TZK_LOG_FORMAT(LogLevel::Debug, "Confirmed %s %s", "Service Group", my_selector_service_group->name.c_str());
		}
	}
	if ( confirm_disabled )
	{
		ImGui::EndDisabled();
	}
	ImGui::SameLine();
	if ( ImGui::Button("Cancel") )
	{
		ImGui::CloseCurrentPopup();
		TZK_LOG(LogLevel::Trace, "Closing Service Selector (cancelled)");
		my_selector_service_group = nullptr;
		my_selector_service = nullptr;
		my_selector_index_service_group = -1;
		my_selector_index_service = -1;
	}

	ImGui::PopStyleVar();
	ImGui::EndPopup();
	return retval;
}


std::shared_ptr<trezanik::imgui::NodeStyle>
ImGuiWkspTopology::GetNodeStyle(
	const char* name
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;

	for ( auto& s : my_wksp_data->node_styles )
	{
		if ( STR_compare(s.first.c_str(), name, case_sensitive) == 0 )
		{
			return s.second;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Node style '%s' not found", name);

	if ( IsReservedStyleName(name) )
	{
		/*
		 * internal fault if we can't actually find a Default: specification that
		 * should actually exist! Nothing stopping someone from putting in the
		 * 'fake' prefix on purpose, so return the first entry (should be Base)
		 */
		assert(!my_wksp_data->node_styles.empty());
		return my_wksp_data->node_styles.begin()->second;
	}

	return nullptr;
}


std::shared_ptr<trezanik::imgui::PinStyle>
ImGuiWkspTopology::GetPinStyle(
	const char* name
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;

	for ( auto& s : my_wksp_data->pin_styles )
	{
		if ( STR_compare(s.first.c_str(), name, case_sensitive) == 0 )
		{
			return s.second;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Pin style '%s' not found", name);

	if ( IsReservedStyleName(name) )
	{
		/*
		 * internal fault if we can't actually find a Default: specification that
		 * should actually exist! Nothing stopping someone from putting in the
		 * 'fake' prefix on purpose, so return the first entry (should be Base)
		 */
		assert(!my_wksp_data->pin_styles.empty());
		return my_wksp_data->pin_styles.begin()->second;
	}

	return nullptr;
}


std::string
ImGuiWkspTopology::GetPinStyle(
	std::shared_ptr<trezanik::imgui::PinStyle> style
)
{
	for ( auto& s : my_wksp_data->pin_styles )
	{
		if ( s.second == style )
		{
			return s.first;
		}
	}

	return "";
}


std::shared_ptr<service>
ImGuiWkspTopology::GetService(
	const char* name
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;

	for ( auto& svc : my_wksp_data->services )
	{
		if ( STR_compare(name, svc->name.c_str(), case_sensitive) == 0 )
		{
			return svc;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Service '%s' not found", name);

	return nullptr;
}


std::shared_ptr<service>
ImGuiWkspTopology::GetService(
	trezanik::core::UUID& id
)
{
	auto  iter = std::find_if(my_wksp_data->services.begin(), my_wksp_data->services.end(), [&id](auto&& p) {
		return p->id == id;
	});
	if ( iter == my_wksp_data->services.end() )
	{
		return nullptr;
	}

	return *iter;
}


std::shared_ptr<service_group>
ImGuiWkspTopology::GetServiceGroup(
	const char* name
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;

	for ( auto& grp : my_wksp_data->service_groups )
	{
		if ( STR_compare(name, grp->name.c_str(), case_sensitive) == 0 )
		{
			return grp;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Service group '%s' not found", name);

	return nullptr;
}


std::shared_ptr<service_group>
ImGuiWkspTopology::GetServiceGroup(
	trezanik::core::UUID& id
)
{
	auto  iter = std::find_if(my_wksp_data->service_groups.begin(), my_wksp_data->service_groups.end(), [&id](auto&& p) {
		return p->id == id;
	});
	if ( iter == my_wksp_data->service_groups.end() )
	{
		return nullptr;
	}

	return *iter;
}


int
ImGuiWkspTopology::IndexFromNodeStyle(
	std::string* style
)
{
	int  index = 0;
	for ( auto& str : my_wksp_data->node_styles )
	{
		if ( str.first == *style )
			return index;

		index++;
	}

	return -1;
}


int
ImGuiWkspTopology::IndexFromPinStyle(
	std::string* style
)
{
	int  index = 0;

	for ( auto& str : my_wksp_data->pin_styles )
	{
		if ( str.first == *style )
			return index;

		index++;
	}

	return -1;
}


void
ImGuiWkspTopology::Populate()
{
	using namespace trezanik::core;

	/*
	 * Load all the workspaces nodes into ImNodes mappings.
	 * All operations are performed on these until the window is closed or
	 * otherwise saved, at which point they are synchronized back.
	 */
	auto& nodes = my_wksp_data->nodes;
	
	for ( auto& iter : nodes )
	{
		AddNode(iter);
	}

	for ( auto& iter : my_wksp_data->links )
	{
		const trezanik::app::pin*  psrc = nullptr;
		const trezanik::app::pin*  ptgt = nullptr;
		std::shared_ptr<trezanik::imgui::BaseNode>  nsrc;
		std::shared_ptr<trezanik::imgui::BaseNode>  ntgt;
		std::shared_ptr<trezanik::imgui::Pin> impin_out;
		std::shared_ptr<trezanik::imgui::Pin> impin_inp;

		/*
		 * Not a fan but works.
		 * Goal is to identify all the imgui:: types from the app:: types, as
		 * loaded into the workspace
		 */
		for ( auto& n : nodes )
		{
			for ( auto& p : n->graph.pins )
			{
				// find the pin id for source and target
				if ( p.id == iter->source || p.id == iter->target )
				{
					TZK_LOG_FORMAT(LogLevel::Trace,
						"Found link %s in node %s for link: %s -> %s",
						p.id == iter->source ? "source" : "target",
						n->id.GetCanonical(),
						iter->source.GetCanonical(), iter->target.GetCanonical()
					);

					// find the IsochroneNode* for the app::graph_node_system
					if ( p.type == PinType::Server )
					{
						ptgt = &p;

						auto niter = std::find_if(my_nodes.begin(), my_nodes.end(), [&n](auto&& node)
						{
							//TZK_LOG_FORMAT(LogLevel::Trace, "%s : %s", node->GetID().GetCanonical(), n->id.GetCanonical());
							return node->GetID() == n->id;
						});
						if ( niter == my_nodes.end() )
						{
							TZK_LOG_FORMAT(LogLevel::Error,
								"No matching graph node %s found for link: %s -> %s",
								"target", iter->source.GetCanonical(), iter->target.GetCanonical()
							);
							continue;
						}
						
						ntgt = *niter;
					}
					else if ( p.type == PinType::Client )
					{
						psrc = &p;
						auto niter = std::find_if(my_nodes.begin(), my_nodes.end(), [&n](auto&& node)
						{
							//TZK_LOG_FORMAT(LogLevel::Trace, "%s : %s", node->GetID().GetCanonical(), n->id.GetCanonical());
							return node->GetID() == n->id;
						});
						if ( niter == my_nodes.end() )
						{
							TZK_LOG_FORMAT(LogLevel::Error,
								"No matching graph node %s found for link: %s -> %s",
								"source", iter->source.GetCanonical(), iter->target.GetCanonical()
							);
							continue;
						}
						
						nsrc = *niter;
					}
					else if ( p.type == PinType::Connector )
					{
						auto niter = std::find_if(my_nodes.begin(), my_nodes.end(), [&n](auto&& node)
						{
							//TZK_LOG_FORMAT(LogLevel::Trace, "%s : %s", node->GetID().GetCanonical(), n->id.GetCanonical());
							return node->GetID() == n->id;
						});
						if ( niter == my_nodes.end() )
						{
							TZK_LOG_FORMAT(LogLevel::Error,
								"No matching graph node found for link: %s -> %s",
								iter->source.GetCanonical(), iter->target.GetCanonical()
							);
							continue;
						}
						if ( psrc == nullptr )
						{
							psrc = &p;
							nsrc = *niter;
						}
						else if ( ptgt == nullptr )
						{
							ptgt = &p;
							ntgt = *niter;
						}
					}

					// nodes can't link to themselves
					break;
				}
			}

			if ( psrc != nullptr && nsrc != nullptr && ptgt != nullptr && ntgt != nullptr)
			{
				// find the trezanik::imgui::Pin for src from the app::pin
				for ( auto& srcpin : nsrc->GetPins() )
				{
					if ( srcpin->GetID() == psrc->id )
					{
						impin_out = srcpin;
						break;
					}
				}
				// and again for the target
				for ( auto& tgtpin : ntgt->GetPins() )
				{
					if ( tgtpin->GetID() == ptgt->id )
					{
						impin_inp = tgtpin;
						break;
					}
				}
			}
		}

		if ( psrc == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"No source found for link: %s -> %s",
				iter->source.GetCanonical(), iter->target.GetCanonical()
			);
			continue;
		}
		if ( ptgt == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"No target found for link: %s -> %s",
				iter->source.GetCanonical(), iter->target.GetCanonical()
			);
			continue;
		}
		if ( impin_inp == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"No target pin found for link: %s -> %s",
				iter->source.GetCanonical(), iter->target.GetCanonical()
			);
			continue;
		}
		if ( impin_out == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"No source pin found for link: %s -> %s",
				iter->source.GetCanonical(), iter->target.GetCanonical()
			);
			continue;
		}

		TZK_LOG_FORMAT(LogLevel::Trace,
			"Creating Link for " TZK_PRIxPTR " to " TZK_PRIxPTR,
			impin_out.get(), impin_inp.get()
		);

		
		// create the link in the nodegraph itself
		auto  ngl = my_nodegraph.CreateLink<imgui::Link>(iter->id,
			impin_out, impin_inp, &my_nodegraph,
			&iter->text, (ImVec2*)&iter->offset, &iter->method
		);
		// assign the link to the pins
		impin_inp->AssignLink(ngl);
		impin_out->AssignLink(ngl);


		/*
		 * Pin CreateLink and here are the sources of 'creation' and therefore
		 * tooltips, without being per-frame. Listeners are always handled via
		 * their constructor, then dynamic updates
		 */
		std::string  tt = "Connected to:\n";
		auto iscn = dynamic_cast<IsochroneNode*>(impin_inp->GetAttachedNode());
		tt += iscn->GetWorkspaceNode()->name;
		impin_out->SetTooltipText(tt);
	}

	// no-op needed for node_styles

	// no-op needed for pin_styles

	// no-op needed for services

	// no-op needed for service_groups


	my_wksp->AssignDockClient("Canvas Debug", my_wksp->my_settings->settings.dock_canvasdbg, std::bind(&trezanik::imgui::ImNodeGraph::DrawDebug, &my_nodegraph), drawclient_canvasdbg_uuid);
	my_wksp->AssignDockClient("Property View", my_wksp->my_settings->settings.dock_propview, std::bind(&ImGuiWkspTopology::DrawPropertyView, this), drawclient_propview_uuid);
}


int
ImGuiWkspTopology::RemoveNode(
	trezanik::imgui::BaseNode* node
)
{
	using namespace trezanik::core;

	if ( node == nullptr )
	{
		// won't make this an error, unlikely to be user initiated
		TZK_LOG(LogLevel::Warning, "ImGui node is a nullptr");
		TZK_DEBUG_BREAK;
		return EINVAL;
	}

	/*
	 * we do removal of node graph items when manipulated by the user
	 * 
	 * ImNodeGraph is unaware of anything except drawing (base) nodes, so we must
	 * be the one to tell it to purge. No event dispatch needed as a result, despite
	 * its distribution being desired for consistency
	 */

	if ( !node->IsPendingDestruction() )
	{
		// remove from visual grid (marked only)
		my_nodegraph.DeleteNode(node);
	}

	auto gns_iter = std::find_if(my_nodes.begin(), my_nodes.end(), [&node](auto& p)
	{
		return p->GetID() == node->GetID();
	});
	if ( gns_iter == my_nodes.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Could not find node for '%s'",
			node->GetID().GetCanonical()
		);

		return ENOENT;
	}

	// remove from internal state only; no effect to the graph or dataset
	TZK_LOG_FORMAT(LogLevel::Debug, "Removing node '%s' from topology", node->GetID().GetCanonical());
	my_nodes.erase(gns_iter);

#if 0 // Code Disabled: We must not touch workspace data here, Workspace handles it in event handler
	auto dat_iter = std::find_if(my_wksp_data->nodes.begin(), my_wksp_data->nodes.end(), [&node](auto& p)
	{
		return p->id == node->GetID();
	});
	if ( dat_iter == my_wksp_data->nodes.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Could not find node for '%s'",
			node->GetID().GetCanonical()
		);
		return ENOENT;
	}
	
	TZK_LOG_FORMAT(LogLevel::Debug, "Removing node '%s' from dataset", node->GetID().GetCanonical());
	my_wksp_data->nodes.erase(dat_iter);
#endif

	return ErrNONE;
}


int
ImGuiWkspTopology::RemoveNode(
	std::shared_ptr<workspace_node> node
)
{
	using namespace trezanik::core;

	// get the BaseNode* (as a IsochroneNode*) from the workspace_node
	auto mapiter = std::find_if(my_nodes.begin(), my_nodes.end(), [&node](auto& p)
	{
		return p->GetID() == node->id;
	});
	
	if ( mapiter == my_nodes.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Could not find node for '%s'",
			node->id.GetCanonical()
		);

		return ENOENT;
	}

	/*
	 * Repeat lookup in this method, worth it to save repeating the cleanup code
	 * here and missing something in future
	 */
	return RemoveNode(mapiter->get());
}


int
ImGuiWkspTopology::RemoveNodeStyle(
	std::shared_ptr<trezanik::imgui::NodeStyle> style
)
{
	using namespace trezanik::core;

	auto  iter = std::find_if(my_wksp_data->node_styles.begin(), my_wksp_data->node_styles.end(), [style](auto entry)
	{
		return entry.second == style;
	});

	if ( iter == my_wksp_data->node_styles.end() )
	{
		TZK_LOG(LogLevel::Warning, "Node style not found");
		return ENOENT;
	}

	if ( IsReservedStyleName(iter->first.c_str()) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Removing inbuilt style '%s' denied", iter->first.c_str());
		return EACCES;
	}

	size_t  count = 0;

	for ( auto& n : my_wksp_data->nodes )
	{
		if ( STR_compare(n->graph.style.c_str(), iter->first.c_str(), false) == 0 )
		{
			count++;
		}
	}

	if ( count > 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Unable to remove style '%s' - is in use by %zu nodes", iter->first.c_str(), count);
		return EBUSY;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Removing style '%s'", iter->first.c_str());
	my_wksp_data->node_styles.erase(iter);

	return ErrNONE;
}


int
ImGuiWkspTopology::RemoveNodeStyle(
	const char* name
)
{
	using namespace trezanik::core;

	if ( IsReservedStyleName(name) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Removing inbuilt style '%s' denied", name);
		return EACCES;
	}

	size_t  count = 0;

	for ( auto& n : my_wksp_data->nodes )
	{
		if ( STR_compare(n->graph.style.c_str(), name, false) == 0 )
		{
			count++;
		}
	}

	if ( count > 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Unable to remove style '%s' - is in use by %zu nodes", name, count);
		return EBUSY;
	}

	auto  iter = std::find_if(my_wksp_data->pin_styles.begin(), my_wksp_data->pin_styles.end(), [name](auto& entry)
	{
		return entry.first.compare(name) == 0;
	});
	if ( iter == my_wksp_data->pin_styles.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Node style '%s' not found", name);
		return ENOENT;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Removing style '%s'", name);
	my_wksp_data->pin_styles.erase(iter);

	return ErrNONE;
}


int
ImGuiWkspTopology::RemovePinStyle(
	std::shared_ptr<trezanik::imgui::PinStyle> style
)
{
	using namespace trezanik::core;

	auto  iter = std::find_if(my_wksp_data->pin_styles.begin(), my_wksp_data->pin_styles.end(), [style](auto& entry)
	{
		return entry.second == style;
	});

	if ( iter == my_wksp_data->pin_styles.end() )
	{
		TZK_LOG(LogLevel::Warning, "Pin style not found");
		return ENOENT;
	}
	
	if ( IsReservedStyleName(iter->first.c_str()) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Removing inbuilt style '%s' denied", iter->first.c_str());
		return EACCES;
	}

	size_t  count = 0;

	for ( auto& n : my_wksp_data->nodes )
	{
		for ( auto& p : n->graph.pins )
		{
			if ( STR_compare(p.style.c_str(), iter->first.c_str(), false) == 0 )
			{
				count++;
			}
		}
	}

	if ( count > 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Unable to remove style '%s' - is in use by %zu pins", iter->first.c_str(), count);
		return EBUSY;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Removing style '%s'", iter->first.c_str());
	my_wksp_data->pin_styles.erase(iter);

	return ErrNONE;
}


int
ImGuiWkspTopology::RemovePinStyle(
	const char* name
)
{
	using namespace trezanik::core;

	size_t  count = 0;

	// 1) Reserved Name
	if ( IsReservedStyleName(name) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Removing inbuilt style '%s' denied", name);
		return EACCES;
	}

	// 2) Still in use
	for ( auto& n : my_wksp_data->nodes )
	{
		for ( auto& p : n->graph.pins )
		{
			if ( STR_compare(p.style.c_str(), name, false) == 0 )
			{
				count++;
			}
		}
	}

	if ( count > 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Unable to remove style '%s' - is in use by %zu nodes", name, count);
		return EBUSY;
	}

	// 3) Actually found
	auto  iter = std::find_if(my_wksp_data->pin_styles.begin(), my_wksp_data->pin_styles.end(), [name](auto& entry)
	{
		return entry.first.compare(name) == 0;
	});
	if ( iter == my_wksp_data->pin_styles.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Pin style '%s' not found", name);
		return ENOENT;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Removing style '%s'", name);
	my_wksp_data->pin_styles.erase(iter);

	return ErrNONE;
}


int
ImGuiWkspTopology::RenameNodeStyle(
	const char* original_name,
	const char* new_name
)
{
	using namespace trezanik::core;

	if ( IsReservedStyleName(original_name) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Renaming inbuilt node style '%s' denied", original_name);
		return EACCES;
	}

	for ( auto& s : my_wksp_data->node_styles )
	{
		if ( STR_compare(s.first.c_str(), original_name, true) == 0 )
		{
			s.first = new_name;
			TZK_LOG_FORMAT(LogLevel::Info, "Node style '%s' renamed to '%s'", original_name, new_name);
			//AddCommand
			return ErrNONE;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Node style '%s' not found", original_name);
	return ENOENT;
}


int
ImGuiWkspTopology::RenamePinStyle(
	const char* original_name,
	const char* new_name
)
{
	using namespace trezanik::core;

	if ( IsReservedStyleName(original_name) )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Renaming inbuilt pin style '%s' denied", original_name);
		return EACCES;
	}

	for ( auto& s : my_wksp_data->pin_styles )
	{
		if ( STR_compare(s.first.c_str(), original_name, true) == 0 )
		{
			s.first = new_name;
			TZK_LOG_FORMAT(LogLevel::Info, "Pin style '%s' renamed to '%s'", original_name, new_name);
			//AddCommand
			return ErrNONE;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Pin style '%s' not found", original_name);
	return ENOENT;
}


void
ImGuiWkspTopology::SortNodes()
{
	using namespace trezanik::core;

	SortNodeMethod  sort_method = static_cast<SortNodeMethod>(my_wksp->my_settings->settings.nodelist_sort_order);

	TZK_LOG_FORMAT(LogLevel::Debug, "Sorting nodes using method: %s", TConverter<SortNodeMethod>::ToString(sort_method).c_str());

	auto&  vecref = my_wksp_data->nodes;
	switch ( sort_method )
	{
	case SortNodeMethod::Chronological_Forward:
		std::sort(vecref.begin(), vecref.end(), SortNodes_ChronoForward());
		break;
	case SortNodeMethod::Chronological_Reverse:
		std::sort(vecref.begin(), vecref.end(), SortNodes_ChronoReverse());
		break;
	case SortNodeMethod::Alphabetical_Forward:
		std::sort(vecref.begin(), vecref.end(), SortNodes_AlphaForward());
		break;
	case SortNodeMethod::Alphabetical_Reverse:
		std::sort(vecref.begin(), vecref.end(), SortNodes_AlphaReverse());
		break;
	case SortNodeMethod::Targets_Forward:
		std::sort(vecref.begin(), vecref.end(), SortNodes_TargetForward());
		break;
	case SortNodeMethod::Targets_Reverse:
		std::sort(vecref.begin(), vecref.end(), SortNodes_TargetReverse());
		break;
	default:
		TZK_LOG_FORMAT(LogLevel::Error, "No sorting applied; invalid method (%u)", static_cast<uint8_t>(sort_method));
	}
}


void
ImGuiWkspTopology::UpdatePinTooltip(
	trezanik::imgui::Pin* pin
) const
{
	std::string  tooltip;
	
	/** @todo resolve duplicate defs in app, ala PinType::Server */

	if ( pin->Type() == imgui::PinType_Server )
	{
		/*
		 * Server pin; show the service details only.
		 * Can show all connectors, but needs spacing/layout consideration!
		 * 
		 * This is set in the ServerPin constructor, but that won't receive
		 * dynamic updates (i.e. renames), so still needs doing here.
		 */
		ServerPin*  sp = static_cast<ServerPin*>(pin);
		if ( sp->IsServiceGroup() )
			tooltip = sp->GetServiceGroup()->name;
		else
			tooltip = sp->GetService()->name;
	}
	else if ( pin->Type() == imgui::PinType_Client && pin->IsConnected() )
	{
		/*
		 * Client pin that is also connected; show what it connects to.
		 * Need to test a huge amount and truncate as needed
		 */
		tooltip = "Connected to:";

		for ( auto& l : pin->GetLinks() )
		{
			ServerPin*  sp = static_cast<ServerPin*>(l->Target().get());
			
			tooltip += "\n";

			auto iscn = dynamic_cast<IsochroneNode*>(l->Target()->GetAttachedNode());
			tooltip += iscn->GetWorkspaceNode()->name;
#if 0  // Code Disabled: Pins with names ?
			tooltip += " [";
			if ( !sp->GetName()->empty() )
			{
				tooltip += sp->Name();
			}
			tooltip += "] : ";
#else
			tooltip += " : ";
#endif

			if ( sp->IsServiceGroup() )
				tooltip += sp->GetServiceGroup()->name;
			else
				tooltip += sp->GetService()->name;
		}
	}
	else if ( pin->Type() == imgui::PinType_Connector && pin->IsConnected() )
	{
		// Connector pin that's connected
		tooltip = "Connected to: ";
		
		// Connector types can only have one link
		for ( auto& l : pin->GetLinks() )
		{
			bool  is_source = pin == l->Source().get();
			auto  iscn_src = dynamic_cast<IsochroneNode*>(l->Source()->GetAttachedNode());
			auto  iscn_tgt = dynamic_cast<IsochroneNode*>(l->Target()->GetAttachedNode());

			tooltip += is_source ? iscn_tgt->GetWorkspaceNode()->name : iscn_src->GetWorkspaceNode()->name;
			//tooltip += is_source ? *l->Target()->GetAttachedNode()->GetID().GetCanonical() : *l->Source()->GetAttachedNode()->GetID().GetCanonical();
		}
	}
	else
	{
		// Client/Connector pin, disconnected. Blank tooltip = no display
	}

	pin->SetTooltipText(tooltip);
}


void
ImGuiWkspTopology::UpdateWorkspaceData()
{
	// run through nodes, auto-create/delete here
#if 0
	for ( auto& n : my_wksp_data->nodes )
	{

	}
#endif

	/*
	 * For now, this is used for node and pin style changes only. Nodes being
	 * modified will be ignored, otherwise we might as well re-call SetWorkspace
	 * and delete everything (which I'm trying to avoid).
	 * 
	 * Invalid styles (e.g. custom one assigned now deleted) are fine as they'll
	 * revert to defaults if not found
	 */

	for ( auto& n : my_nodes )
	{
		n->SetStyle(GetNodeStyle(n->GetWorkspaceNode()->graph.style.c_str()));

		for ( auto& p : n->GetPins() )
		{
			for ( auto& ps : n->GetWorkspaceNode()->graph.pins )
			{
				if ( ps.id == p->GetID() )
				{
					p->SetStyle(GetPinStyle(ps.style.c_str()));
				}
			}
		}
	}
}





/**************************************************************
 * Topology children, potentially relocate to dedicated files *
 **************************************************************/


// ---------------------
// IsochroneNode

IsochroneNode::IsochroneNode(
	trezanik::core::UUID& id,
	ImGuiWorkspace* imwksp,
	std::shared_ptr<workspace_node> wnode
)
: BaseNode(id)
, my_wksp_node(wnode)
, _workspace(imwksp)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_header_text = &my_wksp_node->name;
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


IsochroneNode::~IsochroneNode()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		// custom components needing special handling? :: GetWorkspaceNode()->has_component(...);
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
IsochroneNode::Draw()
{
	BaseNode::Draw(); // invokes DrawContent
}


void
IsochroneNode::DrawContent()
{
	ImGui::PushID(this);

	ImGui::TextWrapped("%s", my_wksp_node->graph.datastr.c_str());

	ImGui::PopID();
}



// ---------------------
// ClientPin

void
ClientPin::CreateLink(
	trezanik::imgui::Pin* other
)
{
	using namespace trezanik::core;

	if ( other == nullptr )
	{
		TZK_LOG(LogLevel::Error, "'Other' pin is a nullptr");
		return;
	}
	if ( other == this )
	{
		TZK_LOG(LogLevel::Warning, "'Other' pin is itself");
		return;
	}
	if ( other->Type() == imgui::PinType_Client )
	{
		TZK_LOG(LogLevel::Error, "Source and target pins are both Clients");
		return;
	}

	// call the Server method
	other->CreateLink(this);
}


void
ClientPin::RemoveLink(
	std::shared_ptr<trezanik::imgui::Link> link
)
{
	// ensure original method is invoked
	Pin::RemoveLink(link);

	// custom actions

	SetTooltipText("");
}


// ---------------------
// ConnectorPin

void
ConnectorPin::CreateLink(
	trezanik::imgui::Pin* other
)
{
	using namespace trezanik::core;

	if ( other == nullptr )
	{
		TZK_LOG(LogLevel::Error, "'Other' pin is a nullptr");
		return;
	}
	if ( other == this )
	{
		TZK_LOG(LogLevel::Warning, "'Other' pin is itself");
		return;
	}
	if ( other->Type() != imgui::PinType_Connector )
	{
		TZK_LOG(LogLevel::Error, "Both pin types must be Connectors");
		return;
	}
	// check this pin isn't already connected, but also the target!
	if ( IsConnected() || other->IsConnected() )
	{
		TZK_LOG(LogLevel::Error, "Connector pins can only have a single link at a time");
		return;
	}

	// Identical to ServerPin creation

	auto  node = dynamic_cast<IsochroneNode*>(GetAttachedNode());
	assert(node != nullptr);
	auto  othernode = other->GetAttachedNode();
	if ( othernode == node )
	{
		TZK_LOG(LogLevel::Error, "Pins cannot connect on the same node");
		return;
	}

	auto  imwksp = node->GetWorkspace();
	// create in ImGuiWorkspace data (and forwards to nodegraph)
	auto  imlink = imwksp->GetTopology()->CreateLink(other->shared_from_this(), shared_from_this());

	// for nodegraph only: track the link object
	AssignLink(imlink);
	// other pin needs to track it too
	other->AssignLink(imlink);
}


void
ConnectorPin::RemoveLink(
	std::shared_ptr<trezanik::imgui::Link> link
)
{
	// ensure original method is invoked
	Pin::RemoveLink(link);

	SetTooltipText("");
}


// ---------------------
// ServerPin

void
ServerPin::CreateLink(
	trezanik::imgui::Pin* other
)
{
	using namespace trezanik::core;
	using namespace trezanik::imgui;

	if ( other == nullptr )
	{
		TZK_LOG(LogLevel::Error, "'Other' pin is a nullptr");
		return;
	}
	if ( other == this )
	{
		TZK_LOG(LogLevel::Warning, "'Other' pin is itself");
		return;
	}
	if ( other->Type() != imgui::PinType_Client )
	{
		TZK_LOG(LogLevel::Error, "Server Pins can only have connections from Client Pins");
		return;
	}

	// uint8_t max connections enforced
	if ( _links.size() >= max_connections )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Maximum link count reached for Pin %s", this->GetID().GetCanonical());
		return;
	}

	// check that this pin doesn't already have the 'other' pin linked

	auto  node = dynamic_cast<IsochroneNode*>(GetAttachedNode());
	assert(node != nullptr);
	if ( other->GetAttachedNode() == node )
	{
		TZK_LOG(LogLevel::Error, "Pins cannot connect on the same node");
		return;
	}
	for ( auto& l : other->GetLinks() )
	{
		/*
		 * Note:
		 * It's still possible to have a different client pin (from the same
		 * source node) connecting to this pin, and it won't be flagged. Don't
		 * consider this a fault, if a user wants to do something so strange
		 * it's up to them.
		 */
		if ( l->Source().get() == other && l->Target().get() == this )
		{
			TZK_LOG(LogLevel::Error, "Duplicate link denied");
			return;
		}
	}

	auto  imwksp = node->GetWorkspace();
	// create in ImGuiWorkspace data (and forwards to nodegraph)
	auto  imlink = imwksp->GetTopology()->CreateLink(other->shared_from_this(), shared_from_this());
	// for nodegraph only: track the link object
	AssignLink(imlink);
	// other pin needs to track it too
	other->AssignLink(imlink);
}


void
ServerPin::RemoveLink(
	std::shared_ptr<trezanik::imgui::Link> link
)
{
	// ensure original method is invoked
	Pin::RemoveLink(link);

	// custom actions

}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI
