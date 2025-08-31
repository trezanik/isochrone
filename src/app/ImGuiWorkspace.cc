/**
 * @file        src/app/ImGuiWorkspace.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

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
#include "core/TConverter.h"
#include "core/error.h"

#include "imgui/dear_imgui/imgui_internal.h"
#include "imgui/CustomImGui.h"
#include "imgui/ImNodeGraphPin.h"
#include "imgui/ImNodeGraphLink.h"
#include "imgui/BaseNode.h"

#include <algorithm>
#include <sstream>


namespace trezanik {
namespace app {


constexpr char  popupname_hardware[] = "Hardware";
constexpr char  popupname_service_group[] = "Service Group";
constexpr char  popupname_service_selector[] = "Service Selector";
constexpr char  popupname_service[] = "Service";

static trezanik::core::UUID  drawclient_canvasdbg_uuid("9cbc06c0-c1e6-472c-a73a-1855039b1a1f");
static trezanik::core::UUID  drawclient_propview_uuid("9a663f51-9162-4bec-964e-dd5f3da2db8e");



/**
 * Enumeration for Service Management selection and control flow
 * 
 * @sa ServiceManagementSelection
 */
enum class SvcMgmtSwitch : uint8_t
{
	Select_ServiceGroup,
	Select_ServiceGroupService,
	Select_Service,
	Include,
	Unselect_ServiceGroup,
	Unselect_ServiceGroupService,
	Unselect_Service,
	Exclude
};



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


ImGuiWorkspace::ImGuiWorkspace(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, my_evtmgr(*core::ServiceLocator::EventDispatcher())
, my_context_node(nullptr)
, my_context_pin(nullptr)
, my_context_link(nullptr)
, my_selected_service_group_service_index(-1)
, my_selected_service_group_index(-1)
, my_selected_service_index(-1)
, my_open_service_selector_popup(false)
, my_service_selector_radio_value(0)
, my_selector_index_service(-1)
, my_selector_index_service_group(-1)
, my_open_hardware_popup(false)
, my_draw_hardware_popup(false)
, my_commands_pos(0)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		// default; 128 records retained. happy to make configurable + dynamic
		//my_commands.reserve(128);
		// default; 32 records retained. happy to make configurable + dynamic
		my_undoredo_nodes.reserve(32);

		// immediate binding, nothing variable
		my_nodegraph.ContextPopUpContent(
			std::bind(&ImGuiWorkspace::ContextPopup, this, std::placeholders::_1)
		);
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiWorkspace::~ImGuiWorkspace()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		std::lock_guard<std::mutex>  lock(_gui_interactions.mutex);

		auto  evtmgr = core::ServiceLocator::EventDispatcher();

		for ( auto& id : my_reg_ids )
		{
			evtmgr->Unregister(id);
		}

		/*
		 * Would never expect these to be executed on application closure!
		 * AppImGui destructor is the expected purging point, as it brings
		 * everything down (i.e. the docks) before this.
		 * 
		 * For a standard closure of the workspace without the application
		 * being closed, then yes, expect these invoked.
		 */
		for ( auto& dc : my_draw_clients )
		{
			switch ( dc->dock )
			{
			case WindowLocation::Bottom: _gui_interactions.dock_bottom->RemoveDrawClient(dc); break;
			case WindowLocation::Left:   _gui_interactions.dock_left->RemoveDrawClient(dc); break;
			case WindowLocation::Right:  _gui_interactions.dock_right->RemoveDrawClient(dc); break;
			case WindowLocation::Top:    _gui_interactions.dock_top->RemoveDrawClient(dc); break;
			default: break;
			}
		}
		my_draw_clients.clear();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


template<>
int
ImGuiWorkspace::AddNode(
	std::shared_ptr<graph_node_boundary> gn
)
{
	using namespace trezanik::core;

	auto sptr = my_nodegraph.CreateNode<BoundaryNode>(
		ImVec2((float)gn->position.x, (float)gn->position.y), gn, this
	);

	/*
	 * CreateNode can only fail if make_shared fails, which throws, so this will
	 * never be hit. Retaining in prep for if the parameters passed in, or
	 * further operations, establish grounds for a failure.
	 */
	if ( sptr == nullptr )
	{
		TZK_LOG(LogLevel::Error, "Failed to create new NodeGraph BoundaryNode");
		return ErrFAILED;
	}

	if ( gn->style.empty() )
	{
		gn->style = reserved_style_boundary.c_str();
	}

	sptr->SetName(&gn->name);
	sptr->SetStyle(GetNodeStyle(gn->style.c_str()));

	gn->position = ImVec2((float)gn->position.x, (float)gn->position.y);

	if ( gn->size.y != 0 && gn->size.x != 0 )
	{
		sptr->SetStaticSize(gn->size);
	}
	
	AddNodePins(sptr, gn->pins);

	my_wksp_data.nodes.insert(gn);
	/*
	 * consider making the internal nodes reference the data set, saves
	 * [num_nodes * shared_pointer_size] memory
	 */
	my_nodes[gn] = sptr;

	sptr->AddListener(this);
	sptr->NotifyListeners(imgui::NodeUpdate::Created);

	return ErrNONE;
}


template<>
int
ImGuiWorkspace::AddNode(
	std::shared_ptr<graph_node_multisystem> gn
)
{
	using namespace trezanik::core;
	using namespace trezanik::imgui;

	auto sptr = my_nodegraph.CreateNode<MultiSystemNode>(
		ImVec2((float)gn->position.x, (float)gn->position.y), gn, this
	);

	if ( sptr == nullptr )
	{
		TZK_LOG(LogLevel::Error, "Failed to create new NodeGraph MultiSystemNode");
		return ErrFAILED;
	}

	if ( gn->style.empty() )
	{
		gn->style = reserved_style_multisystem.c_str();
	}

	sptr->SetName(&gn->name);
	sptr->SetStyle(GetNodeStyle(gn->style.c_str()));

	gn->position = ImVec2((float)gn->position.x, (float)gn->position.y);

	if ( gn->size.y != 0 && gn->size.x != 0 )
	{
		sptr->SetStaticSize(gn->size);
	}

	AddNodePins(sptr, gn->pins);

	my_wksp_data.nodes.insert(gn);
	my_nodes[gn] = sptr;

	sptr->AddListener(this);
	sptr->NotifyListeners(imgui::NodeUpdate::Created);

	return ErrNONE;
}


template<>
int
ImGuiWorkspace::AddNode(
	std::shared_ptr<graph_node_system> gn
)
{
	using namespace trezanik::core;
	using namespace trezanik::imgui;

	auto sptr = my_nodegraph.CreateNode<SystemNode>(
		ImVec2((float)gn->position.x, (float)gn->position.y), gn, this
	);

	if ( sptr == nullptr )
	{
		TZK_LOG(LogLevel::Error, "Failed to create new NodeGraph SystemNode");
		return ErrFAILED;
	}

	/*
	 * Ensure the graph node style is populated even if we're using the default
	 * styles; this allows the combo box/whatever to display the active style,
	 * which is more desired than a blank box entry - when the list does actually
	 * contain the style in use.
	 * We make sure not to save this, if it's the default however.
	 */
	if ( gn->style.empty() )
	{
		gn->style = reserved_style_system.c_str();
	}
	
	sptr->SetName(&gn->name);
	sptr->SetStyle(GetNodeStyle(gn->style.c_str()));

	gn->position = ImVec2((float)gn->position.x, (float)gn->position.y);

	if ( gn->size.y != 0 && gn->size.x != 0 )
	{
		sptr->SetStaticSize(gn->size);
	}

	AddNodePins(sptr, gn->pins);
	
	
	/*
	 * track this node shared_ptr, all graph node operations done with these
	 * If loaded from file (via SetWorkspace), this will be a duplicate and the
	 * set will skip insertion.
	 */
	my_wksp_data.nodes.insert(gn);
	my_nodes[gn] = sptr;

	sptr->AddListener(this);

	/*
	 * Notify listeners of this creation; if this function is called with a new
	 * node (one that we created, not loaded from file), the workspace will have
	 * no knowledge of it - a bit of a problem when you come to save.
	 * We put all workspace updating inside a dedicated method for ease of
	 * visibility (yes, even when we are the listener).
	 */
	sptr->NotifyListeners(imgui::NodeUpdate::Created);

	return ErrNONE;
}


void
ImGuiWorkspace::AddNodePins(
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
ImGuiWorkspace::AddNodeStyle(
	const char* name,
	std::shared_ptr<trezanik::imgui::NodeStyle> style
)
{
	using namespace trezanik::core;

	/*
	 * Iterate the vector and locate the name, which must be unique in the set.
	 * As noted in class documentation, this cannot be a direct map/set
	 */
	for ( auto& s : my_wksp_data.node_styles )
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

	my_wksp_data.node_styles.emplace_back(std::make_pair<>(name, style));

	return ErrNONE;
}


int
ImGuiWorkspace::AddPinStyle(
	const char* name,
	std::shared_ptr<trezanik::imgui::PinStyle> style
)
{
	using namespace trezanik::core;

	for ( auto& s : my_wksp_data.pin_styles )
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

	my_wksp_data.pin_styles.emplace_back(std::make_pair<>(name, style));

	return ErrNONE;
}


template<>
int
ImGuiWorkspace::AddPropertyRow<ImVec2>(
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
ImGuiWorkspace::AddPropertyRow<PinPosition>(
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
ImGuiWorkspace::AddPropertyRow<trezanik::core::UUID>(
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
ImGuiWorkspace::AddPropertyRow<std::string>(
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
				for ( auto& e : my_wksp_data.node_styles )
				{
					style_loop(e);
				}
			}
			else
			{
				for ( auto& e : my_wksp_data.pin_styles )
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


void
ImGuiWorkspace::AssignDockClient(
	const char* menu_name,
	WindowLocation dock,
	client_draw_function bind_func,
	const trezanik::core::UUID& client_id
)
{
	using namespace trezanik::core;

	auto  dc = std::make_shared<DrawClient>();

	dc->func = bind_func;
	dc->dock = dock;
	dc->menu_name = menu_name;

	/*
	 * Important: log/vkbd/rss are not workspace-based, and they have config
	 * set in the application file, not the workspace one.
	 * We will not monitor nor save their changes in this context.
	 */
	if ( client_id == blank_uuid )
	{
		/*
		 * Blank ID items can have standard manipulation while the application
		 * is running, but will NOT be able to save any associated settings,
		 * since we associate settings to their IDs on load+save
		 */
		TZK_LOG_FORMAT(LogLevel::Warning, "Blank UUID supplied unexpectedly: %s", menu_name);
		dc->id.Generate();
		dc->name = my_workspace->Name() + "|Unnamed";	
	}
	else
	{
		dc->id = client_id;
		dc->name = my_workspace->Name() + "|" + menu_name;
	}

	my_draw_clients.push_back(dc);

	switch ( dc->dock )
	{
	case WindowLocation::Bottom: _gui_interactions.dock_bottom->AddDrawClient(dc); break;
	case WindowLocation::Left:   _gui_interactions.dock_left->AddDrawClient(dc); break;
	case WindowLocation::Right:  _gui_interactions.dock_right->AddDrawClient(dc); break;
	case WindowLocation::Top:    _gui_interactions.dock_top->AddDrawClient(dc); break;
	default:
		break;
	}
}


void
ImGuiWorkspace::BreakLink(
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
			RemoveLink(srcpin->GetID(), tgtpin->GetID());
			my_nodegraph.RemoveLink(l);
			// whichever is 'this' pin must be removed last, breaks iterator
			if ( pin_is_src )
			{
				tgtpin->RemoveLink(l);
				srcpin->RemoveLink(l);
			}
			else
			{
				srcpin->RemoveLink(l);
				tgtpin->RemoveLink(l);
			}
			return;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Failed to find link for pin %s",
		pin->GetID().GetCanonical()
	);
}


void
ImGuiWorkspace::BreakLink(
	const trezanik::core::UUID& id
)
{
	using namespace trezanik::core;

	for ( auto& ngl : my_nodegraph.GetLinks() )
	{
		if ( ngl->GetID() == id )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Breaking link %s", ngl->GetID().GetCanonical());

			// relay to the datastore
			RemoveLink(ngl->Source()->GetID(), ngl->Target()->GetID());
			// remove from pins
			ngl->Source()->RemoveLink(ngl);
			ngl->Target()->RemoveLink(ngl);
			// and the nodegraph - must return, iterator broken
			my_nodegraph.RemoveLink(ngl);
			return;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Failed to find link with id %s", id.GetCanonical());
}


ImVec2
ImGuiWorkspace::ContextCalcNodePinPosition()
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
ImGuiWorkspace::ContextPopup(
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
			// gah
			auto  snode = dynamic_cast<SystemNode*>(my_context_node);
			if ( snode != nullptr )
			{
				close_popup = DrawContextPopupNodeSelect(snode);
			}
			else
			{
				auto  msnode = dynamic_cast<MultiSystemNode*>(my_context_node);
				if ( msnode != nullptr )
				{
					close_popup = DrawContextPopupNodeSelect(msnode);
				}
				else
				{
					auto  bnode = dynamic_cast<BoundaryNode*>(my_context_node);
					if ( bnode != nullptr )
					{
						close_popup = DrawContextPopupNodeSelect(bnode);
					}
				}
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
			auto  snode = dynamic_cast<SystemNode*>(my_context_node);
			if ( snode != nullptr )
			{
				close_popup = DrawContextPopupNodeSelect(snode);
			}
			else
			{
				auto  msnode = dynamic_cast<MultiSystemNode*>(my_context_node);
				if ( msnode != nullptr )
				{
					close_popup = DrawContextPopupNodeSelect(msnode);
				}
				else
				{
					auto  bnode = dynamic_cast<BoundaryNode*>(my_context_node);
					if ( bnode != nullptr )
					{
						close_popup = DrawContextPopupNodeSelect(bnode);
					}
				}
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
ImGuiWorkspace::CreateLink(
	std::shared_ptr<trezanik::imgui::Pin> source,
	std::shared_ptr<trezanik::imgui::Pin> target
)
{
	trezanik::core::UUID  uuid;
	uuid.Generate();

	auto  link = std::make_shared<app::link>(
		uuid, source->GetID(), target->GetID()
	);

	my_wksp_data.links.insert(link);

	auto  retval = std::make_shared<imgui::Link>(
		link->id, source, target,
		&my_nodegraph,
		&link->text, &link->offset
	);
	return retval;
}


void
ImGuiWorkspace::Draw()
{
	using namespace trezanik::core;

	if ( my_workspace == nullptr )
	{
		return;
	}

	if ( _gui_interactions.active_workspace == my_workspace->GetID() )
	{
		// we're the active workspace

		if ( _gui_interactions.save_current_workspace )
		{
			/* add in future, if desired - don't re-write unless modified?
			if ( my_gui.workspaces[my_gui.active_workspace].second->IsModified() )
			{
				// handle
			}*/

			/*
			 * Update the settings that tie into the nodegraph and/or have not
			 * been dynamically updated; workspace data such as nodes, services
			 * and styles are tracked already, but settings are not.
			 * 
			 * There's no direct access into dependent project, so it's either
			 * link into another notification/event routine, or just acquire
			 * everything fresh now. Opting for this as easier (it's also 23:43
			 * and I've been drinking), but will be better to have a proper
			 * location - not to mention handling failure detection.
			 * 
			 * WindowLocation handling via dedicated method as it's already
			 * routed through AppImGui for the application draw clients
			 */
			my_wksp_data.settings[settingname_grid_colour_background] = core::TConverter<size_t>::ToString(my_nodegraph.settings.grid_style.colours.background);
			my_wksp_data.settings[settingname_grid_colour_origin] = core::TConverter<size_t>::ToString(my_nodegraph.settings.grid_style.colours.origins);
			my_wksp_data.settings[settingname_grid_colour_primary] = core::TConverter<size_t>::ToString(my_nodegraph.settings.grid_style.colours.primary);
			my_wksp_data.settings[settingname_grid_colour_secondary] = core::TConverter<size_t>::ToString(my_nodegraph.settings.grid_style.colours.secondary);
			my_wksp_data.settings[settingname_grid_draw] = core::TConverter<bool>::ToString(my_nodegraph.settings.grid_style.draw);
			my_wksp_data.settings[settingname_grid_draworigin] = core::TConverter<bool>::ToString(my_nodegraph.settings.grid_style.draw_origin);
			my_wksp_data.settings[settingname_grid_size] = core::TConverter<size_t>::ToString(my_nodegraph.settings.grid_style.size);
			my_wksp_data.settings[settingname_grid_subdivisions] = core::TConverter<size_t>::ToString(my_nodegraph.settings.grid_style.subdivisions);
			my_wksp_data.settings[settingname_node_drawheaders] = core::TConverter<bool>::ToString(my_nodegraph.settings.node_draw_headers);
			my_wksp_data.settings[settingname_node_dragfromheadersonly] = core::TConverter<bool>::ToString(my_nodegraph.settings.node_drag_from_headers_only);

			int  rc;
			if ( (rc = my_workspace->Save(my_workspace->GetPath(), &my_wksp_data)) != ErrNONE )
			{
				// error already notified
			}
			_gui_interactions.save_current_workspace = false;
		}
#if 0  // Code Disabled: Implemented in AppImGui::PostEnd to get around mutex lock
		if ( _gui_interactions.close_current_workspace )
		{
			/*
			 * This will crash if we attempt this here as the mutex is already
			 * locked. Either figure out a way to resolve (desired, for
			 * locality and user prompting), or achieve the same in AppImGui
			 */
		}
#endif
	}


	if ( my_title.empty() )
	{
		my_title = "Workspace: " + my_wksp_data.name;
		/*
		 * fix @bug 45 - every time the workspace name is changed, it results
		 * in a new window id and imgui interprets it as a 'new' window, and
		 * brings it to focus.
		 * Add the workspace id, which doesn't ever change, alongside the '###'
		 * operator to disassociate (see dear imgui #251)
		 */
		my_title += "###";
		my_title += my_workspace->GetID().GetCanonical();
	}

	ImGuiWindowFlags  wnd_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
	ImVec2  min_size(360.f, 240.f);
	
	//ImGuiWindowFlags_UnsavedDocument

	ImGui::SetNextWindowPos(_gui_interactions.workspace_pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(_gui_interactions.workspace_size, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(min_size, ImVec2(FLT_MAX, FLT_MAX));

	if ( ImGui::Begin(my_title.c_str(), nullptr, wnd_flags) )
	{
		ImGuiWindowFlags  window_flags = ImGuiWindowFlags_None;

		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
		ImGui::BeginChild("ChildR", ImVec2(0, 0), ImGuiChildFlags_Border, window_flags);

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
		 * I hate this here!
		 * 
		 * So, since Links don't have a derived type implementation within this
		 * project, we use them 'raw'; and they have the ability to delete by a
		 * key press (per implementation following).
		 * The context routine has access to this workspace directly, but the
		 * key routine doesn't (it's in the imgui dependency).
		 * 
		 * There's numerous ways of fixing this, however I am simply looping the
		 * links here and handling the operation for minimal code changes in the
		 * interim.
		 * I do want event processing that could pick this up, but it will be
		 * inconsistent to add now, and a derived type akin to the Pins probably
		 * makes the most sense long-term. Could do both, but I found this in
		 * the code review right before the alpha release and trying to keep
		 * fundamental modifications to a minimum!
		 */
		for ( auto& l : my_nodegraph.GetLinks() )
		{
			// due to iterator invalidation, can't multi-select and delete all yet!
			if ( l->IsSelected() && !ImGui::IsAnyItemActive() && ImGui::IsKeyPressed(ImGuiKey_Delete, false) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Deleting Link for Pins %s<->%s",
					l->Source()->GetID().GetCanonical(), l->Target()->GetID().GetCanonical()
				);

				// breaks iterator
				BreakLink(l->GetID());
				break;
			}
		}

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

		ImGui::EndChild(); // ChildR
		ImGui::PopStyleVar();
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
			ImGui::End();
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
			inode->AddServerPin(
				pinpos, style,
				nullptr, my_selector_service,
				my_context_node, &my_nodegraph
			);
		}
		else if ( my_selector_service_group != nullptr )
		{
			inode->AddServerPin(
				pinpos,
				GetPinStyle(reserved_style_service_group.c_str()),
				my_selector_service_group, nullptr,
				my_context_node, &my_nodegraph
			);
		}
		else
		{
			TZK_LOG(LogLevel::Warning, "No selected service or service group after a modal confirmation");
		}

		my_context_node = nullptr;
	}
	if ( my_open_hardware_popup )
	{
		ImGui::OpenPopup(popupname_hardware);
		my_open_hardware_popup = false;
		my_draw_hardware_popup = true;
	}
	if ( my_draw_hardware_popup )
	{
		// micro-optimization possible
		SystemNode* sysnode = dynamic_cast<SystemNode*>(my_context_node == nullptr ? my_selected_nodes[0].get() : my_context_node);
		if ( sysnode != nullptr )
		{
			// content region of workspace, not the full application client area!
			ImVec2  cr = ImGui::GetContentRegionAvail() * 0.75f;
			ImGui::SetNextWindowSize(cr, ImGuiCond_Appearing);
			DrawHardwareDialog(sysnode);
		}
		else
		{
			my_draw_hardware_popup = false;
			ImGui::ClosePopupToLevel(0, false);
		}
	}
	
	if ( _gui_interactions.show_service_management )
	{
		DrawServiceManagement();
	}

	ImGui::End();
}


bool
ImGuiWorkspace::DrawContextPopupLinkSelect(
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
ImGuiWorkspace::DrawContextPopupMultiSelect(
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
ImGuiWorkspace::DrawContextPopupNoSelect()
{
	using namespace trezanik::core;

	ImGui::Text("Graph Context Menu");
	ImGui::Separator();

	// little lambda to assign all the base starting values
	auto newnode_base = [this](graph_node* gn) {
		gn->position = my_context_cursor_pos;
		gn->size.x = TZK_WINDOW_DEFAULT_NEWNODE_WIDTH;
		gn->size.y = TZK_WINDOW_DEFAULT_NEWNODE_HEIGHT;
		gn->size_is_static = true;
		gn->id.Generate();
	};
	
	if ( ImGui::Button("New System Node") )
	{
		TZK_LOG(LogLevel::Debug, "Creating new system node");

		auto  gn = std::make_shared<graph_node_system>();

		newnode_base(gn.get());
		// name assignment isn't required, but it looks much better
		gn->name = "New Node";

		AddNode(gn);

		return true;
	}
	if ( ImGui::Button("New Multi-System Node") )
	{
		TZK_LOG(LogLevel::Debug, "Creating new multi-system node");

		auto  gn = std::make_shared<graph_node_multisystem>();

		newnode_base(gn.get());
		gn->name = "New Multi-System Node";

		AddNode(gn);

		return true;
	}
	if ( ImGui::Button("New Boundary") )
	{
		TZK_LOG(LogLevel::Debug, "Creating new boundary");

		auto  gn = std::make_shared<graph_node_boundary>();

		newnode_base(gn.get());
		gn->name = "Boundary";

		AddNode(gn);

		return true;
	}

	return false;
}


bool
ImGuiWorkspace::DrawContextPopupNodeSelect(
	BoundaryNode* node
)
{
	using namespace trezanik::core;

	bool  retval = false;

	ImGui::Text("Boundary Node: %s", node->GetName()->c_str());
	ImGui::Separator();

	if ( ImGui::BeginMenu("Set Style") )
	{
		for ( auto& style : my_wksp_data.node_styles )
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

				// no need to cover EndDisabled, should be unselectable
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

	float  button_width = ImGui::GetContentRegionAvail().x;

	ImGui::SetNextWindowSize(ImVec2(button_width, 0));
	if ( ImGui::Button("Delete Node") )
	{
		TZK_LOG(LogLevel::Trace, "Deleting node");
		RemoveNode(node);
		retval = true;
	}

	return retval;
}


bool
ImGuiWorkspace::DrawContextPopupNodeSelect(
	MultiSystemNode* node
)
{
	using namespace trezanik::core;

	bool  retval = false;

	// nice to have its name/identifier here for assurance
	ImGui::Text("Node Context Menu");
	ImGui::Separator();

	
	// duplicate of system node implementation
	if ( ImGui::BeginMenu("Pins") )
	{
		/// @bug 25 - text is cut-off, not wide enough depending on prior
		if ( ImGui::MenuItem("Add Server (inbound)") )
		{
			TZK_LOG(LogLevel::Trace, "Adding server pin");

			my_open_service_selector_popup = true;
			retval = true;
		}
		if ( ImGui::MenuItem("Add Client (outbound)") )
		{
			TZK_LOG(LogLevel::Trace, "Adding client pin");

			// outputs don't have service assignments, obtained from input connections

			node->AddClientPin(
				ContextCalcNodePinPosition(),
				GetPinStyle(reserved_style_client.c_str()),
				my_context_node, &my_nodegraph
			);
			retval = true;
		}
		ImGui::Separator();
		if ( ImGui::MenuItem("Add Generic Connector") )
		{
			TZK_LOG(LogLevel::Trace, "Adding connector pin");

			node->AddConnectorPin(
				ContextCalcNodePinPosition(),
				GetPinStyle(reserved_style_connector.c_str()),
				my_context_node, &my_nodegraph
			);
			retval = true;
		}
		ImGui::EndMenu();
	}

	ImGui::Separator();

	if ( ImGui::BeginMenu("Set Style") )
	{
		for ( auto& style : my_wksp_data.node_styles )
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

	float  button_width = ImGui::GetContentRegionAvail().x;

	ImGui::SetNextWindowSize(ImVec2(button_width, 0));
	if ( ImGui::Button("Delete Node") )
	{
		TZK_LOG(LogLevel::Trace, "Deleting node");
		RemoveNode(node);
		retval = true;
	}

	return retval;
}


bool
ImGuiWorkspace::DrawContextPopupNodeSelect(
	SystemNode* node
)
{
	using namespace trezanik::core;

	bool  retval = false;

	// nice to have its name/identifier here for assurance
	ImGui::Text("Node Context Menu");
	ImGui::Separator();

	if ( ImGui::BeginMenu("Recon") )
	{
		ImGui::MenuItem("nmap");
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

			node->AddClientPin(
				ContextCalcNodePinPosition(),
				GetPinStyle(reserved_style_client.c_str()),
				my_context_node, &my_nodegraph
			);
			retval = true;
		}
		ImGui::Separator();
		if ( ImGui::MenuItem("Add Generic Connector") )
		{
			TZK_LOG(LogLevel::Trace, "Adding connector pin");

			node->AddConnectorPin(
				ContextCalcNodePinPosition(),
				GetPinStyle(reserved_style_connector.c_str()),
				my_context_node, &my_nodegraph
			);
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
		for ( auto& style : my_wksp_data.node_styles )
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

#if 0  // Code Disabled: example and visual usage
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
		RemoveNode(node);
		retval = true;
	}

	return retval;
}


bool
ImGuiWorkspace::DrawContextPopupPinSelect(
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
		for ( auto& style : my_wksp_data.pin_styles )
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
ImGuiWorkspace::DrawHardwareDialog(
	SystemNode* node
)
{
	using namespace trezanik::core;

	if ( node == nullptr || !ImGui::BeginPopupModal(popupname_hardware, &my_draw_hardware_popup) )
	{
		my_draw_hardware_popup = false;
		return;
	}

	ImGuiTreeNodeFlags  all_treeflags = ImGuiTreeNodeFlags_SpanAllColumns | ImGuiTreeNodeFlags_DefaultOpen;
		// | ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_OpenOnArrow;

	auto gns = std::dynamic_pointer_cast<graph_node_system>(node->GetGraphNode());
	ImGui::PushID(this);
	int  idinc = 0;
	bool  disable_elem = false;
	static std::string  label;

	ImGui::Text("Node : %s", node->GetName()->c_str());
	ImGui::Spacing();

	/// @todo hardcoded for now, decide on layout and optimize

	bool  tree_open = false;
	bool  subtree_open = false;
	int   column_count = 2;
	ImGuiTableFlags  table_flags
		= ImGuiTableFlags_BordersV
		| ImGuiTableFlags_BordersOuterH
		| ImGuiTableFlags_Resizable
		| ImGuiTableFlags_RowBg
		| ImGuiTableFlags_NoBordersInBody;
	
	if ( ImGui::BeginTable("Hardware", column_count, table_flags) )
	{
		ImGui::TableSetupColumn("Property##column_prop", ImGuiTableColumnFlags_NoHide);
		ImGui::TableSetupColumn("Value##column_value", ImGuiTableColumnFlags_NoHide);
		ImGui::TableHeadersRow();
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		int  delete_entry_unset = -1;
		int  delete_entry = delete_entry_unset;
		int  delete_subentry = delete_entry_unset;
	
		if ( ImGui::Button("Add##cpu") )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Adding CPU %zu", gns->system_manual.cpus.size() + 1);
			gns->system_manual.cpus.emplace_back();
		}
		ImGui::SameLine();
		label = "CPUs : " + std::to_string(gns->system_manual.cpus.size());
		if ( ImGui::TreeNodeEx(label.c_str(), all_treeflags) )
		{
			idinc = 0;
			ImGui::PushID("cpu");

			for ( auto& e : gns->system_manual.cpus )
			{
				ImGui::PushID(++idinc);
				label = "CPU " + std::to_string(idinc);
			
				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				tree_open = ImGui::TreeNodeEx(label.c_str());
				ImGui::TableNextColumn();
				if ( ImGui::SmallButton("Delete") )
				{
					TZK_LOG_FORMAT(LogLevel::Debug, "Deleting %s", label.c_str());
					delete_entry = idinc - 1; // 0-based index
				}
				if ( tree_open )
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Vendor");
					ImGui::TableNextColumn();
					ImGui::InputText("##Vendor", &e.vendor);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Model");
					ImGui::TableNextColumn();
					ImGui::InputText("##Model", &e.model);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Serial");
					ImGui::TableNextColumn();
					ImGui::InputText("##Serial", &e.serial);

					ImGui::TreePop();
				}

				ImGui::PopID();
			}

			if ( delete_entry != delete_entry_unset )
			{
				gns->system_manual.cpus.erase(gns->system_manual.cpus.begin() + delete_entry);
				delete_entry = delete_entry_unset;
			}

			ImGui::PopID();
			ImGui::TreePop();
		}
	
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		

		if ( ImGui::Button("Add##dimm") )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Adding DIMM %zu", gns->system_manual.dimms.size() + 1);
			gns->system_manual.dimms.emplace_back();
		}
		ImGui::SameLine();
		label = "RAM : " + std::to_string(gns->system_manual.dimms.size()) + " DIMMs";
		if ( ImGui::TreeNodeEx(label.c_str(), all_treeflags) )
		{
			idinc = 0;
			ImGui::PushID("ram");

			for ( auto& e : gns->system_manual.dimms )
			{
				ImGui::PushID(++idinc);
				label = "DIMM " + std::to_string(idinc);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				tree_open = ImGui::TreeNodeEx(label.c_str());
				ImGui::TableNextColumn();
				if ( ImGui::SmallButton("Delete") )
				{
					TZK_LOG_FORMAT(LogLevel::Debug, "Deleting %s", label.c_str());
					delete_entry = idinc - 1;
				}
				if ( tree_open )
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Vendor");
					ImGui::TableNextColumn();
					ImGui::InputText("##Vendor", &e.vendor);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Model");
					ImGui::TableNextColumn();
					ImGui::InputText("##Model", &e.model);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Serial");
					ImGui::TableNextColumn();
					ImGui::InputText("##Serial", &e.serial);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Capacity");
					ImGui::TableNextColumn();
					ImGui::InputText("##Capacity", &e.capacity);

					ImGui::TreePop();
				}
	
				ImGui::PopID();
			}

			if ( delete_entry != delete_entry_unset )
			{
				gns->system_manual.dimms.erase(gns->system_manual.dimms.begin() + delete_entry);
				delete_entry = delete_entry_unset;
			}

			ImGui::PopID();
			ImGui::TreePop();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if ( ImGui::Button("Add##disk") )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Adding Disk %zu", gns->system_manual.disks.size() + 1);
			gns->system_manual.disks.emplace_back();
		}
		ImGui::SameLine();
		label = "Disks : " + std::to_string(gns->system_manual.disks.size());
		if ( ImGui::TreeNodeEx(label.c_str(), all_treeflags) )
		{
			idinc = 0;
			ImGui::PushID("disk");

			for ( auto& e : gns->system_manual.disks )
			{
				ImGui::PushID(++idinc);
				label = "Disk " + std::to_string(idinc);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				tree_open = ImGui::TreeNodeEx(label.c_str());
				ImGui::TableNextColumn();
				if ( ImGui::SmallButton("Delete") )
				{
					TZK_LOG_FORMAT(LogLevel::Debug, "Deleting %s", label.c_str());
					delete_entry = idinc - 1;
				}
				if ( tree_open )
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Vendor");
					ImGui::TableNextColumn();
					ImGui::InputText("##Vendor", &e.vendor);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Model");
					ImGui::TableNextColumn();
					ImGui::InputText("##Model", &e.model);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Serial");
					ImGui::TableNextColumn();
					ImGui::InputText("##Serial", &e.serial);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Capacity");
					ImGui::TableNextColumn();
					ImGui::InputText("##Capacity", &e.capacity);

					ImGui::TreePop();
				}

				ImGui::PopID();
			}

			if ( delete_entry != delete_entry_unset )
			{
				gns->system_manual.disks.erase(gns->system_manual.disks.begin() + delete_entry);
				delete_entry = delete_entry_unset;
			}

			ImGui::PopID();
			ImGui::TreePop();
		}
	
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
	
		if ( ImGui::Button("Add##gpu") )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Adding GPU %zu", gns->system_manual.gpus.size() + 1);
			gns->system_manual.gpus.emplace_back();
		}
		ImGui::SameLine();
		label = "GPUs : " + std::to_string(gns->system_manual.gpus.size());
		if ( ImGui::TreeNodeEx(label.c_str(), all_treeflags) )
		{
			idinc = 0;
			ImGui::PushID("gpu");

			for ( auto& e : gns->system_manual.gpus )
			{
				ImGui::PushID(++idinc);
				label = "GPU " + std::to_string(idinc);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				tree_open = ImGui::TreeNodeEx(label.c_str());
				ImGui::TableNextColumn();
				if ( ImGui::SmallButton("Delete") )
				{
					TZK_LOG_FORMAT(LogLevel::Debug, "Deleting %s", label.c_str());
					delete_entry = idinc - 1;
				}
				if ( tree_open )
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Vendor");
					ImGui::TableNextColumn();
					ImGui::InputText("##Vendor", &e.vendor);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Model");
					ImGui::TableNextColumn();
					ImGui::InputText("##Model", &e.model);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Serial");
					ImGui::TableNextColumn();
					ImGui::InputText("##Serial", &e.serial);

					ImGui::TreePop();
				}

				ImGui::PopID();
			}

			if ( delete_entry != delete_entry_unset )
			{
				gns->system_manual.gpus.erase(gns->system_manual.gpus.begin() + delete_entry);
				delete_entry = delete_entry_unset;
			}

			ImGui::PopID();
			ImGui::TreePop();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if ( !gns->system_manual.mobo.empty() )
		{
			disable_elem = true;
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("Add##mobo") )
		{
			TZK_LOG(LogLevel::Debug, "Adding Motherboard");
			gns->system_manual.mobo.emplace_back();
		}
		if ( disable_elem )
		{
			ImGui::EndDisabled();
			disable_elem = false;
		}
		ImGui::SameLine();
		label = "Motherboard : " + std::to_string(gns->system_manual.mobo.size());
		// maximum 1 instance
		if ( gns->system_manual.mobo.empty() )
		{
			ImGui::Text("%s", label.c_str());
		}
		else if ( ImGui::TreeNodeEx(label.c_str(), all_treeflags) )
		{
			idinc = 0;
			ImGui::PushID("mobo");

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			label = "Motherboard##elem";
			tree_open = ImGui::TreeNodeEx(label.c_str());
			ImGui::TableNextColumn();
			if ( ImGui::SmallButton("Delete") )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Deleting %s", label.c_str());
				gns->system_manual.mobo.clear();
			}
			if ( tree_open )
			{
				for ( auto& e : gns->system_manual.mobo )
				{
					ImGui::PushID(++idinc);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Vendor");
					ImGui::TableNextColumn();
					ImGui::InputText("##Vendor", &e.vendor);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Model");
					ImGui::TableNextColumn();
					ImGui::InputText("##Model", &e.model);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Serial");
					ImGui::TableNextColumn();
					ImGui::InputText("##Serial", &e.serial);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("BIOS");
					ImGui::TableNextColumn();
					ImGui::InputText("##BIOS", &e.bios);

					ImGui::PopID();
				}

				ImGui::TreePop();
			}

			ImGui::PopID();
			ImGui::TreePop();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if ( ImGui::Button("Add##psu") )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Adding PSU %zu", gns->system_manual.psus.size() + 1);
			gns->system_manual.psus.emplace_back();
		}
		ImGui::SameLine();
		label = "PSUs : " + std::to_string(gns->system_manual.psus.size());
		if ( ImGui::TreeNodeEx(label.c_str(), all_treeflags) )
		{
			idinc = 0;
			ImGui::PushID("psu");
			
			for ( auto& e : gns->system_manual.psus )
			{
				ImGui::PushID(++idinc);
				label = "PSU " + std::to_string(idinc);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				tree_open = ImGui::TreeNodeEx(label.c_str());
				ImGui::TableNextColumn();
				if ( ImGui::SmallButton("Delete") )
				{
					TZK_LOG_FORMAT(LogLevel::Debug, "Deleting %s", label.c_str());
					delete_entry = idinc - 1;
				}
				if ( tree_open )
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Vendor");
					ImGui::TableNextColumn();
					ImGui::InputText("##Vendor", &e.vendor);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Model");
					ImGui::TableNextColumn();
					ImGui::InputText("##Model", &e.model);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Serial");
					ImGui::TableNextColumn();
					ImGui::InputText("##Serial", &e.serial);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Wattage");
					ImGui::TableNextColumn();
					ImGui::InputText("##Wattage", &e.wattage);

					ImGui::TreePop();
				}
		
				ImGui::PopID();
			}

			if ( delete_entry != delete_entry_unset )
			{
				gns->system_manual.psus.erase(gns->system_manual.psus.begin() + delete_entry);
				delete_entry = delete_entry_unset;
			}
		
			ImGui::PopID();
			ImGui::TreePop();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if ( ImGui::Button("Add##nic") )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Adding NIC %zu", gns->system_manual.interfaces.size() + 1);
			gns->system_manual.interfaces.emplace_back();
		}
		ImGui::SameLine();
		label = "Network Interfaces : " + std::to_string(gns->system_manual.interfaces.size());
		if ( ImGui::TreeNodeEx(label.c_str(), all_treeflags) )
		{
			idinc = 0;
			ImGui::PushID("nic");
			
			for ( auto& e : gns->system_manual.interfaces )
			{
				ImGui::PushID(++idinc);
				label = "NIC " + std::to_string(idinc);

				ImGui::TableNextRow();
				ImGui::TableNextColumn();

				tree_open = ImGui::TreeNodeEx(label.c_str());
				ImGui::TableNextColumn();
				if ( ImGui::SmallButton("Delete") )
				{
					TZK_LOG_FORMAT(LogLevel::Debug, "Deleting %s", label.c_str());
					delete_entry = idinc - 1;
				}
				if ( tree_open )
				{
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Alias");
					ImGui::TableNextColumn();
					ImGui::InputText("##Alias", &e.alias);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Model");
					ImGui::TableNextColumn();
					ImGui::InputText("##Model", &e.model);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("MAC Address");
					ImGui::TableNextColumn();
					if ( ImGui::InputText("##MACAddress", &e.mac) )
					{
						trezanik::core::aux::mac_address  macaddr;
						if ( trezanik::core::aux::string_to_macaddr(e.mac.c_str(), macaddr) == 1 )
						{
							e.valid_mac = true;
						}
						else
						{
							e.valid_mac = false;
						}
					}
					if ( !e.valid_mac )
					{
						ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
					}

					int  secid = 0;

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					if ( ImGui::Button("Add##nameserver") )
					{
						TZK_LOG_FORMAT(LogLevel::Debug, "Adding nameserver %zu for interface %i", e.nameservers.size() + 1, idinc);
						e.nameservers.emplace_back();
					}
					ImGui::SameLine();
					label = "Nameservers : " + std::to_string(e.nameservers.size());
					if ( ImGui::TreeNodeEx(label.c_str(), all_treeflags) )
					{
						for ( auto& ens : e.nameservers )
						{
							ImGui::PushID(++secid);
							label = "Nameserver " + std::to_string(secid);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();

							subtree_open = ImGui::TreeNodeEx(label.c_str());
							ImGui::TableNextColumn();
							if ( ImGui::SmallButton("Delete") )
							{
								TZK_LOG_FORMAT(LogLevel::Debug, "Deleting %s", label.c_str());
								delete_subentry = secid - 1;
							}
							if ( subtree_open )
							{
								// would be great to update label based on IPv4/IPv6
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::TextUnformatted("IP Address");
								ImGui::TableNextColumn();
								if ( ImGui::InputText("##IPAddr", &ens.nameserver) )
								{
									trezanik::core::aux::ip_address  ipaddr;
									if ( trezanik::core::aux::string_to_ipaddr(ens.nameserver.c_str(), ipaddr) > 0 )
									{
										ens.valid_nameserver = true;
									}
									else
									{
										ens.valid_nameserver = false;
									}
								}
								if ( !ens.valid_nameserver )
								{
									ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
								}

								ImGui::TreePop();
							}

							ImGui::PopID();
						}

						if ( delete_subentry != delete_entry_unset )
						{
							e.nameservers.erase(e.nameservers.begin() + delete_subentry);
							delete_subentry = delete_entry_unset;
						}

						ImGui::TreePop();
					}

					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					secid = 0;
					if ( ImGui::Button("Add##address") )
					{
						TZK_LOG_FORMAT(LogLevel::Debug, "Adding address %zu for interface %zu", e.addresses.size() + 1, idinc);
						e.addresses.emplace_back();
					}
					ImGui::SameLine();
					label = "Addresses : " + std::to_string(e.addresses.size());
					if ( ImGui::TreeNodeEx(label.c_str(), all_treeflags) )
					{
						for ( auto& eaddr : e.addresses )
						{
							ImGui::PushID(++secid);
							label = "Address " + std::to_string(secid);

							ImGui::TableNextRow();
							ImGui::TableNextColumn();

							subtree_open = ImGui::TreeNodeEx(label.c_str());
							ImGui::TableNextColumn();
							if ( ImGui::SmallButton("Delete") )
							{
								TZK_LOG_FORMAT(LogLevel::Debug, "Deleting %s", label.c_str());
								delete_subentry = secid - 1;
							}
							if ( subtree_open )
							{
								// would be great to update label based on IPv4/IPv6
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::TextUnformatted("IP Address");
								ImGui::TableNextColumn();
								if ( ImGui::InputText("##IPAddr", &eaddr.address) )
								{
									trezanik::core::aux::ip_address  ipaddr;
									if ( trezanik::core::aux::string_to_ipaddr(eaddr.address.c_str(), ipaddr) > 0 )
									{
										eaddr.valid_address = true;
									}
									else
									{
										eaddr.valid_address = false;
									}
								}
								if ( !eaddr.valid_address )
								{
									ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
								}

								// if IPv6, prefixlen
								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::TextUnformatted("Subnet Mask");
								ImGui::TableNextColumn();
								if ( ImGui::InputText("##SubnetMask", &eaddr.mask) )
								{
									trezanik::core::aux::ip_address  ipaddr;
									if ( trezanik::core::aux::string_to_ipaddr(eaddr.mask.c_str(), ipaddr) > 0 )
									{
										eaddr.valid_mask = true;
									}
									else
									{
										eaddr.valid_mask = false;
									}
								}
								if ( !eaddr.valid_mask )
								{
									ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
								}

								ImGui::TableNextRow();
								ImGui::TableNextColumn();
								ImGui::TextUnformatted("Gateway");
								ImGui::TableNextColumn();
								if ( ImGui::InputText("##Gateway", &eaddr.gateway) )
								{
									trezanik::core::aux::ip_address  ipaddr;
									if ( trezanik::core::aux::string_to_ipaddr(eaddr.gateway.c_str(), ipaddr) > 0 )
									{
										eaddr.valid_gateway = true;
									}
									else
									{
										eaddr.valid_gateway = false;
									}
								}
								if ( !eaddr.valid_gateway )
								{
									ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
								}

								ImGui::TreePop();
							}

							ImGui::PopID();
						}

						if ( delete_subentry != delete_entry_unset )
						{
							e.addresses.erase(e.addresses.begin() + delete_subentry);
							delete_subentry = delete_entry_unset;
						}

						ImGui::TreePop();
					}

					ImGui::TreePop();
				}

				if ( delete_entry != delete_entry_unset )
				{
					gns->system_manual.interfaces.erase(gns->system_manual.interfaces.begin() + delete_entry);
					delete_entry = delete_entry_unset;
				}

				ImGui::PopID();
			}

			ImGui::PopID();
			ImGui::TreePop();
		}

		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if ( !gns->system_manual.os.empty() )
		{
			disable_elem = true;
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("Add##os") )
		{
			TZK_LOG(LogLevel::Debug, "Adding Operating System");
			gns->system_manual.os.emplace_back();
		}
		if ( disable_elem )
		{
			ImGui::EndDisabled();
			disable_elem = false;
		}
		ImGui::SameLine();
		label = "Operating System : " + std::to_string(gns->system_manual.os.size());

		// maximum 1 instance
		if ( gns->system_manual.os.empty() )
		{
			ImGui::Text("%s", label.c_str());
		}
		else if ( ImGui::TreeNodeEx(label.c_str(), all_treeflags) )
		{
			idinc = 0;
			ImGui::PushID("os");

			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			label = "Operating System##elem";
			tree_open = ImGui::TreeNodeEx(label.c_str());
			ImGui::TableNextColumn();
			if ( ImGui::SmallButton("Delete") )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Deleting %s", label.c_str());
				gns->system_manual.os.clear();
			}
			if ( tree_open )
			{
				for ( auto& e : gns->system_manual.os )
				{
					ImGui::PushID(++idinc);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Architecture");
					ImGui::TableNextColumn();
					ImGui::InputText("##Arch", &e.arch);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Kernel");
					ImGui::TableNextColumn();
					ImGui::InputText("##Kernel", &e.kernel);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Version");
					ImGui::TableNextColumn();
					ImGui::InputText("##Version", &e.version);
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("Name");
					ImGui::TableNextColumn();
					ImGui::InputText("##Name", &e.name);

					ImGui::PopID();
				}

				ImGui::TreePop();

			}

			ImGui::PopID();
			ImGui::TreePop();
		}

		ImGui::EndTable();
	}

	ImGui::PopID();
	

	if ( ImGui::Button("Close") )
	{
		my_draw_hardware_popup = false;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
}


void
ImGuiWorkspace::DrawPropertyView()
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

			// if modified, my_name.clear(); - each frame will rebuild if empty
			if ( AddPropertyRow(PropertyRowType::TextInput, "Name", &my_wksp_data.name) )
			{
				my_title.clear();
			}
			AddSeparatorRow("Configuration");
			row_count += 2;


			// disable elements that are not yet integrated
			ImGui::TableNextColumn();
			ImGui::Text("Draw Headers");
			ImGui::TableNextColumn();
		ImGui::BeginDisabled();
			if ( ImGui::ToggleButton("##DrawHeaders", &my_nodegraph.settings.node_draw_headers) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Workspace.DrawHeader = %s", core::TConverter<bool>::ToString(my_nodegraph.settings.node_draw_headers).c_str());
			}
		ImGui::EndDisabled();
			ImGui::SameLine();
			ImGui::HelpMarker("Toggle for each Node drawing its header\n"
				"- Does not apply to BoundaryNodes, which must always have their headers shown"
		"- Disabled : not yet implemented"
			);
			
			ImGui::TableNextColumn();
			ImGui::Text("Drag From Header Only");
			ImGui::TableNextColumn();
			if ( ImGui::ToggleButton("##DragFromHeaderOnly", &my_nodegraph.settings.node_drag_from_headers_only) )
			{
				TZK_LOG_FORMAT(LogLevel::Trace, "Workspace.DragFromHeaderOnly = %s", core::TConverter<bool>::ToString(my_nodegraph.settings.node_drag_from_headers_only).c_str());
			}
			ImGui::SameLine();
			ImGui::HelpMarker("Toggle for nodes being movable from any free space, or only their header\n"
				"- Does not apply to BoundaryNodes, which always drag from their headers only"
			);
			row_count += 2;

			

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
				ImGui::PushFont(_gui_interactions.font_fixed_width);
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
				ImGui::Text("%s", l->Source()->GetAttachedNode()->GetName()->c_str());

				ImGui::TableNextColumn();
				ImGui::Text("Target Node");
				ImGui::TableNextColumn();
				ImGui::Text("%s", l->Target()->GetAttachedNode()->GetName()->c_str());

				row_count += 7;

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

				/*
				 * shouldn't really be doing this per frame, is it worth maintaining
				 * pointers instead of accessing referencing?
				 * could also cache and detect selected node changing
				 * 
				 * also note as per workspace comments, we want to adjust this to a
				 * component-based lookup
				 */
				if ( node->Typename() == typename_system )
				{
					auto  sgn = std::dynamic_pointer_cast<graph_node_system>(node->GetGraphNode());

					assert(sgn != nullptr);

					/*
					 * I really don't want to make this an incrementable that *every* one
					 * needs to invoke, but I'm also aligned with hardcoded counts being
					 * highly error-prone and ugly.
					 * We'll do it for now, hopefully imgui will have a table sizing fix
					 * by the time we need to revisit.
					 */
					row_count += 6; // yes, counted by hand

					AddPropertyRow(PropertyRowType::TextReadOnly, "ID", &sgn->id);
					if ( AddPropertyRow(PropertyRowType::TextInput, "Name", &sgn->name) )
					{
						node->NotifyListeners(imgui::NodeUpdate::Name);
					}
					AddPropertyRow(PropertyRowType::TextReadOnly, "Type", (std::string*)&node->Typename());
					if ( AddPropertyRow(PropertyRowType::FloatInput, "Position", &sgn->position) )
					{
						node->SetPosition(sgn->position);
						node->NotifyListeners(imgui::NodeUpdate::Position);
					}
					if ( AddPropertyRow(PropertyRowType::FloatInput, "Size", &sgn->size) )
					{
						ImVec2  requested_size = sgn->size;

						if ( requested_size.x < imgui::node_minimum_width )
						{
							requested_size.x = imgui::node_minimum_width;
						}
						if ( requested_size.y < imgui::node_minimum_height )
						{
							requested_size.y = imgui::node_minimum_height;
						}

						node->SetStaticSize(requested_size);
						node->NotifyListeners(imgui::NodeUpdate::Size);
					}
					if ( AddPropertyRow(PropertyRowType::NodeStyle, "Style", &sgn->style) )
					{
						TZK_LOG_FORMAT(LogLevel::Trace, "Setting new node style: %s", sgn->style.c_str());
						node->SetStyle(GetNodeStyle(sgn->style.c_str()));
						node->NotifyListeners(imgui::NodeUpdate::Style);
					}
					if ( AddPropertyRow(PropertyRowType::TextMultilineInput, "Data", &sgn->datastr) )
					{
						node->NotifyListeners(imgui::NodeUpdate::Data);
					}
	
					DrawPropertyView_Pins(row_count, sgn->pins, node);
				}
				else if ( node->Typename() == typename_multisys )
				{
					auto  mgn = std::dynamic_pointer_cast<graph_node_multisystem>(node->GetGraphNode());

					assert(mgn != nullptr);

					row_count += 8; // manual count

					AddPropertyRow(PropertyRowType::TextReadOnly, "ID", &mgn->id);
					AddPropertyRow(PropertyRowType::TextInput, "Name", &mgn->name);
					AddPropertyRow(PropertyRowType::TextReadOnly, "Type", (std::string*)&node->Typename());
					AddPropertyRow(PropertyRowType::FloatInput, "Position", &mgn->position);

					// more duplication

					if ( AddPropertyRow(PropertyRowType::FloatInput, "Size", &mgn->size) )
					{
						ImVec2  requested_size = mgn->size;

						if ( requested_size.x < imgui::node_minimum_width )
						{
							requested_size.x = imgui::node_minimum_width;
						}
						if ( requested_size.y < imgui::node_minimum_height )
						{
							requested_size.y = imgui::node_minimum_height;
						}

						node->SetStaticSize(requested_size);
						node->NotifyListeners(imgui::NodeUpdate::Size);
					}
					if ( AddPropertyRow(PropertyRowType::NodeStyle, "Style", &mgn->style) )
					{
						TZK_LOG_FORMAT(LogLevel::Trace, "Setting new node style: %s", mgn->style.c_str());
						node->SetStyle(GetNodeStyle(mgn->style.c_str()));
						node->NotifyListeners(imgui::NodeUpdate::Style);
					}
					if ( AddPropertyRow(PropertyRowType::TextMultilineInput, "Data", &mgn->datastr) )
					{
						node->NotifyListeners(imgui::NodeUpdate::Data);
					}

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

					disp_list(mgn->hostnames, str_newhost, "Hostname", "Delete##Host", "##new_host", "Add Host");
					disp_list(mgn->ips, str_newip, "IP", "Delete##IP", "##new_ip", "Add IP");
					disp_list(mgn->ip_ranges, str_newiprange, "IP Range", "Delete##IPRange", "##new_iprange", "Add IP Range");
					disp_list(mgn->subnets, str_newsubnet, "Subnet", "Delete##Subnet", "##new_subnet", "Add Subnet");

					DrawPropertyView_Pins(row_count, mgn->pins, node);
				}
				else if ( node->Typename() == typename_boundary )
				{
					auto  bgn = std::dynamic_pointer_cast<graph_node_boundary>(node->GetGraphNode());

					assert(bgn != nullptr);

					row_count += 6;

					AddPropertyRow(PropertyRowType::TextReadOnly, "ID", &bgn->id);
					if ( AddPropertyRow(PropertyRowType::TextInput, "Name", &bgn->name) )
					{
						node->NotifyListeners(imgui::NodeUpdate::Name);
					}
					AddPropertyRow(PropertyRowType::TextReadOnly, "Type", (std::string*)&node->Typename());
					if ( AddPropertyRow(PropertyRowType::FloatInput, "Position", &bgn->position) )
					{
						node->SetPosition(bgn->position);
						node->NotifyListeners(imgui::NodeUpdate::Position);
					}
					if ( AddPropertyRow(PropertyRowType::FloatInput, "Size", &bgn->size) )
					{
						ImVec2  requested_size = bgn->size;

						if ( requested_size.x < imgui::node_minimum_width )
						{
							requested_size.x = imgui::node_minimum_width;
						}
						if ( requested_size.y < imgui::node_minimum_height )
						{
							requested_size.y = imgui::node_minimum_height;
						}

						node->SetStaticSize(requested_size);
						node->NotifyListeners(imgui::NodeUpdate::Size);
					}
					if ( AddPropertyRow(PropertyRowType::NodeStyle, "Style", &bgn->style) )
					{
						TZK_LOG_FORMAT(LogLevel::Trace, "Setting new node style: %s", bgn->style.c_str());
						node->SetStyle(GetNodeStyle(bgn->style.c_str()));
						node->NotifyListeners(imgui::NodeUpdate::Style);
					}
				}
				else
				{
					//warn
				}
			}

			ImGui::EndTable();
		}

		// correct size for the next frame
		outer_size.y = ImGui::GetFrameHeight() + row_count * (ImGui::GetTextLineHeight() + ImGui::GetStyle().CellPadding.y * 2.0f);

	} // node properties
	// this is only applicable for system nodes; consider suppression
	if ( ImGui::CollapsingHeader("System Information (Manual)", ImGuiTreeNodeFlags_None) )
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

		static bool  hide_empty_fields = true;

		ImGui::Checkbox("Hide empty fields", &hide_empty_fields);

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

				if ( node->Typename() == typename_system )
				{
					/// @todo replace with DrawPropertyView_SystemInformation()
					auto  sgn = std::dynamic_pointer_cast<graph_node_system>(node->GetGraphNode());
					assert(sgn != nullptr);

					// remember, all IdLabels need to be unique. we could push+pop after each row...
					int  labelid = 0;
					/// @todo pass labelid into AddPropRow, have it inc, push and pop within the method
					for ( auto& elem : sgn->system_manual.cpus )
					{
						row_count += 3;

						AddSeparatorRow("CPU");
						ImGui::PushID(++labelid);
						if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, hide_empty_fields) )
						{
						}
						ImGui::PopID();
					}
					for ( auto& elem : sgn->system_manual.dimms )
					{
						row_count += 5;

						AddSeparatorRow("DIMM");
						ImGui::PushID(++labelid);
						if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Capacity", &elem.capacity, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Slot", &elem.slot, hide_empty_fields) )
						{
						}
						ImGui::PopID();
					}
					for ( auto& elem : sgn->system_manual.disks )
					{
						row_count += 4;

						AddSeparatorRow("Disk");
						ImGui::PushID(++labelid);
						if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Capacity", &elem.capacity, hide_empty_fields) )
						{
						}
						ImGui::PopID();
					}
					for ( auto& elem : sgn->system_manual.gpus )
					{
						row_count += 3;

						AddSeparatorRow("GPU");
						ImGui::PushID(++labelid);
						if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, hide_empty_fields) )
						{
						}
						ImGui::PopID();
					}
					for ( auto& elem : sgn->system_manual.psus )
					{
						row_count += 4;

						AddSeparatorRow("PSU");
						ImGui::PushID(++labelid);
						if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Wattage", &elem.wattage, hide_empty_fields) )
						{
						}
						ImGui::PopID();
					}
					for ( auto& elem : sgn->system_manual.mobo )
					{
						row_count += 4;

						AddSeparatorRow("Motherboard");
						ImGui::PushID(++labelid);
						if ( AddPropertyRow(PropertyRowType::TextInput, "Vendor", &elem.vendor, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &elem.model, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Serial", &elem.serial, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "BIOS", &elem.bios, hide_empty_fields) )
						{
						}
						ImGui::PopID();
					}
					for ( auto& elem : sgn->system_manual.os )
					{
						row_count += 4;

						AddSeparatorRow("Operating System");
						ImGui::PushID(++labelid);
						if ( AddPropertyRow(PropertyRowType::TextInput, "Architecture", &elem.arch, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Kernel", &elem.kernel, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Name", &elem.name, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Version", &elem.version, hide_empty_fields) )
						{
						}
						ImGui::PopID();
					}
					for ( auto& intf : sgn->system_manual.interfaces )
					{
						row_count += 3;

						AddSeparatorRow("Interface");

						ImGui::PushID(++labelid);
						if ( AddPropertyRow(PropertyRowType::TextInput, "Alias", &intf.alias, hide_empty_fields) )
						{
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "MAC", &intf.mac, hide_empty_fields) )
						{
							trezanik::core::aux::mac_address  macaddr;
							intf.valid_mac = trezanik::core::aux::string_to_macaddr(intf.mac.c_str(), macaddr) == 1;
						}
						if ( !intf.valid_mac && !(hide_empty_fields && intf.mac.empty()) )
						{
							row_count++;
							ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
						}
						if ( AddPropertyRow(PropertyRowType::TextInput, "Model", &intf.model, hide_empty_fields) )
						{
						}
						
						for ( auto& ia : intf.addresses )
						{
							row_count += 3;

							AddSeparatorRow("Address");
							ImGui::PushID(++labelid);

							if ( AddPropertyRow(PropertyRowType::TextInput, "IPv4", &ia.address, hide_empty_fields) )
							{
								trezanik::core::aux::ip_address  ipaddr;
								ia.valid_address = trezanik::core::aux::string_to_ipaddr(ia.address.c_str(), ipaddr) > 0;
							}
							if ( !ia.valid_address && !(hide_empty_fields && ia.address.empty()) )
							{
								row_count++;
								ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
							}
							/*if ( AddPropertyRow(IdLabel::IPv6, "IPv6", &ia.str_ipv6) )
							{
							}*/
							if ( AddPropertyRow(PropertyRowType::TextInput, "Subnet Mask", &ia.mask, hide_empty_fields) )
							{
								trezanik::core::aux::ip_address  ipaddr;
								ia.valid_mask = trezanik::core::aux::string_to_ipaddr(ia.mask.c_str(), ipaddr) > 0;
							}
							if ( !ia.valid_mask && !(hide_empty_fields && ia.mask.empty()) )
							{
								row_count++;
								ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
							}
							if ( AddPropertyRow(PropertyRowType::TextInput, "Default Gateway", &ia.gateway, hide_empty_fields) )
							{
								trezanik::core::aux::ip_address  ipaddr;
								ia.valid_gateway = trezanik::core::aux::string_to_ipaddr(ia.gateway.c_str(), ipaddr) > 0;
							}
							if ( !ia.valid_gateway && !(hide_empty_fields && ia.gateway.empty()) )
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
							if ( AddPropertyRow(PropertyRowType::TextInput, "Nameserver", &in.nameserver, hide_empty_fields) )
							{
								trezanik::core::aux::ip_address  ipaddr;
								in.valid_nameserver = trezanik::core::aux::string_to_ipaddr(in.nameserver.c_str(), ipaddr) > 0;
							}
							if ( !in.valid_nameserver && !(hide_empty_fields && in.nameserver.empty()) )
							{
								row_count++;
								ImGui::TextColored(ImVec4(200, 0, 0, 200), "Invalid format");
							}
							ImGui::PopID();
						}
						ImGui::PopID();
					}
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
		int   selected_num = TConverter<imgui::PinSocketShape>::ToUint8(shape) - 1;
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
					shape = TConverter<imgui::PinSocketShape>::FromUint8((uint8_t)selected_num + 1); // +1 as 0=Invalid
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
		auto&  ns = my_wksp_data.node_styles;
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
		auto  ps = my_wksp_data.pin_styles;
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
			std::string  lbl_d = "Link Hover Thickness##" + s.first;
			std::string  lbl_e = "Link Selected Thickness##" + s.first;
			std::string  lbl_f = "Link Thickness##" + s.first;
			std::string  lbl_g = "Outline Colour##" + s.first;
			std::string  lbl_h = "Socket Colour##" + s.first;
			std::string  lbl_i = "Socket Connected Radius##" + s.first;
			std::string  lbl_j = "Socket Hovered Radius##" + s.first;
			std::string  lbl_k = "Socket Radius##" + s.first;
			std::string  lbl_l = "Socket Shape##" + s.first;
			std::string  lbl_m = "Socket Thickness##" + s.first;

			inputfloat(&s.second->link_dragged_thickness, lbl_c);
			inputfloat(&s.second->link_hovered_thickness, lbl_d);
			inputfloat(&s.second->link_selected_outline_thickness, lbl_e);
			inputfloat(&s.second->link_thickness, lbl_f);
			colouredit4(s.second->outline_colour, lbl_g);
			colouredit4(s.second->socket_colour, lbl_h);
			inputfloat(&s.second->socket_connected_radius, lbl_i);
			inputfloat(&s.second->socket_hovered_radius, lbl_j);
			inputfloat(&s.second->socket_radius, lbl_k);
			comboshape(s.second->socket_shape, lbl_l);
			inputfloat(&s.second->socket_thickness, lbl_m);
		}

	} // pin styles
}


void
ImGuiWorkspace::DrawPropertyView_Pins(
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
		if ( !my_workspace->IsValidRelativePosition(p.pos.x, p.pos.y) )
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
ImGuiWorkspace::DrawServiceManagement()
{
	using namespace trezanik::core;

	// poor minimum calculations, doesn't consider font size
	const ImVec2  min_wnd_size = ImVec2(700.f, 300.f); // cover 4*125+50
	const ImVec2  min_section_size = ImVec2(125.f, 240.f);

	//if ( !is_draw_client )
	{
	ImGui::SetNextWindowPosCenter(ImGuiCond_Appearing); // not always, permit to move
	ImGui::SetNextWindowSizeConstraints(min_wnd_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::SetNextWindowSize(min_wnd_size, ImGuiCond_Appearing);

	if ( !ImGui::Begin("Service Management", &_gui_interactions.show_service_management, ImGuiWindowFlags_NoScrollbar) )
	{
		ImGui::End();
		return;
	}
	}

	ImVec2  wnd_size = ImGui::GetContentRegionAvail();
	ImVec2  mid_section = wnd_size;
	ImVec2  section_size = wnd_size;
	
	mid_section.x = 50.f; // hardcoded
	section_size.x -= mid_section.x;
	section_size.x -= ((ImGui::GetStyle().WindowPadding.x * 2) * 5); // left + right, 5 sections
	section_size.x *= 0.25; // 4 variable sections

	float   button_height = 30.f; // can't do FLT_MAX
	float   button_width = (section_size.x - 5) * 0.5f;
	ImVec2  button_size(button_width, button_height);

	// placeholder for disabled elements we still want drawn (with empty/null content)
	static std::string  tmp_str;
	static int tmp_int = 0;
	static int tmp_selection = -1;

	static bool  tooltips = false;
	static bool  service_group_modified = false; 
	static bool  service_modified = false;
	bool  svcgrp_modified_this_frame = false;
	bool  svc_modified_this_frame = false;

	auto active_group_equals_loaded = [](service_group& active, service_group& loaded)
	{
		if ( active.name != loaded.name )
			return false;
		if ( active.comment != loaded.comment )
			return false;
		if ( active.services != loaded.services )
			return false;

		return true;
	};
	auto active_svc_equals_loaded = [](service& active, service& loaded)
	{
		if ( active.name != loaded.name )
			return false;
		if ( active.comment != loaded.comment )
			return false;
		if ( active.port_num != loaded.port_num )
			return false;
		if ( active.port_num_high != loaded.port_num_high )
			return false;
		if ( active.icmp_code != loaded.icmp_code )
			return false;
		if ( active.icmp_type != loaded.icmp_type )
			return false;
		if ( active.protocol_num != loaded.protocol_num )
			return false;

		return true;
	};

	/*
	 * Disable everything unless service saved/cancelled. thankfully these stack!
	 * This also makes the separator texts (titles) show as disabled, which looks
	 * good but alas is inconsistent with the rest. I would like this layout, but
	 * needs a little more work and handling prior to each child window.
	 * Something to look at in future.
	 */
	if ( service_modified ) ImGui::BeginDisabled();

	ImGui::SetNextWindowSizeConstraints(min_section_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::BeginChild("Service Groups", section_size);
	{
		// these (and text inputs further down) don't fully expand...explicit set
		ImGui::PushItemWidth(section_size.x);
		ImGui::SeparatorText("Service Groups");

		if ( ImGui::BeginListBox("##AllServiceGroups") )
		{
			int   pos = -1;

			// if active service group modifications outstanding, selection disabled
			if ( service_group_modified ) ImGui::BeginDisabled();

			for ( auto& g : my_wksp_data.service_groups )
			{
				const bool  is_selected = (++pos == my_selected_service_group_index);

				if ( ImGui::Selectable(g->name.c_str(), is_selected) )
				{
					// since there's no trivial 'deselect', re-selection will clear
					if ( my_selected_service_group_index == pos )
					{
						TZK_LOG_FORMAT(LogLevel::Trace, "Unselected %s: %d (%s)", "Service Group", pos, g->name.c_str());
						ServiceManagementSelection(SvcMgmtSwitch::Unselect_ServiceGroup);
					}
					else
					{
						TZK_LOG_FORMAT(LogLevel::Trace, "Selected %s: %d (%s)", "Service Group", pos, g->name.c_str());
						my_selected_service_group_index = pos;
						my_loaded_service_group = g;
						// copy the object and apply edits to it until saved
						my_active_service_group = std::make_shared<service_group>(*my_loaded_service_group);
						ServiceManagementSelection(SvcMgmtSwitch::Select_ServiceGroup);
					}
				}
				if ( is_selected )
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			if ( service_group_modified ) ImGui::EndDisabled();
			ImGui::EndListBox();
		}
		ImGui::PopItemWidth();

		if ( service_group_modified ) ImGui::BeginDisabled();
		if ( ImGui::Button("Add##ServiceGroupAdd", button_size) )
		{
			TZK_LOG(LogLevel::Trace, "Creating new inline service group");
			ServiceManagementSelection(SvcMgmtSwitch::Unselect_ServiceGroup);
			my_active_service_group = std::make_shared<service_group>();
			my_active_service_group->name = "Service Group Name";
			svcgrp_modified_this_frame = true;
		}
		if ( service_group_modified ) ImGui::EndDisabled();

		ImGui::SameLine();

		// temporary required as 'unselect' makes index -1, breaking disabled stack count
		const bool  element_disabled = (my_selected_service_group_index == -1 || service_group_modified);

		if ( element_disabled ) ImGui::BeginDisabled();
		if ( ImGui::Button("Remove##ServiceGroupRemove", button_size) )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Removing service group '%s'", my_active_service_group->name.c_str());
			
			auto& vecref = my_wksp_data.service_groups;
			auto  iter = std::find_if(vecref.begin(), vecref.end(), [this](auto&& p) {
				return p->name == my_active_service_group->name;
			});
			if ( iter == vecref.end() )
			{
				TZK_LOG_FORMAT(LogLevel::Error, "Service group '%s' not found in map", my_active_service_group->name.c_str());
			}
			else
			{
				vecref.erase(iter);
			}

			my_loaded_service_group.reset();
			my_active_service_group.reset();
			ServiceManagementSelection(SvcMgmtSwitch::Unselect_ServiceGroup);
		}
		if ( element_disabled ) ImGui::EndDisabled();

	}
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::SetNextWindowSizeConstraints(min_section_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::BeginChild("Service Group", section_size);
	{
		ImGui::PushItemWidth(section_size.x);
		ImGui::SeparatorText("Service Group");

		if ( my_active_service_group == nullptr )
		{
			// much clearer keeping this separate, but could be done in-line
			ImGui::BeginDisabled();
			ImGui::InputTextWithHint("##ServiceGroupName", "Service Group Name", &tmp_str);
			if ( ImGui::BeginListBox("##IncludedServices") )
				ImGui::EndListBox();
			ImGui::InputTextWithHint("##ServiceGroupComment", "Comment", &tmp_str);
			ImGui::EndDisabled();
		}
		else
		{
			ImGui::InputTextWithHint("##ServiceGroupName", "Service Group Name", &my_active_service_group->name);
			if ( ImGui::IsItemEdited() )
				svcgrp_modified_this_frame = true;

			if ( ImGui::BeginListBox("##IncludedServices") )
			{
				int  pos = -1;

				// all actions taken on the copy
				for ( auto& s : my_active_service_group->services )
				{
					const bool  is_selected = (++pos == my_selected_service_group_service_index);

					if ( ImGui::Selectable(s.c_str(), is_selected) )
					{
						if ( service_modified )
						{
							// no-op
						}
						else if ( my_selected_service_group_service_index == pos )
						{
							TZK_LOG_FORMAT(LogLevel::Trace, "Unselected %s: %d (%s)", "Service [Included]", pos, s.c_str());
							ServiceManagementSelection(SvcMgmtSwitch::Unselect_ServiceGroupService);
						}
						else
						{
							TZK_LOG_FORMAT(LogLevel::Trace, "Selected %s: %d (%s)", "Service [Included]", pos, s.c_str());
							my_selected_service_group_service_index = pos;
							my_loaded_service = GetService(s.c_str());
							my_active_service = std::make_shared<service>(*my_loaded_service);
							service_modified = false;
							ServiceManagementSelection(SvcMgmtSwitch::Select_ServiceGroupService);
						}
					}
					if ( is_selected )
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
			ImGui::InputTextWithHint("##ServiceGroupComment", "Comment", &my_active_service_group->comment);
			if ( ImGui::IsItemEdited() )
				svcgrp_modified_this_frame = true;
		}
		
		ImGui::PopItemWidth();

		if ( svcgrp_modified_this_frame )
		{
			if ( my_loaded_service_group == nullptr )
				service_group_modified = true; // 'new' item
			else if ( !active_group_equals_loaded(*my_active_service_group, *my_loaded_service_group) )
				service_group_modified = true;
			else
				service_group_modified = false;
		}

		bool  element_disabled = !service_group_modified || my_active_service_group == nullptr || my_active_service_group->name.empty();
		
		if ( element_disabled ) ImGui::BeginDisabled();
		if ( ImGui::Button("Save##ServiceGroupSave", button_size) )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Saving service group: Name='%s', Comment='%s', Services(Count)=%zu",
				my_active_service_group->name.c_str(), my_active_service_group->comment.c_str(), my_active_service_group->services.size()
			);

			auto&  vecref = my_wksp_data.service_groups;
			auto   iter = vecref.end();
			std::string  svclist;

			if ( my_active_service_group != nullptr )
			{
				iter = std::find_if(vecref.begin(), vecref.end(), [this](auto&& p) {
					return p->name == my_active_service_group->name;
				});

				// all this is just to log (debug) the service list. could just report the count?
				for ( auto& s : my_active_service_group->services )
				{
					svclist += s;
					svclist += ";";
				}
				if ( !svclist.empty() )
					svclist.pop_back();
			}

			if ( iter != vecref.end() )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Updating existing service group: '%s': (%zu) %s", my_active_service_group->name.c_str(), my_active_service_group->services.size(), svclist.c_str());

				auto& orig = (*iter);
				orig->comment = my_active_service_group->comment;
				orig->name = my_active_service_group->name;
				orig->services = my_active_service_group->services;

				TZK_LOG_FORMAT(LogLevel::Debug, "Amended service group details: '%s': (%zu) %s", my_active_service_group->name.c_str(), my_active_service_group->services.size(), svclist.c_str());
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Adding new service group: '%s': (%zu) %s", my_active_service_group->name.c_str(), my_active_service_group->services.size(), svclist.c_str());

				vecref.push_back(my_active_service_group);
			}

			std::sort(vecref.begin(), vecref.end(), SortServiceGroup());
			
			// locate this new service group in the map so we can have it selected
			my_selected_service_group_index = 0;
			for ( auto& v : vecref )
			{
				if ( v->name == my_active_service_group->name )
					break;
				my_selected_service_group_index++;
			}
			assert((size_t)my_selected_service_group_index < vecref.size());

			service_group_modified = false;
		}

		ImGui::SameLine();

		if ( ImGui::Button("Cancel##ServiceGroupCancel", button_size) )
		{
			TZK_LOG(LogLevel::Debug, "Cancelling changes to service group");
			// reduplicate if we were not working on a temporary
			my_active_service_group.reset();
			if ( my_loaded_service_group != nullptr )
			{
				my_active_service_group = std::make_shared<service_group>(*my_loaded_service_group);
			}
			service_group_modified = false;
		}
		if ( element_disabled ) ImGui::EndDisabled();
	}
	ImGui::EndChild();
	ImGui::SameLine();
	ImGui::BeginGroup(); // service group <-> service
	{
		ImGui::PushItemWidth(mid_section.x);
		ImGui::Dummy(ImVec2(0.f, 50.f));

		const bool  inc_disabled = (my_active_service_group == nullptr || my_selected_service_index == -1);
		const bool  exc_disabled = (my_selected_service_group_service_index == -1);

		if ( inc_disabled ) ImGui::BeginDisabled();
		if ( ImGui::Button("<< Include") )
		{
			assert(my_active_service_group != nullptr);
			TZK_LOG_FORMAT(LogLevel::Debug, "Including service '%s' in '%s'", my_active_service->name.c_str(), my_active_service_group->name.c_str());
			my_active_service_group->services.push_back(my_active_service->name);

			service_group_modified = (my_loaded_service_group == nullptr) ?
				true : !active_group_equals_loaded(*my_active_service_group, *my_loaded_service_group);

			ServiceManagementSelection(SvcMgmtSwitch::Include);
		}
		if ( inc_disabled ) ImGui::EndDisabled();

		ImGui::Dummy(ImVec2(0.f, 25.f));

		if ( exc_disabled ) ImGui::BeginDisabled();
		if ( ImGui::Button("Exclude >>") )
		{
			assert(my_active_service_group != nullptr);
			auto  vpos = std::find(my_active_service_group->services.begin(), my_active_service_group->services.end(), my_active_service->name);
			TZK_LOG_FORMAT(LogLevel::Debug, "Excluding service '%s' from '%s'", my_active_service->name.c_str(), my_active_service_group->name.c_str());
			my_active_service_group->services.erase(vpos);

			service_group_modified = (my_loaded_service_group == nullptr) ?
				true : !active_group_equals_loaded(*my_active_service_group, *my_loaded_service_group);

			ServiceManagementSelection(SvcMgmtSwitch::Exclude);
		}
		if ( exc_disabled ) ImGui::EndDisabled();

		ImGui::PopItemWidth();
	}
	ImGui::EndGroup();
	ImGui::SameLine();
	ImGui::SetNextWindowSizeConstraints(min_section_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::BeginChild("Services", section_size);
	{
		ImGui::PushItemWidth(section_size.x);
		ImGui::SeparatorText("Services");
		if ( ImGui::BeginListBox("##AllServices") ) // filtered if service group selected
		{
			if ( service_modified ) ImGui::BeginDisabled();

			if ( my_active_service_group )
			{
				// show only services NOT in selected service group
				int   pos = -1;

				for ( auto& s : my_wksp_data.services )
				{
					bool  exists = false;

					for ( auto& as : my_active_service_group->services )
					{
						if ( as == s->name )
						{
							exists = true;
							break;
						}
					}

					if ( exists )
						continue;

					const bool  is_selected = (++pos == my_selected_service_index);

					if ( ImGui::Selectable(s->name.c_str(), is_selected) )
					{
						if ( my_selected_service_index == pos )
						{
							TZK_LOG_FORMAT(LogLevel::Trace, "Unselected %s: %d (%s)", "Service", pos, s->name.c_str());
							ServiceManagementSelection(SvcMgmtSwitch::Unselect_Service);
						}
						else
						{
							TZK_LOG_FORMAT(LogLevel::Trace, "Selected %s: %d (%s)", "Service", pos, s->name.c_str());
							my_selected_service_index = pos;
							my_loaded_service = s;
							my_active_service = std::make_shared<service>(*my_loaded_service);
							service_modified = false;
							ServiceManagementSelection(SvcMgmtSwitch::Select_Service);
						}
					}
					if ( is_selected )
					{
						ImGui::SetItemDefaultFocus();
					}
				}
			}
			else
			{
				// show all services
				int   pos = -1;

				for ( auto& s : my_wksp_data.services )
				{
					const bool  is_selected = (++pos == my_selected_service_index);

					if ( ImGui::Selectable(s->name.c_str(), is_selected) )
					{
						if ( my_selected_service_index == pos )
						{
							TZK_LOG_FORMAT(LogLevel::Trace, "Unselected %s: %d (%s)", "Service", pos, s->name.c_str());
							ServiceManagementSelection(SvcMgmtSwitch::Unselect_Service);
						}
						else
						{
							TZK_LOG_FORMAT(LogLevel::Trace, "Selected %s: %d (%s)", "Service", pos, s->name.c_str());
							my_selected_service_index = pos;
							my_loaded_service = s;
							my_active_service = std::make_shared<service>(*my_loaded_service);
							service_modified = false;
							ServiceManagementSelection(SvcMgmtSwitch::Select_Service);
						}
					}
					if ( is_selected )
					{
						ImGui::SetItemDefaultFocus();
					}
				}
			}

			if ( service_modified ) ImGui::EndDisabled();

			ImGui::EndListBox();
		}
		ImGui::PopItemWidth();

		if ( service_modified ) ImGui::BeginDisabled();
		if ( ImGui::Button("Add##ServiceAdd", button_size) )
		{
			TZK_LOG(LogLevel::Trace, "Creating new inline service");
			ServiceManagementSelection(SvcMgmtSwitch::Unselect_Service);
			// unselect purges these active+loaded services
			my_active_service = std::make_shared<service>();
			my_active_service->name = "Service Name";
			svc_modified_this_frame = true;
		}
		if ( service_modified ) ImGui::EndDisabled();
		
		ImGui::SameLine();

		const bool  element_disabled = (my_selected_service_index == -1 || service_modified);

		if ( element_disabled ) ImGui::BeginDisabled();
		if ( ImGui::Button("Remove##ServiceRemove", button_size) )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Removing service '%s'", my_active_service->name.c_str());

			/**
			 * @warning
			 * This service is already in active use in the workspace; while the
			 * nodegraph will remain valid due to usage of a shared_ptr, when
			 * saved it will be gone and result in a bad-validity workspace
			 * config.
			 * While we flag such an event, it'd be advisable to alert the
			 * user here of active use, and get confirmation, or outright
			 * reject the removal.
			 * element_disabled right above is ripe for being set against the
			 * active_service use_count...
			 */

			auto& vecref = my_wksp_data.services;
			auto  iter = std::find_if(vecref.begin(), vecref.end(), [this](auto&& p) {
					return p->name == my_active_service->name;
			});
			if ( iter == vecref.end() )
			{
				TZK_LOG_FORMAT(LogLevel::Error, "Service '%s' not found in map", my_active_service->name.c_str());
			}
			else
			{
				// potential check, but this is really volatile until we're stable..
				// use count; 1 = raw vector storage, 2 = us
				if ( iter->use_count() != 2 )
				{
					TZK_LOG_FORMAT(LogLevel::Error, "Service '%s' is in use; will not remove", (*iter)->name.c_str());
				}
				else
				{
					vecref.erase(iter);
				}
			}

			my_loaded_service.reset();
			my_active_service.reset();
			ServiceManagementSelection(SvcMgmtSwitch::Unselect_Service);
		}
		if ( element_disabled ) ImGui::EndDisabled();
	}
	ImGui::EndChild();
	ImGui::SameLine();
	if ( service_modified ) ImGui::EndDisabled();
	ImGui::SetNextWindowSizeConstraints(min_section_size, ImVec2(FLT_MAX, FLT_MAX));
	ImGui::BeginChild("Service Editor", section_size);
	{
		ImGui::PushItemWidth(section_size.x);
		ImGui::SeparatorText("Service");

		// NOTE: must match with our IPProto enum member order
		const char* ip_protos[] = { "", "TCP", "UDP", "ICMP" };

		if ( my_active_service == nullptr )
		{
			// much clearer keeping this separate, but could be done in-line
			ImGui::BeginDisabled();
			ImGui::InputTextWithHint("##ServiceName", "Service Name", &tmp_str);
			ImGui::PushItemWidth(100.f);
			ImGui::Combo("##", &tmp_selection, ip_protos, IM_ARRAYSIZE(ip_protos));
			ImGui::InputInt("Port##ServicePort", &tmp_int);
			ImGui::PopItemWidth();
			ImGui::InputTextWithHint("##ServiceComment", "Comment", &tmp_str);
			ImGui::EndDisabled();
		}
		else
		{
			ImGui::InputTextWithHint("##ServiceName", "Service Name", &my_active_service->name);
			if ( ImGui::IsItemEdited() )
				svc_modified_this_frame = true;
			
			ImGui::PushItemWidth(100.f);

			/*
			 * int casting to match imgui types only; apply validators after
			 * each frame/save/load to ensure accurate values
			 */
			ImGui::Combo("##", &my_active_service->protocol_num, ip_protos, IM_ARRAYSIZE(ip_protos));
			if ( my_active_service->protocol_num < 1 || my_active_service->protocol_num > IM_ARRAYSIZE(ip_protos) )
				my_active_service->protocol_num = 1; // first real value
			if ( ImGui::IsItemEdited() )
				svc_modified_this_frame = true;
			
			if ( my_active_service->protocol_num == IPProto::icmp )
			{
				/*
				 * Given the limited number of ICMP types, we could simply make
				 * this a dropdown
				 */
				ImGui::InputInt("Type##ICMPType", &my_active_service->icmp_type);
				if ( my_active_service->icmp_type < 0 )
					my_active_service->icmp_type = 0;
				else if ( my_active_service->icmp_type > UINT8_MAX )
					my_active_service->icmp_type = UINT8_MAX;
				if ( ImGui::IsItemEdited() )
					svc_modified_this_frame = true;

				ImGui::InputInt("Code##ICMPCode", &my_active_service->icmp_code);
				if ( my_active_service->icmp_code < 0 )
					my_active_service->icmp_code = 0;
				else if ( my_active_service->icmp_code > UINT8_MAX )
					my_active_service->icmp_code = UINT8_MAX;
				if ( ImGui::IsItemEdited() )
					svc_modified_this_frame = true;
			}
			else
			{
				// limit width so the label isn't cut off; we only need '0'-'65535' anyway
				ImGui::PushItemWidth(50.f);

				// low/lone port
				ImGui::InputInt("Port##ServicePortLow", &my_active_service->port_num, 0, 0);
				if ( my_active_service->port_num <= 0 )
					my_active_service->port_num = 1;
				else if ( my_active_service->port_num > UINT16_MAX )
					my_active_service->port_num = UINT16_MAX;
				if ( ImGui::IsItemEdited() )
					svc_modified_this_frame = true;

				// high-port, used for implementing ranges
				ImGui::InputInt("To Port##ServicePortHigh", &my_active_service->port_num_high, 0, 0);
				if ( my_active_service->port_num_high == 0 )
				{ /* permit 0 as unset for this only */ }
				else if ( my_active_service->port_num_high < my_active_service->port_num )
					my_active_service->port_num_high = my_active_service->port_num;
				else if ( my_active_service->port_num_high > UINT16_MAX )
					my_active_service->port_num_high = UINT16_MAX;
				if ( ImGui::IsItemEdited() )
					svc_modified_this_frame = true;

				ImGui::PopItemWidth();
			}
			

			ImGui::PopItemWidth();

			ImGui::InputTextWithHint("##ServiceComment", "Comment", &my_active_service->comment);
			if ( ImGui::IsItemEdited() )
				svc_modified_this_frame = true;
		}

		ImGui::PopItemWidth();

		if ( svc_modified_this_frame )
		{
			if ( my_loaded_service == nullptr )
				service_modified = true; // 'new' item
			else if ( !active_svc_equals_loaded(*my_active_service, *my_loaded_service) )
				service_modified = true;
			else
				service_modified = false;
		}

		const bool element_disabled = (!service_modified || my_active_service == nullptr || my_active_service->name.empty());

		if ( element_disabled ) ImGui::BeginDisabled();
		if ( ImGui::Button("Save##ServiceSave", button_size) )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Saving service: Name='%s', Port=%d, PortHigh=%d, Type=%d, Code=%d, Protocol=%d, Comment='%s'",
				my_active_service->name.c_str(), my_active_service->port_num, my_active_service->port_num_high, my_active_service->icmp_type, my_active_service->icmp_code,
				my_active_service->protocol_num, my_active_service->comment.c_str()
			);

			my_workspace->CheckServiceName(my_active_service->name);

			// Workspace::AddService loads; we assign
			my_active_service->protocol = TConverter<IPProto>::ToString((IPProto)my_active_service->protocol_num);

			if ( my_active_service->protocol_num == IPProto::icmp )
			{
				my_active_service->port = std::to_string(my_active_service->icmp_type);
				my_active_service->high_port = std::to_string(my_active_service->icmp_code);
			}
			else
			{
				my_active_service->port = std::to_string(my_active_service->port_num);
				if ( my_active_service->port_num_high != 0 )
				{
					my_active_service->high_port = std::to_string(my_active_service->port_num_high);
				}
			}

			auto&  vecref = my_wksp_data.services;
			auto   iter = vecref.end();
			
			if ( my_active_service != nullptr )
			{
				iter = std::find_if(vecref.begin(), vecref.end(), [this](auto&& p) {
					return p->name == my_active_service->name;
				});
			}

			int  i1 = my_active_service->port_num;
			int  i2 = my_active_service->port_num_high;
			if ( my_active_service->protocol_num == IPProto::icmp )
			{
				i1 = my_active_service->icmp_type;
				i2 = my_active_service->icmp_code;
			}

			if ( iter != vecref.end() )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Updating existing service: '%s': %s/%d-%d", my_active_service->name.c_str(), my_active_service->protocol.c_str(), i1, i2);

				auto&  orig = (*iter);
				orig->comment = my_active_service->comment;
				orig->high_port = my_active_service->high_port;
				orig->icmp_code = my_active_service->icmp_code;
				orig->icmp_type = my_active_service->icmp_type;
				orig->name = my_active_service->name;
				orig->port = my_active_service->port;
				orig->port_num = my_active_service->port_num;
				orig->port_num_high = my_active_service->port_num_high;
				orig->protocol = my_active_service->protocol;
				orig->protocol_num = my_active_service->protocol_num;

				// active & loaded service should be identical now
				TZK_LOG_FORMAT(LogLevel::Debug, "Amended service details: '%s': %s/%d-%d", my_active_service->name.c_str(), my_active_service->protocol.c_str(), i1, i2);
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Adding service '%s': %s/%d-%d", my_active_service->name.c_str(), my_active_service->protocol.c_str(), i1, i2);

				vecref.push_back(my_active_service);
			}

			// create a new copied instance for further modifications
			my_loaded_service = my_active_service;
			my_active_service.reset();
			my_active_service = std::make_shared<service>(*my_loaded_service);

			// always sort on save, minimal time; required for following selection too
			std::sort(vecref.begin(), vecref.end(), SortService());

			// locate this new service in the map so we can have it selected
			my_selected_service_index = 0;
			for ( auto& v : vecref )
			{
				/*
				 * Services list is dynamic, referencing the same map. Only way
				 * we can determine position in the service list is by also
				 * performing the presence check, and omitting those in here!
				 */
				if ( my_active_service_group != nullptr )
				{
					bool  listed = false;
					for ( auto& as : my_active_service_group->services )
					{
						if ( as == v->name )
						{
							listed = true;
							break;
						}
					}
					if ( listed )
						continue;
				}

				if ( v->name == my_active_service->name )
					break;
				my_selected_service_index++;
			}
			assert((size_t)my_selected_service_index < vecref.size());

			service_modified = false;
		}

		ImGui::SameLine();

		if ( ImGui::Button("Cancel##ServiceCancel", button_size) )
		{
			TZK_LOG(LogLevel::Debug, "Cancelling changes to service");
			// reduplicate
			my_active_service.reset();
			if ( my_loaded_service != nullptr )
			{
				my_active_service = std::make_shared<service>(*my_loaded_service);
			}
			service_modified = false;
		}
		if ( element_disabled ) ImGui::EndDisabled();

		// since we have the spare space, additional details can be output here

#if 0  // Code Disabled: future addition, tooltip assistance/display without selection?
		ImGui::Dummy(ImVec2(0.f, 50.f));
		ImGui::Checkbox("Tooltips##smgttt", &tooltips);
#endif
	}
	ImGui::EndChild();

	//if ( !is_draw_client )
	{
	ImGui::End();
	}
}


int
ImGuiWorkspace::DrawServiceSelector()
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
				for ( auto& svc : my_wksp_data.services )
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
				for ( auto& svcg : my_wksp_data.service_groups )
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
ImGuiWorkspace::GetNodeStyle(
	const char* name
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;

	for ( auto& s : my_wksp_data.node_styles )
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
		assert(!my_wksp_data.node_styles.empty());
		return my_wksp_data.node_styles.begin()->second;
	}

	return nullptr;
}


std::shared_ptr<trezanik::imgui::PinStyle>
ImGuiWorkspace::GetPinStyle(
	const char* name
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;

	for ( auto& s : my_wksp_data.pin_styles )
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
		assert(!my_wksp_data.pin_styles.empty());
		return my_wksp_data.pin_styles.begin()->second;
	}

	return nullptr;
}


std::string
ImGuiWorkspace::GetPinStyle(
	std::shared_ptr<trezanik::imgui::PinStyle> style
)
{
	for ( auto& s : my_wksp_data.pin_styles )
	{
		if ( s.second == style )
		{
			return s.first;
		}
	}

	return "";
}


std::shared_ptr<service>
ImGuiWorkspace::GetService(
	const char* name
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;

	for ( auto& svc : my_wksp_data.services )
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
ImGuiWorkspace::GetService(
	trezanik::core::UUID& id
)
{
	auto  iter = std::find_if(my_wksp_data.services.begin(), my_wksp_data.services.end(), [&id](auto&& p) {
		return p->id == id;
	});
	if ( iter == my_wksp_data.services.end() )
	{
		return nullptr;
	}

	return *iter;
}


std::shared_ptr<service_group>
ImGuiWorkspace::GetServiceGroup(
	const char* name
)
{
	using namespace trezanik::core;

	bool  case_sensitive = true;

	for ( auto& grp : my_wksp_data.service_groups )
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
ImGuiWorkspace::GetServiceGroup(
	trezanik::core::UUID& id
)
{
	auto  iter = std::find_if(my_wksp_data.service_groups.begin(), my_wksp_data.service_groups.end(), [&id](auto&& p) {
		return p->id == id;
	});
	if ( iter == my_wksp_data.service_groups.end() )
	{
		return nullptr;
	}

	return *iter;
}


#if 0 // Code Disabled: testing PoC for Command pattern
std::shared_ptr<IsochroneNode>
ImGuiWorkspace::GetUndoRedoNode(
	std::shared_ptr<IsochroneNode> node,
	trezanik::imgui::NodeUpdate nu
)
{
	/*
	 * Entrypoints:
	 * 1) App start, no prior nodes. return nullptr
	 * 2) Unrelated modification done last. return nullptr
	 * 3) Related/same modification done last. return undoredo node, so it can be updated
	 */

	if ( !my_undoredo_nodes.empty() && my_undoredo_nodes.back()->Difference(node) == nu )
	{
		// same difference; return the last
		return my_undoredo_nodes.back();
	}

	return nullptr;
}
#endif


std::shared_ptr<Workspace>
ImGuiWorkspace::GetWorkspace()
{
	return my_workspace;
}


int
ImGuiWorkspace::IndexFromNodeStyle(
	std::string* style
)
{
	int  index = 0;
	for ( auto& str : my_wksp_data.node_styles )
	{
		if ( str.first == *style )
			return index;

		index++;
	}

	return -1;
}


int
ImGuiWorkspace::IndexFromPinStyle(
	std::string* style
)
{
	int  index = 0;

	for ( auto& str : my_wksp_data.pin_styles )
	{
		if ( str.first == *style )
			return index;

		index++;
	}

	return -1;
}


trezanik::app::pin
ImGuiWorkspace::NodeGraphPinToWorkspacePin(
	trezanik::imgui::Pin* ng_pin
)
{
	using namespace trezanik::core;

	auto  sp = dynamic_cast<ServerPin*>(ng_pin);
	auto  cp = dynamic_cast<ClientPin*>(ng_pin);
	auto  tp = dynamic_cast<ConnectorPin*>(ng_pin);
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
		UUID  uuid;
		trezanik::app::pin  retval(uuid, pos, PinType::Invalid);
		return retval;
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

	return retval;
}


int
ImGuiWorkspace::Notification(
	trezanik::core::UUID& uuid,
	trezanik::imgui::NodeUpdate update
)
{
	using namespace trezanik::core;
	using namespace trezanik::imgui;

	/*
	 * Remember, we're invoked from BaseNode calling its NotifyListeners method,
	 * this being our implementation.
	 * It is presently the only way - outside of handling all interior methods
	 * inline, which is spammy - to handle updates, which enables us to keep the
	 * various node structs in sync so they can be written to file properly.
	 * 
	 * After some refactoring, there are four items that must be handled here,
	 * with everything else only being used for Command pattern and Events:
	 * 1) Position
	 *    Member variable of BaseNode; drag operations are local to the class.
	 *    No reason we couldn't have it held as a pointer to the graph node
	 *    following a quick source check...
	 * 2) Size
	 *    Just like Position, this is another member variable of BaseNode and
	 *    could be a pointer
	 * 3) Pin Add
	 *    Pins are created within the nodegraph and have no equivalent object
	 *    creation, with BaseNode using the raw Pin object
	 * 4) Pin Remove
	 *    As per Pin Add too
	 * 
	 * With a little effort I don't see why this couldn't be entirely clear, but
	 * we need it for other usage too so might as well retain original state
	 * unless something needs us to change.
	 */

	std::shared_ptr<IsochroneNode>  node;


	for ( auto& n : my_nodegraph.GetNodes() )
	{
		// find the node
		if ( n->GetID() == uuid )
		{
			node = std::dynamic_pointer_cast<IsochroneNode>(n);
			break;
		}
	}
	
	if ( node == nullptr )
	{
		TZK_LOG(LogLevel::Warning, "No associated node");
		return ENOENT;
	}

	auto  gn = node->GetGraphNode();

#if 0 // Code Disabled: testing PoC for Command pattern
	ImVec4  v4;
	command_data  cur;
	command_data  now;
#endif

	// handle the update
	switch ( update )
	{
	case NodeUpdate::Boundary:
		break;
	case NodeUpdate::Created:
		// redundant; workspace + imgui have the latest node trackings
		break;
	case NodeUpdate::Dragged:
		{
			// no workspace sync needed

			EventData::node_update  nu;
			
			nu.flags = NodeUpdateFlags_Position;
			nu.node_uuid = uuid;
			nu.workspace_uuid = my_workspace->GetID();

			my_evtmgr.DispatchEvent(uuid_nodeupdate, nu);

#if 0 // Code Disabled: testing PoC for Command pattern
			auto inode = GetUndoRedoNode(node, NodeUpdate::Position);
			if ( inode != nullptr )
			{
				inode->SetPosition(node->GetPosition());
				my_commands.back().GetAfter().vec2 = node->GetPosition();
			}
			else
			{
				now.id = uuid;
				now.vec2 = node->GetPosition();

				my_commands.emplace_back(Cmd::NodeMove, std::move(cur), std::move(now));
			}
#endif
		}
		break;
	case NodeUpdate::MarkedForDeletion:
		{
			// node deletions are already handled (nodegraph update, pre-draw), but should be done here
		
			EventData::node_baseline  n;
			n.node_uuid = uuid;
			n.workspace_uuid = my_workspace->GetID();

			// candidate for delayed dispatch, since this isn't 'deleted' yet
			my_evtmgr.DispatchEvent(uuid_nodedelete, n);

#if 0 // Code Disabled: testing PoC for Command pattern
			cur.id = uuid;
			// must hold all properties, including style, pins, <links>, listeners
			// have workspace retain the deleted node(s) in another vector?
			now.id = uuid;

			my_commands.emplace_back(Cmd::NodeDelete, std::move(cur), std::move(now));
#endif
		}
		break;
	case NodeUpdate::Name:
		{
		
			EventData::node_update  nu;
			
			nu.flags = NodeUpdateFlags_Name;
			nu.node_uuid = uuid;
			nu.workspace_uuid = my_workspace->GetID();

			my_evtmgr.DispatchEvent(uuid_nodeupdate, nu);

			//my_workspace->CommandIssued(NameEdit)
		}
		break;
	case NodeUpdate::LinkBroken:
		{
		}
		break;
	case NodeUpdate::LinkEstablished:
		{
		}
		break;
	case NodeUpdate::PinAdded:
		{
			auto  oldc = gn->pins.size();

			/*
			 * Iterate all node pins; for each, iterate all gn pins. If found,
			 * it isn't new. If not, create it.
			 * Node pins have been updated by imgui, gn pins are delayed.
			 */
			for ( auto& p : node->GetPins() )
			{
				bool  found = false;
				
				for ( auto& gnp : gn->pins )
				{
					if ( gnp.id == p->GetID() )
					{
						found = true;
						break;
					}
				}
				if ( !found )
				{
					gn->pins.emplace_back(NodeGraphPinToWorkspacePin(p.get()));

					EventData::node_update  nu;
					
					nu.flags = NodeUpdateFlags_PinAdd;
					nu.node_uuid = uuid;
					nu.workspace_uuid = my_workspace->GetID();

					my_evtmgr.DispatchEvent(uuid_nodeupdate, nu);
					break;
				}
			}

			auto  newc = gn->pins.size();

			if ( newc - 1 != oldc )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Expecting new pin count of %zu, is instead %zu", oldc + 1, newc);
			}
		}
		break;
	case NodeUpdate::PinRemoved:
		{
			auto  oldc = gn->pins.size();

			/*
			 * Iterate all gn pins; for each, iterate all node pins. If found,
			 * it still exists. If not, delete it.
			 * Node pins have been updated by imgui, gn pins are delayed.
			 */
			for ( auto iter = gn->pins.begin(); iter != gn->pins.end(); iter++ )
			{
				bool  found = false;

				for ( auto& p : node->GetPins() )
				{
					if ( iter->id == p->GetID() )
					{
						found = true;
						break;
					}
				}

				if ( !found )
				{
					gn->pins.erase(iter);

					EventData::node_update  nu;
					
					nu.flags = NodeUpdateFlags_PinDel;
					nu.node_uuid = uuid;
					nu.workspace_uuid = my_workspace->GetID();

					my_evtmgr.DispatchEvent(uuid_nodeupdate, nu);
					break;
				}
			}

			auto  newc = gn->pins.size();

			if ( newc + 1 != oldc )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Expecting new pin count of %zu, is instead %zu", oldc - 1, newc);
			}
		}
		break;
	case NodeUpdate::Position:
		{
			/*
			 * Dragging the node in the graph will not result in the graph_node
			 * being updated (node has local members), so we have to sync it
			 * back with each move
			 */
			auto  pos = node->GetPosition();
			gn->position = pos;

			// CommandPattern - on issue, does the syncing, so it WILL have the before+after values, and can be removed from here
			//my_commands.emplace_back(Cmd::NodeMove, datbefore, datnow);
			//my_workspace->CommandIssued(ChangePosition(id, pos);

			EventData::node_update  nu;

			nu.flags = NodeUpdateFlags_Position;
			nu.node_uuid = uuid;
			nu.workspace_uuid = my_workspace->GetID();
			
			my_evtmgr.DispatchEvent(uuid_nodeupdate, nu);
		}
		break;
	case NodeUpdate::Selected:
	case NodeUpdate::Unselected: // same operation
		{

		}
		break;
	case NodeUpdate::Size:
		{
			// As per Position, dynamic updates need sync back
			auto  size = node->GetSize();
			gn->size = size;

			//my_workspace->CommandIssued(ChangeSize(gn);

			EventData::node_update  nu;

			nu.flags = NodeUpdateFlags_Size;
			nu.node_uuid = uuid;
			nu.workspace_uuid = my_workspace->GetID();

			my_evtmgr.DispatchEvent(uuid_nodeupdate, nu);
		}
		break;
	case NodeUpdate::Style:
		{
			EventData::node_update  nu;

			nu.flags = NodeUpdateFlags_Style;
			nu.node_uuid = uuid;
			nu.workspace_uuid = my_workspace->GetID();

			my_evtmgr.DispatchEvent(uuid_nodeupdate, nu);
		}
		break;
	case NodeUpdate::Type:
		break;
	default:
		// unknown/unhandled
		return ErrFAILED;
	}

	return ErrNONE;
}


int
ImGuiWorkspace::RemoveLink(
	const trezanik::core::UUID& id
)
{
	using namespace trezanik::core;

	for ( auto& l : my_wksp_data.links )
	{
		if ( l->id == id )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Removing link %s", l->id.GetCanonical());

			EventData::link_baseline  lnk;
			lnk.workspace_uuid = my_workspace->GetID();
			lnk.link_uuid = l->id;
			lnk.source_uuid = l->source;
			lnk.target_uuid = l->target;

			my_wksp_data.links.erase(l);
			core::ServiceLocator::EventDispatcher()->DispatchEvent(uuid_linkdelete, lnk);
			return ErrNONE;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Link ID '%s' not found", id.GetCanonical());
	return ENOENT;
}


int
ImGuiWorkspace::RemoveLink(
	const trezanik::core::UUID& srcid,
	const trezanik::core::UUID& tgtid
)
{
	using namespace trezanik::core;

	for ( auto& l : my_wksp_data.links )
	{
		if ( l->source == srcid && l->target == tgtid )
		{
			TZK_LOG_FORMAT(LogLevel::Trace, "Removing link %s", l->id.GetCanonical());

			EventData::link_baseline  lnk;
			lnk.workspace_uuid = my_workspace->GetID();
			lnk.link_uuid = l->id;
			lnk.source_uuid = l->source;
			lnk.target_uuid = l->target;

			my_wksp_data.links.erase(l);
			core::ServiceLocator::EventDispatcher()->DispatchEvent(uuid_linkdelete, lnk);

			return ErrNONE;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Warning, "Link pairing source ID '%s' and target ID '%s' not found",
		srcid.GetCanonical(), tgtid.GetCanonical()
	);
	return ENOENT;
}


int
ImGuiWorkspace::RemoveNode(
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
	 * be the one to tell it to purge.
	 */

	// remove from visual grid
	my_nodegraph.DeleteNode(node);

	node->RemoveListener(this);
	

	auto mapiter = std::find_if(my_nodes.begin(), my_nodes.end(), [&node](auto& p)
	{
		return p.second->GetID() == node->GetID();
	});
	if ( mapiter == my_nodes.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Could not find node for '%s'",
			node->GetID().GetCanonical()
		);

		// extra check, search and remove from dataset still? Handle by function
		return ENOENT;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Removing node '%s' from map", node->GetID().GetCanonical());
	my_nodes.erase(mapiter);

	auto setiter = std::find_if(my_wksp_data.nodes.begin(), my_wksp_data.nodes.end(), [&node](auto& p)
	{
		return p->id == node->GetID();
	});
	if ( setiter == my_wksp_data.nodes.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Error,
			"Could not find node for '%s'",
			node->GetID().GetCanonical()
		);
		return ENOENT;
	}
	
	TZK_LOG_FORMAT(LogLevel::Debug, "Removing node '%s' from set", node->GetID().GetCanonical());
	my_wksp_data.nodes.erase(setiter);

	return ErrNONE;
}


int
ImGuiWorkspace::RemoveNodeStyle(
	std::shared_ptr<trezanik::imgui::NodeStyle> style
)
{
	using namespace trezanik::core;

	auto  iter = std::find_if(my_wksp_data.node_styles.begin(), my_wksp_data.node_styles.end(), [style](auto entry)
	{
		return entry.second == style;
	});

	if ( iter == my_wksp_data.node_styles.end() )
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

	for ( auto& n : my_wksp_data.nodes )
	{
		if ( STR_compare(n->style.c_str(), iter->first.c_str(), false) == 0 )
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
	my_wksp_data.node_styles.erase(iter);

	return ErrNONE;
}


int
ImGuiWorkspace::RemoveNodeStyle(
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

	for ( auto& n : my_wksp_data.nodes )
	{
		if ( STR_compare(n->style.c_str(), name, false) == 0 )
		{
			count++;
		}
	}

	if ( count > 0 )
	{
		TZK_LOG_FORMAT(LogLevel::Error, "Unable to remove style '%s' - is in use by %zu nodes", name, count);
		return EBUSY;
	}

	auto  iter = std::find_if(my_wksp_data.pin_styles.begin(), my_wksp_data.pin_styles.end(), [name](auto& entry)
	{
		return entry.first.compare(name) == 0;
	});
	if ( iter == my_wksp_data.pin_styles.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Node style '%s' not found", name);
		return ENOENT;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Removing style '%s'", name);
	my_wksp_data.pin_styles.erase(iter);

	return ErrNONE;
}


int
ImGuiWorkspace::RemovePinStyle(
	std::shared_ptr<trezanik::imgui::PinStyle> style
)
{
	using namespace trezanik::core;

	auto  iter = std::find_if(my_wksp_data.pin_styles.begin(), my_wksp_data.pin_styles.end(), [style](auto& entry)
	{
		return entry.second == style;
	});

	if ( iter == my_wksp_data.pin_styles.end() )
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

	for ( auto& n : my_wksp_data.nodes )
	{
		for ( auto& p : n->pins )
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
	my_wksp_data.pin_styles.erase(iter);

	return ErrNONE;
}


int
ImGuiWorkspace::RemovePinStyle(
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
	for ( auto& n : my_wksp_data.nodes )
	{
		for ( auto& p : n->pins )
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
	auto  iter = std::find_if(my_wksp_data.pin_styles.begin(), my_wksp_data.pin_styles.end(), [name](auto& entry)
	{
		return entry.first.compare(name) == 0;
	});
	if ( iter == my_wksp_data.pin_styles.end() )
	{
		TZK_LOG_FORMAT(LogLevel::Warning, "Pin style '%s' not found", name);
		return ENOENT;
	}

	TZK_LOG_FORMAT(LogLevel::Debug, "Removing style '%s'", name);
	my_wksp_data.pin_styles.erase(iter);

	return ErrNONE;
}


int
ImGuiWorkspace::RenameNodeStyle(
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

	for ( auto& s : my_wksp_data.node_styles )
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
ImGuiWorkspace::RenamePinStyle(
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

	for ( auto& s : my_wksp_data.pin_styles )
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
ImGuiWorkspace::ServiceManagementSelection(
	SvcMgmtSwitch what
)
{
	switch ( what )
	{
	case SvcMgmtSwitch::Exclude:
		{
			my_selected_service_group_service_index = -1;
			my_active_service.reset();
		}
		break;
	case SvcMgmtSwitch::Include:
		{
			my_selected_service_index = -1;
			my_active_service.reset();
		}
		break;
	case SvcMgmtSwitch::Select_Service:
		{
			assert(my_active_service != nullptr);
			assert(my_loaded_service != nullptr);
			assert(my_selected_service_index != -1);
			my_selected_service_group_service_index = -1;
		}
		break;
	case SvcMgmtSwitch::Select_ServiceGroup:
		{
			assert(my_active_service_group != nullptr);
			assert(my_loaded_service_group != nullptr);
			assert(my_selected_service_group_index != -1);
			my_selected_service_group_service_index = -1;
			if ( my_active_service )
			{
				my_active_service.reset();
				my_selected_service_index = -1;
			}
		}
		break;
	case SvcMgmtSwitch::Select_ServiceGroupService:
		{
			assert(my_active_service != nullptr);
			assert(my_loaded_service != nullptr);
			assert(my_selected_service_group_service_index != -1);
			my_selected_service_index = -1;
		}
		break;
	case SvcMgmtSwitch::Unselect_Service:
		{
			my_active_service.reset();
			my_loaded_service.reset();
			my_selected_service_index = -1;
		}
		break;
	case SvcMgmtSwitch::Unselect_ServiceGroup:
		{
			my_active_service_group.reset();
			my_loaded_service_group.reset();
			my_selected_service_group_index = -1;
			my_selected_service_group_service_index = -1;
#if 0  // don't unselect a non-included service, as unrelated
			my_active_service = nullptr;
			my_selected_service_index = -1;
#endif
		}
		break;
	case SvcMgmtSwitch::Unselect_ServiceGroupService:
		{
			my_active_service.reset();
			my_loaded_service.reset();
			my_selected_service_group_service_index = -1;
		}
		break;
	default:
		break;
	}
}


int
ImGuiWorkspace::SetWorkspace(
	std::shared_ptr<Workspace> wksp
)
{
	using namespace trezanik::core;

	int  retval = ErrNONE;

	if ( wksp == nullptr )
	{
		if ( my_workspace != nullptr )
		{
			TZK_LOG(LogLevel::Trace, "Workspace unset");
		}

		my_workspace = wksp;
		return retval;
	}

	TZK_LOG_FORMAT(LogLevel::Debug,
		"Workspace assigned to window - %s : %s",
		wksp->ID().GetCanonical(), wksp->Name().c_str()
	);

	my_workspace = wksp;

	// no event/notification triggers until a workspace is assigned in


	my_wksp_data = my_workspace->WorkspaceData();

	/*
	 * Load all the workspaces nodes into ImNodes mappings.
	 * All operations are performed on these until the window is closed or
	 * otherwise saved, at which point they are synchronized back.
	 */
	auto& nodes = my_wksp_data.nodes;

	for ( auto& iter : nodes )
	{
		auto  snode = std::dynamic_pointer_cast<graph_node_system>(iter);
		if ( snode != nullptr )
		{
			AddNode(snode);
			continue;
		}
		auto  mnode = std::dynamic_pointer_cast<graph_node_multisystem>(iter);
		if ( mnode != nullptr )
		{
			AddNode(mnode);
			continue;
		}
		auto  bnode = std::dynamic_pointer_cast<graph_node_boundary>(iter);
		if ( bnode != nullptr )
		{
			AddNode(bnode);
			continue;
		}

		TZK_LOG_FORMAT(LogLevel::Error, "Unrecognized or unhandled node type '%s'", iter->type.name());
	}

	for ( auto& iter : my_wksp_data.links )
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
			for ( auto& p : n->pins )
			{
				// find the pin id for source and target
				if ( p.id == iter->source || p.id == iter->target )
				{
					TZK_LOG_FORMAT(LogLevel::Trace,
						"Found link %s in node %s (%s) for link: %s -> %s",
						p.id == iter->source ? "source" : "target",
						n->id.GetCanonical(), n->name.c_str(),
						iter->source.GetCanonical(), iter->target.GetCanonical()
					);

					// find the IsochroneNode* for the app::graph_node_system
					if ( p.type == PinType::Server )
					{
						ptgt = &p;

						auto niter = std::find_if(my_nodes.begin(), my_nodes.end(), [&n](auto&& node)
						{
							//TZK_LOG_FORMAT(LogLevel::Trace, "%s : %s", node->GetID().GetCanonical(), n->id.GetCanonical());
							return node.second->GetID() == n->id;
						});
						if ( niter == my_nodes.end() )
						{
							TZK_LOG_FORMAT(LogLevel::Error,
								"No matching graph node %s found for link: %s -> %s",
								"target", iter->source.GetCanonical(), iter->target.GetCanonical()
							);
							retval = ErrDATA;
							continue;
						}
						
						ntgt = niter->second;
					}
					else if ( p.type == PinType::Client )
					{
						psrc = &p;
						auto niter = std::find_if(my_nodes.begin(), my_nodes.end(), [&n](auto&& node)
						{
							//TZK_LOG_FORMAT(LogLevel::Trace, "%s : %s", node->GetID().GetCanonical(), n->id.GetCanonical());
							return node.second->GetID() == n->id;
						});
						if ( niter == my_nodes.end() )
						{
							TZK_LOG_FORMAT(LogLevel::Error,
								"No matching graph node %s found for link: %s -> %s",
								"source", iter->source.GetCanonical(), iter->target.GetCanonical()
							);
							retval = ErrDATA;
							continue;
						}
						
						nsrc = niter->second;
					}
					else if ( p.type == PinType::Connector )
					{
						auto niter = std::find_if(my_nodes.begin(), my_nodes.end(), [&n](auto&& node)
						{
							//TZK_LOG_FORMAT(LogLevel::Trace, "%s : %s", node->GetID().GetCanonical(), n->id.GetCanonical());
							return node.second->GetID() == n->id;
						});
						if ( niter == my_nodes.end() )
						{
							TZK_LOG_FORMAT(LogLevel::Error,
								"No matching graph node found for link: %s -> %s",
								iter->source.GetCanonical(), iter->target.GetCanonical()
							);
							retval = ErrDATA;
							continue;
						}
						if ( psrc == nullptr )
						{
							psrc = &p;
							nsrc = niter->second;
						}
						else if ( ptgt == nullptr )
						{
							ptgt = &p;
							ntgt = niter->second;
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
			retval = ErrDATA;
			continue;
		}
		if ( ptgt == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"No target found for link: %s -> %s",
				iter->source.GetCanonical(), iter->target.GetCanonical()
			);
			retval = ErrDATA;
			continue;
		}
		if ( impin_inp == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"No target pin found for link: %s -> %s",
				iter->source.GetCanonical(), iter->target.GetCanonical()
			);
			retval = ErrDATA;
			continue;
		}
		if ( impin_out == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Error,
				"No source pin found for link: %s -> %s",
				iter->source.GetCanonical(), iter->target.GetCanonical()
			);
			retval = ErrDATA;
			continue;
		}

		TZK_LOG_FORMAT(LogLevel::Trace,
			"Creating Link for " TZK_PRIxPTR " to " TZK_PRIxPTR,
			impin_out.get(), impin_inp.get()
		);


		/*
		 * REFACTOR; pre-create link object, pass down
		 * without dependency issue, imgui link could simply inherit and extend
		 * also vice-versa right now, but that's impossible for nodes and inconsistent!
		 */
		
		/*
		 * create the link in the nodegraph itself
		 * raw parameters as nodegraph isn't aware of ::app datatypes (future resolution)
		 */
		auto  ngl = my_nodegraph.CreateLink(iter->id, impin_out, impin_inp, &iter->text, (ImVec2*)&iter->offset);
		// assign the link to the pins
		impin_inp->AssignLink(ngl);
		impin_out->AssignLink(ngl);


		/*
		 * Pin CreateLink and here are the sources of 'creation' and therefore
		 * tooltips, without being per-frame. Listeners are always handled via
		 * their constructor, then dynamic updates
		 */
		auto iscn = dynamic_cast<IsochroneNode*>(impin_inp->GetAttachedNode());
		std::string  tt = "Connected to:\n";
		tt += iscn->Name();
		tt += " : ";
		tt += impin_inp->GetID().GetCanonical();
		impin_out->SetTooltipText(tt);
	}

	// no-op needed for node_styles

	// no-op needed for pin_styles

	// no-op needed for services

	// no-op needed for service_groups

	// settings
	/*
	 * Special case:
	 * Any draw clients not specified in settings will not be available in the
	 * menu, or anywhere else (will add to its config props later).
	 * This is especially true on the first run for a new workspace. We always
	 * want these available for selection, so ensure they always exist!
	 * Update visibility based on the config.
	 */
	WindowLocation  cdbg_loc = WindowLocation::Hidden;
	WindowLocation  propview_loc = WindowLocation::Hidden;

	for ( auto& setting : my_wksp_data.settings )
	{
		if ( setting.first.compare(settingname_dock_propview) == 0 )
		{
			propview_loc = TConverter<WindowLocation>::FromString(setting.second);
			if ( propview_loc == WindowLocation::Invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring invalid setting supplied for '%s': %s", setting.first.c_str(), setting.second.c_str());
				propview_loc = WindowLocation::Hidden;
			}
		}
		else if ( setting.first.compare(settingname_dock_canvasdbg) == 0 )
		{
			cdbg_loc = TConverter<WindowLocation>::FromString(setting.second);
			if ( cdbg_loc == WindowLocation::Invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring invalid setting supplied for '%s': %s", setting.first.c_str(), setting.second.c_str());
				cdbg_loc = WindowLocation::Hidden;
			}
		}
		else if ( setting.first.compare(settingname_grid_colour_background) == 0 )
		{
			size_t  v = core::TConverter<size_t>::FromString(setting.second);
			if ( v <= UINT32_MAX )
			{
				my_nodegraph.settings.grid_style.colours.background = static_cast<uint32_t>(v);
				my_nodegraph.GetCanvas().configuration.colour = my_nodegraph.settings.grid_style.colours.background;
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring invalid setting supplied for '%s': %s", setting.first.c_str(), setting.second.c_str());
			}
		}
		else if ( setting.first.compare(settingname_grid_colour_primary) == 0 )
		{
			size_t  v = core::TConverter<size_t>::FromString(setting.second);
			if ( v <= UINT32_MAX )
			{
				my_nodegraph.settings.grid_style.colours.primary = static_cast<uint32_t>(v);
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring invalid setting supplied for '%s': %s", setting.first.c_str(), setting.second.c_str());
			}
		}
		else if ( setting.first.compare(settingname_grid_colour_secondary) == 0 )
		{
			size_t  v = core::TConverter<size_t>::FromString(setting.second);
			if ( v <= UINT32_MAX )
			{
				my_nodegraph.settings.grid_style.colours.secondary = static_cast<uint32_t>(v);
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring invalid setting supplied for '%s': %s", setting.first.c_str(), setting.second.c_str());
			}

		}
		else if ( setting.first.compare(settingname_grid_colour_origin) == 0 )
		{
			size_t  v = core::TConverter<size_t>::FromString(setting.second);
			if ( v <= UINT32_MAX )
			{
				my_nodegraph.settings.grid_style.colours.origins = static_cast<uint32_t>(v);
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring invalid setting supplied for '%s': %s", setting.first.c_str(), setting.second.c_str());
			}

		}
		else if ( setting.first.compare(settingname_grid_draw) == 0 )
		{
			my_nodegraph.settings.grid_style.draw = core::TConverter<bool>::FromString(setting.second);
		}
		else if ( setting.first.compare(settingname_grid_draworigin) == 0 )
		{
			my_nodegraph.settings.grid_style.draw_origin = core::TConverter<bool>::FromString(setting.second);
		}
		else if ( setting.first.compare(settingname_grid_size) == 0 )
		{
			size_t  v = core::TConverter<size_t>::FromString(setting.second);
			if ( v >= 10 && v <= 100 && v % 10 == 0 )
			{
				my_nodegraph.settings.grid_style.size = static_cast<int>(v);
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring invalid setting supplied for '%s': %s", setting.first.c_str(), setting.second.c_str());
			}
		}
		else if ( setting.first.compare(settingname_grid_subdivisions) == 0 )
		{
			size_t  v = core::TConverter<size_t>::FromString(setting.second);
			if ( v == 1 || v == 2 || v == 5 || v == 10 )
			{
				my_nodegraph.settings.grid_style.subdivisions = static_cast<int>(v);
			}
			else
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Ignoring invalid setting supplied for '%s': %s", setting.first.c_str(), setting.second.c_str());
			}
		}
		else if ( setting.first.compare(settingname_node_drawheaders) == 0 )
		{
			my_nodegraph.settings.node_draw_headers = core::TConverter<bool>::FromString(setting.second);
		}
		else if ( setting.first.compare(settingname_node_dragfromheadersonly) == 0 )
		{
			my_nodegraph.settings.node_drag_from_headers_only = core::TConverter<bool>::FromString(setting.second);
		}
	}

	AssignDockClient("Canvas Debug", cdbg_loc, std::bind(&trezanik::imgui::ImNodeGraph::DrawDebug, &my_nodegraph), drawclient_canvasdbg_uuid);
	AssignDockClient("Property View", propview_loc, std::bind(&ImGuiWorkspace::DrawPropertyView, this), drawclient_propview_uuid);

	return retval;
}


void
ImGuiWorkspace::UpdatePinTooltip(
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
			auto iscn = dynamic_cast<IsochroneNode*>(l->Target()->GetAttachedNode());
			tooltip += "\n";
			tooltip += iscn->Name();
#if 0  // Code Disabled: UUID in tooltip as well, undesired
			tooltip += " [";
#if 0  // Pins with names
			if ( !sp->GetName()->empty() )
			{
				tooltip += sp->Name();
			}
			else
#endif
			{
				tooltip += sp->GetID().GetCanonical();
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
			tooltip += is_source ? *l->Target()->GetAttachedNode()->GetName() : *l->Source()->GetAttachedNode()->GetName();
		}
	}
	else
	{
		// Client/Connector pin, disconnected. Blank tooltip = no display
	}

	pin->SetTooltipText(tooltip);
}


std::tuple<std::shared_ptr<DrawClient>, WindowLocation>
ImGuiWorkspace::UpdateDrawClientDockLocation(
	const trezanik::core::UUID& drawclient_id,
	WindowLocation newloc
)
{
	using namespace trezanik::core;

	if ( drawclient_id == drawclient_canvasdbg_uuid )
	{
		my_wksp_data.settings[settingname_dock_canvasdbg] = TConverter<WindowLocation>::ToString(newloc);
	}
	else if ( drawclient_id == drawclient_propview_uuid )
	{
		my_wksp_data.settings[settingname_dock_propview] = TConverter<WindowLocation>::ToString(newloc);
	}

	// find the draw client to return details to the caller
	for ( auto& dc : my_draw_clients )
	{
		if ( dc->id == drawclient_id )
		{
			auto  retval = std::make_tuple<>(dc, dc->dock);
			// AddDrawClient in ImGuiSemiFixedDock updates dc->dock, skip here
			return retval;
		}
	}

	TZK_LOG_FORMAT(LogLevel::Error, "Draw Client with ID %s not found", drawclient_id.GetCanonical());

	return std::make_tuple<>(nullptr, WindowLocation::Invalid);
}


void
ImGuiWorkspace::UpdateWorkspaceData()
{
	my_wksp_data = my_workspace->WorkspaceData();

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
		n.second->SetStyle(GetNodeStyle(n.first->style.c_str()));

		for ( auto& p : n.second->GetPins() )
		{
			for ( auto& ps : n.first->pins )
			{
				if ( ps.id == p->GetID() )
				{
					p->SetStyle(GetPinStyle(ps.style.c_str()));
				}
			}
		}
	}
}





// to be split out to ImGuiWorkspaceNodes.cc ??


BoundaryNode::BoundaryNode(
	std::shared_ptr<graph_node_boundary> gn,
	ImGuiWorkspace* imwksp
)
: IsochroneNode(gn->id, imwksp)
, my_gn(gn)
{
	// special; boundaries always underneath other nodes
	_channel = trezanik::imgui::NodeGraphChannel_Bottom;
}


BoundaryNode::~BoundaryNode()
{
}


void
BoundaryNode::Draw()
{
	BaseNode::Draw();
}


void
BoundaryNode::DrawContent()
{
	// we use this to expand the size of the node to the desired sizing
	ImGui::Dummy(ImVec2(GetSize()));
}


bool
BoundaryNode::IsHovered()
{
	/*
	 * Intentionally not doing the hover tracking, linger state *might* be used
	 * eventually but for now that was designed with only regular nodes in mind
	 */

	return ImGui::IsMouseHoveringRect(_inner_header_rect_clipped.Min, _inner_header_rect_clipped.Max, false);
}




MultiSystemNode::MultiSystemNode(
	std::shared_ptr<graph_node_multisystem> gn,
	ImGuiWorkspace* imwksp
)
: IsochroneNode(gn->id, imwksp)
, my_gn(gn)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


MultiSystemNode::~MultiSystemNode()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
MultiSystemNode::Draw()
{
	BaseNode::Draw();
}


void
MultiSystemNode::DrawContent()
{
	ImGui::PushID(this);

	ImGui::TextDisabled("%s", my_gn->datastr.c_str());

	ImGui::PopID();
}




SystemNode::SystemNode(
	std::shared_ptr<graph_node_system> gn,
	ImGuiWorkspace* imwksp
)
: IsochroneNode(gn->id, imwksp)
, my_gn(gn)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


SystemNode::~SystemNode()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
SystemNode::Draw()
{
	BaseNode::Draw(); // invokes DrawContent
}


void
SystemNode::DrawContent()
{
	ImGui::PushID(this);

	ImGui::TextDisabled("%s", my_gn->datastr.c_str());

	ImGui::PopID();
}


std::stringstream
SystemNode::Dump() const
{
#define LOGDMP(ss, x)   (ss) << "\n" << (#x) << " = " << (x)

	std::stringstream  ss;
	ss << BaseNode::Dump().str();

	LOGDMP(ss, my_gn);
	LOGDMP(ss, _saved_node_flags);

	return ss;

#undef LOGDMP
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
	if ( other->GetAttachedNode() == node )
	{
		TZK_LOG(LogLevel::Error, "Pins cannot connect on the same node");
		return;
	}

	auto  imwksp = node->GetWorkspace();

	// create in ImGuiWorkspace data
	auto  imlink = imwksp->CreateLink(other->shared_from_this(), shared_from_this());
	// create in nodegraph, using pointers/references to the dataset
	auto  ngl = _nodegraph->CreateLink(imlink->GetID(), imlink->Source(), imlink->Target(), imlink->GetText(), imlink->GetTextOffset());

	// track the link in both source and target pins
	AssignLink(ngl);
	other->AssignLink(ngl);

	// both need notification
	othernode->NotifyListeners(imgui::NodeUpdate::LinkEstablished);
	node->NotifyListeners(imgui::NodeUpdate::LinkEstablished);
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

	/*
	 * tell workspace to create a link like it would do on file load
	 * we MUST have the text string pointer created, so the user can add and
	 * edit it dynamically
	 */
	auto  node = dynamic_cast<IsochroneNode*>(GetAttachedNode());
	assert(node != nullptr);
	if ( other->GetAttachedNode() == node )
	{
		TZK_LOG(LogLevel::Error, "Pins cannot connect on the same node");
		return;
	}

	auto  imwksp = node->GetWorkspace();
	// create in ImGuiWorkspace data
	auto  imlink = imwksp->CreateLink(other->shared_from_this(), shared_from_this());
	// create in nodegraph, using pointers/references to the dataset
	auto  ngl = _nodegraph->CreateLink(imlink->GetID(), imlink->Source(), imlink->Target(), imlink->GetText(), imlink->GetTextOffset());

	// track the link object
	AssignLink(ngl);
	// other pin needs to track it too
	other->AssignLink(ngl);

	// notification needed on other node too? don't see why it would
	node->NotifyListeners(imgui::NodeUpdate::LinkEstablished);
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
