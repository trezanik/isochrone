/**
 * @file        src/app/ImGuiWorkspace.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiWorkspace.h"
//#include "app/ImGuiWkspDefinition.h"
#include "app/ImGuiWkspForensics.h"
#include "app/ImGuiWkspSettings.h"
#include "app/ImGuiWkspTopology.h"
#include "app/Workspace.h"
#include "app/Application.h"
#include "app/AppConfigDefs.h"
#include "app/AppImGui.h"  // drawclient IDs
#include "app/TConverter.h"
#include "app/Command.h"
#include "app/tasks/Tasker.h"
#include "app/tasks/Ping.h"
#include "app/tasks/PingMonitor.h"
#include "app/event/AppEvent.h"

#include "engine/resources/ResourceCache.h"
#include "engine/services/event/EngineEvent.h"

#include "core/services/config/Config.h"
#include "core/services/event/EventDispatcher.h"
#include "core/services/memory/Memory.h"
#include "core/services/log/Log.h"
#include "core/util/net/net.h"
#include "core/util/net/net_structs.h"
#include "core/util/string/typeconv.h"
#include "core/util/time.h"
#include "core/TConverter.h"
#include "core/error.h"

#include "imgui/dear_imgui/imgui_internal.h"
#include "imgui/event/ImGuiEvent.h"
#include "imgui/CustomImGui.h"
#include "imgui/TConverter.h"

#include <algorithm>
#include <unordered_set>
#include <sstream>


namespace trezanik {
namespace app {


#if !defined(TZK_MAX_NODENAME_LENGTH)
#	define TZK_MAX_NODENAME_LENGTH  127  // 128 including nul
#endif

#if !defined(TZK_MAX_TARGET_LENGTH)
#	define TZK_MAX_TARGET_LENGTH  127  // 128 including nul
#endif



/*
 * 
 */
size_t  num_inbuilt_nodestyles = 0;
size_t  num_inbuilt_pinstyles = 0;

const char  newnode_name[] = "New Node";

const trezanik::core::UUID  drawclient_canvasdbg_uuid("9cbc06c0-c1e6-472c-a73a-1855039b1a1f");
const trezanik::core::UUID  drawclient_propview_uuid("9a663f51-9162-4bec-964e-dd5f3da2db8e");


/**
 * Private implementation of the ImGuiWorkspace class
 */
class ImGuiWorkspace::Impl
{
private:

	struct inputtext_callback_userdata
	{
		ImGuiWorkspace::Impl*  thisptr;
		std::shared_ptr<workspace_node>  node;
	};

	static int
	NodeNameEditCallbackStub(
		ImGuiInputTextCallbackData* data
	)
	{
		inputtext_callback_userdata*  dat = (inputtext_callback_userdata*)data->UserData;
		return dat->thisptr->NodeNameEditCallback(data);
	}

protected:
public:
	Impl(
		ImGuiWorkspace* wksp
	)
	: wksp(wksp)
	, wksp_data(&wksp->my_wksp_data)
	, show_node_dialog(false)
	, progress(0.f)
	, progress_colour(nullptr)
	, selected_node_target(nullptr)
	, node_name_input_buf{'\0'}
	, target_input_buf{'\0'}
	, debug_edit(false)
	, edit_current_node_name(false)
	{
		using namespace trezanik::core;

		TZK_LOG(LogLevel::Trace, "Constructor starting");
		{
			progress_colour = &wksp->_gui_interactions.active_app_style.nl_style.progress_colour1;
		}
		TZK_LOG(LogLevel::Trace, "Constructor finished");
	}


	~Impl()
	{
		using namespace trezanik::core;

		TZK_LOG(LogLevel::Trace, "Destructor starting");
		{
			task_runner.Stop();
		}
		TZK_LOG(LogLevel::Trace, "Destructor finished");
	}


	void
	CompleteNodeRename(
		std::shared_ptr<workspace_node> node
	)
	{
		using namespace trezanik::core;

		bool  was_modified = false;
		char  replacement[] = "(Unnamed)";

		if ( node->name.empty() )
		{
			was_modified = true;
			node->name = replacement;
			TZK_LOG_FORMAT(LogLevel::Warning, "Node rename changed - %s: %s", "was empty string", node->name.c_str());
		}
		if ( node->name.length() > TZK_MAX_NODENAME_LENGTH )
		{
			was_modified = true;
			node->name = node->name.substr(0, TZK_MAX_NODENAME_LENGTH);
			TZK_LOG_FORMAT(LogLevel::Warning, "Node rename changed - %s: %s", "exceeded maximum length", node->name.c_str());
		}
		bool  has_nonwhite = false;
		for ( auto& c : node->name )
		{
			// ensure not all whitespace
			if ( isblank(c) == 0 )
			{
				has_nonwhite = true;
				break;
			}
		}
		if ( !has_nonwhite )
		{
			was_modified = true;
			node->name = replacement;
			TZK_LOG_FORMAT(LogLevel::Warning, "Node rename changed - %s: %s", "only contained whitespace", node->name.c_str());
		}

		if ( !was_modified )
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "Node name edit complete: %s", node->name.c_str());
		}

		EventData::updated_node  un;
		un.node = node;
		un.workspace_id = wksp_data->id;
		ServiceLocator::EventDispatcher()->DispatchEvent(uuid_listnode_updated, un);

		edit_current_node_name = false;
	}


	/**
	 *
	 */
	void
	DrawCommonNodeList()
	{
		using namespace trezanik::core;

		progress += 0.5f * ImGui::GetIO().DeltaTime;

		/*
		 * Per-frame updates; this will be one of the first methods called for
		 * each workspace frame
		 */
		{
			if ( !unselected_lastframe.empty() )
			{
				selected_node = nullptr;
				selected_node_target = nullptr;

				/*
				 * Another ugh (written after the one below).
				 * If the node we select from the nodelist is one of the ones
				 * already selected, then it never hits these checks, so it
				 * deactivates when changing selection.
				 * 
				 * Obvious easy fix is to loop the nodes and pull out the
				 * selection each time, but I don't want to be doing that
				 * every single frame - especially when we're already keeping
				 * track, accurately, of the states everywhere.
				 * 
				 * So, as part of the unselection of the other nodes (in the
				 * applicable situation), we determine if a single one is left
				 * and if so, make it our selection. Sigh.
				 * 
				 * These two sections should barely be double-figures of LoC..
				 */
				size_t  selected = 0;
				std::shared_ptr<imgui::BaseNode>  sel;
				for ( auto& node : wksp->my_topology->my_nodegraph.GetNodes() )
				{
					if ( node->IsSelected() )
					{
						sel = node; // we only use this if there's a single one
						selected++;
					}
				}
				if ( selected < 2 && sel != nullptr ) // 0 if topology inactive, else == 1
				{
					IsochroneNode* iscn = dynamic_cast<IsochroneNode*>(sel.get());
					assert(iscn != nullptr);
					selected_node = iscn->GetWorkspaceNode();
					selected_node_target = (selected_node->selected_target == -1) ? nullptr : &selected_node->targets.at(selected_node->selected_target);
				}

				unselected_lastframe.clear();
			}
			if ( !selected_lastframe.empty() )
			{
				/*
				 * ugh.. we have to grab each nodes individual selection state for this.
				 * 
				 * Since we're at the start of the frame, the nodegraph hasn't had its
				 * Update() call invoked yet, so it still holds the previous frame data
				 * counts.
				 * We can't clear it at the end of the Update, as the count & nodes are
				 * used by popup & additional display handlers. We also can't clear it
				 * at the start, as we'll have no way of determining the real selection
				 * count!
				 * 
				 * If we support multi-selection in this list, then this issue is
				 * redundant.
				 */
				size_t  selected = 0;
				for ( auto& node : wksp->my_topology->my_nodegraph.GetNodes() )
				{
					selected += node->IsSelected() ? 1 : 0;
				}

				/*
				 * Considering click+drag multi-selection, as well as independent
				 * ctrl+click. End goal is for one nodelist selection at maximum.
				 * If no selections, or more than one, display as no selections.
				 */
				if ( selected < 2 ) // 0 if topology inactive, else == 1
				{
					selected_node = *selected_lastframe.begin();
					selected_node_target = (selected_node->selected_target == -1) ? nullptr : &selected_node->targets.at(selected_node->selected_target);
				}
				else
				{
					TZK_LOG_FORMAT(LogLevel::Trace, "%zu nodes selected, removing nodelist selection; one at maximum", wksp->my_topology->my_selected_nodes.size() + selected_lastframe.size());
					selected_node = nullptr;
					selected_node_target = nullptr;
				}

				selected_lastframe.clear();
			}

		}

		auto  wksp_settings = wksp->GetSettings();
		nodelist_style*  nl_style = wksp_settings->settings.nodelist_override_app_style
			? wksp_data->nlist_style.get()
			: &wksp->_gui_interactions.active_app_style.nl_style;
		ImVec2  nodelist_size(nl_style->node_size.x, ImGui::GetContentRegionAvail().y);
		nodelist_size.x += 45.f; // proper calc values???

		if ( !ImGui::BeginChild("NodeList", nodelist_size, ImGuiChildFlags_AlwaysUseWindowPadding, ImGuiWindowFlags_AlwaysVerticalScrollbar) )
		{
			ImGui::EndChild();
			return;
		}

		if ( ImGui::Button("DebugEdit") )
		{
			debug_edit = !debug_edit;
		}
		ImGui::SameLine();
		if ( ImGui::Button("PingMonitor (Toggle)") )
		{
			wksp->_gui_interactions.show_pingmon = !wksp->_gui_interactions.show_pingmon;
		}

		// needs cleaning effort
		if ( ImGui::Combo("Sorting", &wksp_settings->settings.nodelist_sort_order, nodelist_sortstrs) )
		{		
			wksp->ApplySetting(settingname_nodelist_sortorder, nodelist_sortstrmap[nodelist_sortstrs.at(wksp_settings->settings.nodelist_sort_order)].c_str());
		}

		if ( ImGui::Button("+ Add Node") )
		{
			// identical to ImGuiWkspTopology::DrawContextPopupNoSelect() - needs common, DRY
			auto  node = std::make_shared<workspace_node>();
			/*
			 * Unlike the topology view that has the mouse cursor position for
			 * the node location, we're creating it plain.
			 * To avoid bulk overlapping & as many conflicts as possible, have
			 * the new node height combined with the current node count to
			 * provide automatic staggering. Imperfect but very quick and will
			 * prevent most issues
			 */
			node->graph.position = ImVec2(0.f, static_cast<float>(TZK_DEFAULT_NEWNODE_HEIGHT * wksp_data->nodes.size()));
			node->graph.size.x = TZK_DEFAULT_NEWNODE_WIDTH;
			node->graph.size.y = TZK_DEFAULT_NEWNODE_HEIGHT;
			node->name = newnode_name;
			node->id.Generate();

			// common route for adding to workspace_data
			wksp->GetTopology()->AddNode(node);
		}
		ImGui::SameLine();
		bool  disable_edit = selected_node == nullptr;
		if ( disable_edit )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("# Edit Node") )
		{
			show_node_dialog = true;
		}
		if ( disable_edit )
		{
			ImGui::EndDisabled();
		}
		ImGui::SameLine();
		bool  disable_delete = selected_node == nullptr;
		if ( disable_delete )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("- Delete Node") )
		{
			// again, common route for modifications to workspace_data
			wksp->GetTopology()->RemoveNode(selected_node);

			selected_node.reset();
		}
		if ( disable_delete )
		{
			ImGui::EndDisabled();
		}

		for ( auto& n : wksp_data->nodes )
		{
			DrawListNode(n);
		}

		ImGui::EndChild();


		ImGui::SameLine();

		if ( progress > 1.0f )
		{
			progress_colour = (*progress_colour) == nl_style->progress_colour1
				? &nl_style->progress_colour2
				: &nl_style->progress_colour1;
			progress = 0.f;
		}
		if ( debug_edit )
		{
			if ( !ImGui::Begin("DefinitionDebug", &debug_edit) )
			{
				ImGui::End();
				return;
			}

			/*
			 * This is quick and nasty, not intended for production.
			 * It does allow immediate-mode temporary modification though.
			 */
			ImGui::Text("Changes here will not be saved on a workspace closure; for permanent custom styling, create and save one using the style editor");
			ImGui::InputFloat("NodeRounding", &nl_style->node_rounding);
			if ( ImGui::InputFloat2("NodeWidth", &nl_style->node_size.x, "%.0f") )
			{
				if ( nl_style->node_size.x < 10.f )
					nl_style->node_size.x = 10.f;
				else if ( nl_style->node_size.x > 1024.f )
					nl_style->node_size.x = 1024.f;
			}
			if ( ImGui::InputFloat2("NodeHeight", &nl_style->node_size.y, "%.0f") )
			{
				if ( nl_style->node_size.y < ImGui::GetTextLineHeightWithSpacing() )
					nl_style->node_size.y = ImGui::GetTextLineHeightWithSpacing();
				else if ( nl_style->node_size.y > 512.f )
					nl_style->node_size.y = 512.f;
			}
			ImGui::InputFloat("NodeInternalRightWidth", &nl_style->node_internal_rwidth);
			ImGui::ColorEdit4("NodeBackgroundColour", &nl_style->node_background_colour.x, ImGuiColorEditFlags_None);
			ImGui::ColorEdit4("NodeBackgroundColour-Selected", &nl_style->node_background_colour_selected.x, ImGuiColorEditFlags_None);
			ImGui::ColorEdit4("ProgressColour-1", &nl_style->progress_colour1.x, ImGuiColorEditFlags_NoInputs);
			ImGui::SameLine();
			ImGui::ColorEdit4("ProgressColour-2", &nl_style->progress_colour2.x, ImGuiColorEditFlags_NoInputs);

			// bool: show online indicator
			// bool: node colours follow online state
			ImGui::InputFloat("OnlineIndicator-Radius", &nl_style->online_indicator_radius);
			if ( ImGui::ColorEdit4("OnlineIndicatorColour-Down", &nl_style->online_indicator_colour_down.x, ImGuiColorEditFlags_NoInputs) )
				nl_style->u32_online_indicator_colour_down = ImGui::ColorConvertFloat4ToU32(nl_style->online_indicator_colour_down);
			if ( ImGui::ColorEdit4("OnlineIndicatorColour-Mixed", &nl_style->online_indicator_colour_mixed.x, ImGuiColorEditFlags_NoInputs) )
				nl_style->u32_online_indicator_colour_mixed = ImGui::ColorConvertFloat4ToU32(nl_style->online_indicator_colour_mixed);
			if ( ImGui::ColorEdit4("OnlineIndicatorColour-Up", &nl_style->online_indicator_colour_up.x, ImGuiColorEditFlags_NoInputs) )
				nl_style->u32_online_indicator_colour_up = ImGui::ColorConvertFloat4ToU32(nl_style->online_indicator_colour_up);

			ImGui::End();
		}
		if ( show_node_dialog && selected_node != nullptr )
		{
			ImGui::OpenPopup("Workspace Node Editor");
		}

		DrawNodeDialog(selected_node);
	}


	void
	DrawListNode(
		std::shared_ptr<workspace_node> node
	)
	{
		using namespace trezanik::core;

		const bool  is_selected_node = node == selected_node;

		nodelist_style*  nl_style = wksp->GetSettings()->settings.nodelist_override_app_style
			? wksp_data->nlist_style.get()
			: &wksp->_gui_interactions.active_app_style.nl_style;
		ImRect  node_confines;
		node_confines.Min = ImGui::GetCursorScreenPos(); // absolute co-ordinates
		node_confines.Max.x = node_confines.Min.x + nl_style->node_size.x;
		node_confines.Max.y = node_confines.Min.y + nl_style->node_size.y;

		/*
		 * These are child windows for now, but way overkill for our use case
		 * and very heavy.
		 * See about using the same nodegraph-style handling with draw lists,
		 * but this is fine for the interim with low node counts.
		 */
		ImGuiChildFlags   child_flags = ImGuiChildFlags_Borders;
		ImGuiWindowFlags  window_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, nl_style->node_rounding);

		if ( nl_style->node_bg_follows_online_status
		  && wksp->my_pingmon != nullptr
		  && node->has_component(cth_cmpt_online_track)
		  && node->pingmonitor_target_uuid != blank_uuid )
		{
			ImVec4  col = nl_style->online_indicator_colour_mixed;
			
			for ( auto& t : node->targets ) // optimize
			{
				if ( t.uuid == node->pingmonitor_target_uuid )
				{
					switch ( t.up_state )
					{
					case UpState::Down:
						col = nl_style->online_indicator_colour_down;
						break;
					case UpState::Up:
						col = nl_style->online_indicator_colour_up;
						break;
					default:
						col = nl_style->online_indicator_colour_mixed;
					}
				}
			}
			
			/// @todo add selected 'offset' colour
			ImGui::PushStyleColor(ImGuiCol_ChildBg, col);
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_ChildBg, is_selected_node ? nl_style->node_background_colour_selected : nl_style->node_background_colour);
		}
		if ( !ImGui::BeginChild(node->id.GetCanonical(), nl_style->node_size, child_flags, window_flags) )
		{
			ImGui::EndChild();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar();
			return;
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();


		float  elements_width = nl_style->node_size.x - nl_style->node_internal_rwidth;

		ImGui::PushID(node.get());
		ImGui::PushItemWidth(elements_width); // replace with spacing/padding
											  // once we have this capability, only use inputtext if editing
		ImGui::PushStyleColor(ImGuiCol_Text, nl_style->node_text_colour);
		
		if ( edit_current_node_name && is_selected_node )
		{
			inputtext_callback_userdata  udata { this, node };
			ImGuiInputTextFlags  txtinp_flags = 
				ImGuiInputTextFlags_CallbackCompletion
				| ImGuiInputTextFlags_EnterReturnsTrue;

			if ( ImGui::InputText("###Name", &node->name, txtinp_flags, &NodeNameEditCallbackStub, &udata) )
			{
				// handles return, but not tab - callback needed to handle tab too
				// null name check, share functionality
				CompleteNodeRename(node);
			}
		}
		else
		{
			ImGui::Text("%s", node->name.c_str());
		}

#if 0  // Code Disabled: quick test of status indicator
		ImDrawList*  dl = ImGui::GetWindowDrawList();
		ImVec2  newlinepos = ImGui::GetCursorScreenPos();
		ImGui::SameLine();
		ImVec2  pos_online_indicator(ImGui::GetCursorScreenPos());
		ImU32   colour = ImGui::ColorConvertFloat4ToU32(online_indicator_colour_inactive);
		// if online ? : 
		//ImGui::CollapseButton(4183184384, pos_online_indicator);
		dl->AddCircle(pos_online_indicator, indicator_radius, colour);
		//dl->AddCircleFilled(pos_online_indicator, indicator_radius, colour);
		ImGui::SetCursorPos(newlinepos);
#endif

		if ( node->targets.size() > 1 )
		{

			// 3 elements shown at a time
			ImVec2  listbox_size(elements_width, 3 * ImGui::GetTextLineHeightWithSpacing());
			if ( ImGui::BeginListBox("##NodeTargets", listbox_size) )
			{
				int   pos = -1;

				ImGui::Text("Targets:");
				// can always provide these as a tooltip, determine available space, pros/cons

				for ( auto& t : node->targets )
				{
					const bool  is_selected = (++pos == node->selected_target);

					if ( ImGui::Selectable(t.target.c_str(), is_selected) )
					{
						// since there's no trivial 'deselect', re-selection will clear
						node->selected_target = node->selected_target == pos ? -1 : pos;
					}
					if ( is_selected )
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndListBox();
			}
		}
		else
		{
			if ( node->targets.empty() )
			{
				ImGui::Text("Target: Undefined");
			}
			else
			{
				ImGui::Text("Target: %s", node->targets.cbegin()->target.c_str());
			}
		}

		if ( ImGui::IsMouseHoveringRect(node_confines.Min, node_confines.Max) )
		{
			/**
			 * @todo
			 * Add element detection (name, target, listbox target) and enable editing
			 * of the specific item, rather than forcing the name.
			 * This saves the user having to open and edit in a dialog, instead being
			 * completely dynamic in-session
			 */
			if ( ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) ) // && MousePos == NodeNameRect
			{
				edit_current_node_name = true;
				TZK_LOG(LogLevel::Debug, "Double-clicked node");

				if ( selected_node != node )
				{
					// shouldn't ever be the case.. double-click activates selection first
					TZK_LOG(LogLevel::Warning, "Double-clicked node is not the selected node; overriding assignment");
					selected_node = node;
				}
			}
			else if ( ImGui::IsMouseClicked(ImGuiMouseButton_Left) && selected_node != node )
			{
				/*
				 * This is triggered if we change nodelist node selection, but
				 * NOT if we click within the nodegraph.
				 * Unselected event handler will call this in advance if the
				 * latter occurs, so shouldn't be invoked now on a fresh nodelist
				 * selection
				 */
				if ( edit_current_node_name )
				{
					TZK_LOG(LogLevel::Info, "Node switch with incomplete edit");
					// while selected_node still the editing entry, exec validation
					CompleteNodeRename(selected_node);
				}

				/**
				 * @bug 52
				 * Do not assign the node now; due to unfortunate chaining via event
				 * management (and purely because we want this node selection updated
				 * if a new node is selected in the topology), its node update will
				 * trigger unselection of the current item - which we'll have only
				 * just replaced here.
				 * This must therefore come after the node update call, which is fine
				 * since this assignment can be pushed back to the next frame.
				 * We still need to send the event 'in advance' of the change.
				 * 
				 * Note that the topology will only do the event send back to us
				 * if it's actually open (i.e. the active tab) too! So we still need
				 * to store the change regardless of our own dispatch
				 */
				TZK_LOG_FORMAT(LogLevel::Debug, "Selected node: %s", node->name.c_str());

#if 1  // event chaining needed to keep topology and ourselves in sync, as per bug 52
				app::EventData::selected_node  sn;
				sn.node = node;
				sn.workspace_id = wksp_data->id;
				core::ServiceLocator::EventDispatcher()->DispatchEvent(uuid_listnode_selected, sn);
#endif
				selected_lastframe.insert(node);
			}
			else if ( ImGui::IsMouseClicked(ImGuiMouseButton_Right) )
			{
				// context popup

				show_node_dialog = true;
				// OpenPopup NOT suitable here, ID stack is wrong
			}

			//ImGui::TextDisabled("Hovering");
		}


		if ( 1 ) // performing task
		{
			// colour child border differently rather than consuming valuable vertical space

			ImVec4  alt_col = (*progress_colour) == nl_style->progress_colour1
				? nl_style->progress_colour2
				: nl_style->progress_colour1;

			ImGui::PushStyleColor(ImGuiCol_FrameBg, alt_col);
			ImGui::PushStyleColor(ImGuiCol_Border, alt_col);
			ImGui::PushStyleColor(ImGuiCol_PlotHistogram, *progress_colour);
			ImGui::ProgressBar(progress, ImVec2(0.0f, 2.f), "");
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
		}

		// testing for tasks
		if ( ImGui::SmallButton("Ping") )
		{
			/// @todo no functionality; t.target_is_single_item equivalent
			core::aux::ip_address  ipaddr;

			if ( core::aux::string_to_ipaddr(node->targets.front().target.c_str(), ipaddr) > 0 )
			{
				auto  pinger = std::make_shared<Ping>(ipaddr);
				task_runner.AddTask(pinger);
				task_runner.Sync();
			}
		}
		// testing for tasks
		bool  pingmon = node->has_component(cth_cmpt_online_track);
		ImGui::PushID(node.get());
		if ( ImGui::Checkbox("PingMonitor", &pingmon) )
		{
			if ( pingmon )
			{
				// start
				assert(!node->has_component(cth_cmpt_online_track));
				{
					// technically duplicate in UpdateTrackingState, can make consistent in one spot

					auto  cmpt = std::make_unique<node_component_online_tracker>();
					
					if ( node->pingmonitor_target_uuid != core::blank_uuid && wksp->my_pingmon != nullptr )
					{
						for ( auto& t : node->targets )
						{
							if ( t.uuid == node->pingmonitor_target_uuid  )
							{
								wksp->my_pingmon->AddTarget(&t);
							}
						}
					}

					cmpt->pingmon_instance = wksp->my_pingmon;
					node->components.push_back(std::move(cmpt));
				}
			}
			else
			{
				// stop
				if ( wksp->my_pingmon != nullptr && node->pingmonitor_target_uuid != blank_uuid )
				{
					if ( wksp->my_pingmon->RemoveTarget(node->pingmonitor_target_uuid) == ENOENT )
					{
					}
				}
				node->destroy_component(cth_cmpt_online_track);
			}
		}
		ImGui::PopID();

#if 0  // Code Disabled: Inline stats, not needed now we have the dedicated window
		if ( wksp->my_pingmon != nullptr && node->has_component(cth_cmpt_online_track) && node->pingmonitor_target_uuid != blank_uuid )
		{
			const auto&  stats = wksp->my_pingmon->AccessStatistics(node->pingmonitor_target_uuid);
			ImGui::Text("S/E: %zu-%zu | S/F: %zu:%zu\nLast Send: %zu\nLast Response: %zu", stats.start, stats.end,
				stats.success_count, stats.failures.size(), stats.last_send, stats.last_response
			);
		}
#endif

		ImGui::PopStyleColor();
		ImGui::PopID();
		ImGui::EndChild();
	}


	void
	DrawNodeDialog(
		std::shared_ptr<workspace_node> node
	)
	{
		using namespace trezanik::core;

		ImVec2  listbox_size(250.f, 3 * ImGui::GetTextLineHeightWithSpacing());

		ImVec2  min(100.f, 100.f);
		ImVec2  max(768.f, 512.f);
		ImGui::SetNextWindowSizeConstraints(min, max);

		// ensure OpenPopup is performed at the same ID stack level
		if ( !ImGui::BeginPopupModal("Workspace Node Editor", &show_node_dialog, ImGuiWindowFlags_AlwaysAutoResize) || node == nullptr )
		{
			// titlebar closure will not send the update event!
			return;
		}

		inputtext_callback_userdata  udata { this, node };
		ImGuiInputTextFlags  txtinp_flags = 
			ImGuiInputTextFlags_CallbackCompletion
			| ImGuiInputTextFlags_EnterReturnsTrue;

		if ( ImGui::InputText("Node Name", &node->name, txtinp_flags, &NodeNameEditCallbackStub, &udata) )
		{
			// handles return, but not tab - callback needed to handle tab too
			CompleteNodeRename(node);
		}
		ImGui::SameLine();
		ImGui::HelpMarker("Maximum 127 characters, cannot be all whitespace");

		ImGui::Separator();

		ImGui::BeginGroup();

		/// @todo add callback for validation, IP lookups. Hosts must not start with a digit.
		ImGui::InputText("Target", target_input_buf, sizeof(target_input_buf));
		ImGui::SameLine();
		ImGui::HelpMarker("One of Hostname, IP address, IP range, Subnet, e.g.\n"
			"host, 127.0.0.1, 127.0.0.1-127.0.0.128, 127.0.0.0/24"
		);

		if ( ImGui::BeginListBox("##NodeTargets", listbox_size) )
		{
			int   pos = -1;
			for ( auto& t : node->targets )
			{
				const bool  is_selected = (++pos == node->selected_target);

				if ( ImGui::Selectable(t.target.c_str(), is_selected) )
				{
					// since there's no trivial 'deselect', re-selection will clear
					if ( node->selected_target == pos )
					{
						TZK_LOG_FORMAT(LogLevel::Trace, "Unselected %s: %d (%s)", "target", pos, t.target.c_str());
						node->selected_target = -1;
						selected_node_target = nullptr;
					}
					else
					{
						TZK_LOG_FORMAT(LogLevel::Trace, "Selected %s: %d (%s)", "target", pos, t.target.c_str());
						node->selected_target = pos;
						// set: selected_node_target = (workspace_node_target*)&(*std::next(node->targets.begin(), node->selected_target));
						selected_node_target = &node->targets.at(node->selected_target);
					}
				}
				if ( is_selected )
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
		ImGui::EndGroup();
		ImGui::SameLine();
		ImGui::BeginGroup();
		bool  disable_add = target_input_buf[0] == '\0' || IsTargetDuplicate(target_input_buf, selected_node->targets); // + is all whitespace
		if ( disable_add )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("Add") )
		{
			TZK_LOG(LogLevel::Trace, "Adding target");

			workspace_node_target  tgt;
			tgt.disabled = false;
			tgt.target = target_input_buf;
			tgt.uuid.Generate();
			selected_node->targets.push_back(tgt);
			selected_node_target = &selected_node->targets.back();
		}
		if ( disable_add )
		{
			ImGui::EndDisabled();
		}
		ImGui::Separator();

		bool  disable_delete = selected_node_target == nullptr;
		if ( disable_delete )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("Delete") )
		{
			TZK_LOG(LogLevel::Trace, "Deleting target");

			auto  iter = std::find_if(selected_node->targets.begin(), selected_node->targets.end(), [this](auto&& t){
				return t.target == selected_node_target->target;
			});
			if ( iter != selected_node->targets.end() )
			{
				if ( wksp->my_pingmon != nullptr )
				{
					// if this is a live ping monitor target, remove it 
					if ( wksp->my_pingmon->TargetExists(iter->uuid) )
					{
						wksp->my_pingmon->RemoveTarget(iter->uuid);
					}
				}

				// again, if is the current, reset identifier
				if ( iter->uuid == selected_node->pingmonitor_target_uuid )
				{
					selected_node->pingmonitor_target_uuid = blank_uuid;
				}

				selected_node->targets.erase(iter);
			}
			selected_node_target = nullptr;
		}
		if ( disable_delete )
		{
			ImGui::EndDisabled();
		}

		/// @todo no target removal ability, unless we delete the target entirely
		
		auto  tgt_id = selected_node->pingmonitor_target_uuid;
		bool  disable_pmon = selected_node_target == nullptr
			|| selected_node->selected_target == -1
			|| selected_node->pingmonitor_target_uuid == selected_node->targets[selected_node->selected_target].uuid;
		if ( disable_pmon )
		{
			ImGui::BeginDisabled();
		}
		if ( ImGui::Button("Set PingMonitor target") )
		{
			TZK_LOG(LogLevel::Trace, "Assigning target");

			auto  target = selected_node->targets.at(selected_node->selected_target);

			if ( wksp->my_pingmon != nullptr )
			{
#if 0 // can't exist, these are add/delete options only in this dialog
				if ( wksp->my_pingmon->TargetExists(selected_node->pingmonitor_target_uuid) )
				{
					wksp->my_pingmon->ChangeTarget(tgt_id, &target);
				}
#endif
				if ( selected_node->has_component(cth_cmpt_online_track) )
				{
					wksp->my_pingmon->AddTarget(&target);
				}
			}

			selected_node->pingmonitor_target_uuid = target.uuid;
		}
		if ( disable_pmon )
		{
			ImGui::EndDisabled();
		}

		ImGui::EndGroup();
		// optimize!
		auto  res = std::find_if(selected_node->targets.begin(), selected_node->targets.end(), [&tgt_id](auto&& i) {
			return tgt_id == i.uuid;
		});
		if ( res != selected_node->targets.end() )
		{
			ImGui::Text("PingMonitor Target: %s (%s)", res->target.c_str(), selected_node->pingmonitor_target_uuid.GetCanonical());
		}


		if ( ImGui::Button("Close") )
		{
			show_node_dialog = false;
			ImGui::CloseCurrentPopup();

			// even if nothing changed, send out for now
			EventData::updated_node  un;
			un.node = node;
			un.workspace_id = wksp_data->id;
			ServiceLocator::EventDispatcher()->DispatchEvent(uuid_listnode_updated, un);
		}

		ImGui::EndPopup();
	}


	bool
	IsTargetDuplicate(
		const char* target,
		const std::vector<workspace_node_target>& targets
	) const
	{
		bool  case_sens = false; // we deem hostnames as not case sensitive

		for ( auto& t : targets )
		{
			if ( STR_compare(t.target.c_str(), target, case_sens) == 0 )
			{
				return true;
			}
		}

		return false;
	}


	int
	NodeNameEditCallback(
		ImGuiInputTextCallbackData* data
	)
	{
		using namespace trezanik::core;

		inputtext_callback_userdata*  udata = (inputtext_callback_userdata*)data->UserData;

		switch ( data->EventFlag )
		{
		case ImGuiInputTextFlags_CallbackCompletion:
			udata->thisptr->CompleteNodeRename(udata->node);
			break;
		default:
			break;
		}

		return 0;
	}


	/** Pointer to the ImGuiWorkspace that created us, controlling our tab display */
	ImGuiWorkspace*  wksp;

	/** Pointer to the ImGuiWorkspace workspace_data, used for ImGui operations */
	workspace_data*  wksp_data;

	bool  show_node_dialog;

	/**
	 * Progress indicator for nodes with active tasks
	 * 
	 * We share a single value across all nodes (so each will appear in sync if
	 * active), as they don't really need one each. But can be done.
	 */
	float  progress;
	/** Current progress colour. Alternates between first and second from style */
	ImVec4*  progress_colour;

	std::set<std::shared_ptr<workspace_node>>  unselected_lastframe;
	std::set<std::shared_ptr<workspace_node>>  selected_lastframe;

	/** The presently selected node in the list */
	std::shared_ptr<workspace_node>  selected_node;
	/** The target of the currently selected node */
	workspace_node_target*  selected_node_target;

	char  node_name_input_buf[TZK_MAX_NODENAME_LENGTH + 1];
	char  target_input_buf[TZK_MAX_TARGET_LENGTH + 1];

	bool  debug_edit;
	bool  edit_current_node_name;

	Tasker  task_runner;
};



ImGuiWorkspace::ImGuiWorkspace(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, my_impl({ std::make_unique<Impl>(this) })
, my_evtmgr(*core::ServiceLocator::EventDispatcher())
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		my_title = "Loading";

		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::DelayedEvent<std::shared_ptr<engine::EventData::config_change>>>(engine::uuid_configchange, std::bind(&ImGuiWorkspace::HandleConfigChange, this, std::placeholders::_1))));
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::loaded_link>>(uuid_loaded_link, std::bind(&ImGuiWorkspace::HandleLoadedLink, this, std::placeholders::_1))));
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::loaded_node>>(uuid_loaded_node, std::bind(&ImGuiWorkspace::HandleLoadedNode, this, std::placeholders::_1))));
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::loaded_nodestyle>>(uuid_loaded_nodestyle, std::bind(&ImGuiWorkspace::HandleLoadedNodeStyle, this, std::placeholders::_1))));
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::loaded_pinstyle>>(uuid_loaded_pinstyle, std::bind(&ImGuiWorkspace::HandleLoadedPinStyle, this, std::placeholders::_1))));
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::loaded_service>>(uuid_loaded_service, std::bind(&ImGuiWorkspace::HandleLoadedService, this, std::placeholders::_1))));
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::loaded_service_group>>(uuid_loaded_servicegroup, std::bind(&ImGuiWorkspace::HandleLoadedServiceGroup, this, std::placeholders::_1))));
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::loaded_setting>>(uuid_loaded_setting, std::bind(&ImGuiWorkspace::HandleLoadedSetting, this, std::placeholders::_1))));

		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::clear_selected_nodes>>(uuid_listnode_clearselected, std::bind(&ImGuiWorkspace::HandleNodelistSelectionCleared, this, std::placeholders::_1))));
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::selected_node>>(uuid_listnode_selected, std::bind(&ImGuiWorkspace::HandleNodelistSelected, this, std::placeholders::_1))));
		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::updated_node>>(uuid_listnode_updated, std::bind(&ImGuiWorkspace::HandleNodelistNodeUpdate, this, std::placeholders::_1))));

		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<app::EventData::node_target_state>>(uuid_nodetarget_state, std::bind(&ImGuiWorkspace::HandleNodeTargetState, this, std::placeholders::_1))));

		my_reg_ids.emplace(my_evtmgr.Register(std::make_shared<core::Event<imgui::EventData::node_graph_update>>(imgui::uuid_nodegraph_update, std::bind(&ImGuiWorkspace::HandleNodegraphUpdate, this, std::placeholders::_1))));
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiWorkspace::~ImGuiWorkspace()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		std::lock_guard<std::mutex>  lock(_gui_interactions.mutex);

		for ( auto& id : my_reg_ids )
		{
			my_evtmgr.Unregister(id);
		}

		//my_definition.reset();
		my_forensics.reset();
		my_topology.reset();
		my_settings.reset();

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

		my_impl.reset();
	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiWorkspace::ApplySetting(
	const char* setting_name,
	const char* setting_value,
	bool force
)
{
	using namespace trezanik::core;

	auto  common_update = [this, &setting_name, &setting_value]() {
		auto  res = my_wksp_data.settings.find(setting_name);
		if ( res != my_wksp_data.settings.end() )
		{
			/*
			 * cover imgui elements that return true by held buttons with no
			 * value changes; but will also hide initial load values
			 */
			if ( STR_compare(setting_value, res->second.c_str(), true) == 0 )
				return false;

			TZK_LOG_FORMAT(LogLevel::Debug, "%s updated: %s -> %s", setting_name, res->second.c_str(), setting_value);
		}
		else
		{
			TZK_LOG_FORMAT(LogLevel::Debug, "%s set to: %s", setting_name, setting_value);
		}
		my_wksp_data.settings[setting_name] = setting_value;
		return true;
	};
	auto  update_rgba_for = [&setting_value](ImU32& val) {
		const char*  errstr = nullptr;
		uint32_t  rgba = static_cast<uint32_t>(STR_to_unum(setting_value, UINT32_MAX, &errstr));
		if ( errstr != nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Invalid RGBA value supplied (%s), ignoring: %s", errstr, setting_value);
			return false;
		}
		val = rgba;
		return true;
	};

	switch ( TZK_RUNTIME_HASH(setting_name) )
	{
	case cth_dock_canvasdbg:
	case cth_dock_propview:
	case cth_forensics_datapath:
		break;
	case cth_grid_colour_background:
		if ( update_rgba_for(my_topology->my_nodegraph.settings.grid_style.colours.background) )
			common_update();
		break;
	case cth_grid_colour_link:
		if ( update_rgba_for(my_topology->my_nodegraph.settings.grid_style.colours.link) )
			common_update();
		break;
	case cth_grid_colour_origin:
		if ( update_rgba_for(my_topology->my_nodegraph.settings.grid_style.colours.origins) )
			common_update();
		break;
	case cth_grid_colour_primary:
		if ( update_rgba_for(my_topology->my_nodegraph.settings.grid_style.colours.primary) )
			common_update();
		break;
	case cth_grid_colour_secondary:
		if ( update_rgba_for(my_topology->my_nodegraph.settings.grid_style.colours.secondary) )
			common_update();
		break;
	case cth_grid_colour_selector:
		if ( update_rgba_for(my_topology->my_nodegraph.settings.grid_style.colours.selector_rect) )
			common_update();
		break;
	case cth_grid_draw:
		common_update();
		my_topology->my_nodegraph.settings.grid_style.draw = core::TConverter<bool>::FromString(setting_value);
		break;
	case cth_grid_draworigin:
		common_update();
		my_topology->my_nodegraph.settings.grid_style.draw_origin = core::TConverter<bool>::FromString(setting_value);
		break;
	case cth_grid_size:
		{
			uint32_t  i = core::TConverter<uint32_t>::FromString(setting_value);
			if ( i == 0 ) // conversion failed
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Invalid %s supplied, ignoring: %s", "integer", setting_value);
				return;
			}
			if ( i % 10 != 0 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Invalid %s supplied, ignoring: %s", "size", setting_value);
				return;
			}
			common_update();
			my_topology->my_nodegraph.settings.grid_style.size = static_cast<int>(i);
		}
		break;
	case cth_grid_subdivisions:
		{
			uint8_t  i = core::TConverter<uint8_t>::FromString(setting_value);
			if ( i == 0 ) // conversion failed
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Invalid %s supplied, ignoring: %s", "integer", setting_value);
				return;
			}
			if ( i != 1 && i != 2 && i != 5 && i != 10 )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Invalid %s supplied, ignoring: %s", "subdivisions", setting_value);
				return;
			}
			common_update();
			my_topology->my_nodegraph.settings.grid_style.subdivisions = static_cast<int>(i);
		}
		break;
	case cth_link_defaultmethod:
		{
			auto  link_method = imgui::TConverter<imgui::LinkMethod>::FromString(setting_value);
			if ( link_method == imgui::LinkMethod::Invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Invalid %s supplied, ignoring: %s", "LinkMethod", setting_value);
				return;
			}
			common_update();
			my_topology->my_nodegraph.settings.link_default_method = static_cast<int>(link_method);
		}
		break;
	case cth_node_dragfromheadersonly:
		common_update();
		my_topology->my_nodegraph.settings.node_drag_from_headers_only = core::TConverter<bool>::FromString(setting_value);
		break;
	case cth_node_drawheaders:
		common_update();
		my_topology->my_nodegraph.settings.node_draw_headers = core::TConverter<bool>::FromString(setting_value);
		break;
	case cth_node_trackonlinestate:
		common_update();
		my_topology->settings.node_track_online_state = core::TConverter<bool>::FromString(setting_value);
		UpdateTrackingState(my_topology->settings.node_track_online_state);
		break;
	case cth_nodelist_overrideappstyle:
		{
			// boolean has no TConverter entry for integer mapping to determine invalidity, unless we redefine here..
			common_update();
			// does log warning on conversion failure, and if so returns false
			my_settings->settings.nodelist_override_app_style = core::TConverter<bool>::FromString(setting_value);
		
			if ( my_settings->settings.nodelist_override_app_style )
			{
				// apply the custom style
				_gui_interactions.active_app_style.nl_style = *my_wksp_data.nlist_style;
			}
			else
			{
				// revert style to prior
				for ( auto& ast : _gui_interactions.app_styles )
				{
					if ( ast->name == _gui_interactions.active_app_style.name )
					{
						_gui_interactions.active_app_style.nl_style = ast->nl_style;
						break;
					}
				}
			}
		}
		break;
	case cth_nodelist_sortorder:
		{
			auto  sort_method = TConverter<SortNodeMethod>::FromString(setting_value);
			if ( sort_method == SortNodeMethod::Invalid )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Invalid SortNodeMethod supplied, ignoring: %s", setting_value);
				return;
			}
			if ( common_update() || force )
			{
				// update actual variable used
				my_settings->settings.nodelist_sort_order = static_cast<int>(sort_method);
				// immediate live reflection needs additional invocation
				my_topology->SortNodes();
			}
		}
		break;
	default:
		TZK_LOG_FORMAT(LogLevel::Warning, "Not implemented or not found for: %s (%u)", setting_name, TZK_RUNTIME_HASH(setting_name));
		break;
	}
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
ImGuiWorkspace::Draw()
{
	using namespace trezanik::core;

	//ImGuiWindowFlags_UnsavedDocument = my_wksp_data != my_wksp->wksp_data
	ImGuiWindowFlags  wnd_flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
	ImVec2  min_size(360.f, 240.f);

	ImGui::SetNextWindowPos(_gui_interactions.workspace_pos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(_gui_interactions.workspace_size, ImGuiCond_Always);
	ImGui::SetNextWindowSizeConstraints(min_size, ImVec2(FLT_MAX, FLT_MAX));

	if ( !ImGui::Begin(my_title.c_str(), nullptr, wnd_flags) )
	{
		ImGui::End();
		return;
	}
		
	if ( my_workspace == nullptr || my_hold_on_loading )
	{
		// unlikely to ever be seen unless you have a fault, rather large file or slow machine!
		DrawLoadingDetails();
		ImGui::End();
		return;
	}

	// only created on workspace assignment, must never reach here until then
	assert(my_topology != nullptr);

	/*
	 * Handle save requests.
	 * Must be done here so we can feed back the new workspace data (that this
	 * class and its children perform via all UI interactions) into the workspace
	 * object itself, which is what is used for the saving.
	 * 
	 * Requires us to validate we're the active workspace, unless saving all is
	 * requested (not yet implemented). Suspect the Begin() call with return
	 * before this is hit making it safe for active check, but not tested - so
	 * likely needs moving above if doing a save-all
	 */
	if ( _gui_interactions.active_workspace == my_workspace->GetID() )
	{
		if ( _gui_interactions.save_current_workspace )
		{
			int  rc;
			if ( (rc = my_workspace->Save(my_workspace->GetPath(), &my_wksp_data)) != ErrNONE )
			{
				// error already notified
			}
			_gui_interactions.save_current_workspace = false;
		}
	}


	// draw node list within dedicated window confines
	my_impl->DrawCommonNodeList();

	// ensure adjacent consuming remaining space, not underneath the node list
	ImGui::SameLine();

	/// @todo no hscroll if child content doesn't fit, vscroll is automatic
	if ( !ImGui::BeginChild("TabContainer") )
	{
		ImGui::EndChild();
		ImGui::End();
		return;
	}

	_gui_interactions.tabchild_pos = ImGui::GetCursorPos();
	_gui_interactions.tabchild_size = ImGui::GetContentRegionAvail();
	_gui_interactions.tabchild_rect = { _gui_interactions.tabchild_pos, _gui_interactions.tabchild_size };


	/*
	 * We want the common node list drawn first, whose size calculations then
	 * determine what's left and available for each tab - including the starting
	 * position.
	 */
	if ( ImGui::BeginTabBar("WorkspaceTabs") )
	{
		if ( ImGui::BeginTabItem("Forensics##") )
		{
			my_forensics->Draw();
			ImGui::EndTabItem();
		}
		if ( ImGui::BeginTabItem("Topology##") )
		{
			my_topology->Draw();
			ImGui::EndTabItem();
		}
		if ( ImGui::BeginTabItem("Settings##") )
		{
			my_settings->Draw();
			ImGui::EndTabItem();
		}
#if 0  // Code Disabled: To add in future
		if ( selected_nodes == 1 && ImGui::BeginTabItem("SelectedNode##") )
		{
			my_sel_node->Draw();
			ImGui::EndTabItem();
		}
#endif

		ImGui::EndTabBar();
	}

	ImGui::EndChild();
	ImGui::End();
}


void
ImGuiWorkspace::DrawLoadingDetails()
{
	// tabular

	ImGui::BeginGroup();
	ImGui::PushFont(_gui_interactions.font_fixed_width, _gui_interactions.font_fixed_width_size);
	ImGui::Text("Nodes...........:");  ImGui::SameLine();  ImGui::Text("%zu", my_loading_wksp_data.nodes.size());
	ImGui::Text("Links...........:");  ImGui::SameLine();  ImGui::Text("%zu", my_loading_wksp_data.links.size());
	ImGui::Text("Node Styles.....:");  ImGui::SameLine();  ImGui::Text("%zu", my_loading_wksp_data.node_styles.size() - num_inbuilt_nodestyles);
	ImGui::Text("Pin Styles......:");  ImGui::SameLine();  ImGui::Text("%zu", my_loading_wksp_data.pin_styles.size() - num_inbuilt_pinstyles);
	ImGui::Text("Services........:");  ImGui::SameLine();  ImGui::Text("%zu", my_loading_wksp_data.services.size());
	ImGui::Text("Service Groups..:");  ImGui::SameLine();  ImGui::Text("%zu", my_loading_wksp_data.service_groups.size());
	ImGui::Text("Settings........:");  ImGui::SameLine();  ImGui::Text("%zu", my_loading_wksp_data.settings.size());
	ImGui::PopFont();
	ImGui::EndGroup();

	// would be great to add warnings/errors here - but that means events for each??

	ImGui::SameLine();
	if ( ImGui::Button("Close") )
	{
		my_hold_on_loading = false;
	}

	ImGui::Separator();

	// constraints, default size
	ImGuiWindowFlags  wnd_flags =
		ImGuiWindowFlags_HorizontalScrollbar |
		ImGuiWindowFlags_AlwaysVerticalScrollbar;
	ImVec2  subwnd_size(ImGui::GetContentRegionAvail());

	ImGui::SetNextWindowSize(subwnd_size, ImGuiCond_Always);

	if ( !ImGui::BeginChild("LoadOutput", subwnd_size, false, wnd_flags) )
	{
		ImGui::EndChild();
		return;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));
	ImGui::PushFont(_gui_interactions.font_fixed_width, _gui_interactions.font_fixed_width_size);

	{
		std::lock_guard<std::mutex>  lock(my_loading_entries_mutex);

		for ( auto& entry : my_loading_entries )
		{
			// colour based on type/good/bad state
			//ImGui::PushStyleColor(ImGuiCol_Text, colour);
			ImGui::TextUnformatted(entry.c_str());
			//ImGui::PopStyleColor();
		}
	}

	ImGui::PopFont();

	if ( ImGui::GetScrollY() >= ImGui::GetScrollMaxY() )
	{
		ImGui::SetScrollHereY(1.0f);
	}

	ImGui::PopStyleVar();
	ImGui::EndChild();
}


std::shared_ptr<workspace_node>
ImGuiWorkspace::GetSelectedNode()
{
	return my_impl->selected_node;
}


Tasker*
ImGuiWorkspace::GetTasker() const
{
	return &my_impl->task_runner;
}


std::shared_ptr<Workspace>
ImGuiWorkspace::GetWorkspace()
{
	return my_workspace;
}


void
ImGuiWorkspace::HandleConfigChange(
	std::shared_ptr<trezanik::engine::EventData::config_change> cc
)
{
	using namespace trezanik::core;

	if ( cc->new_config.count(TZK_CVAR_SETTING_UI_STYLE_NAME) > 0 )
	{
#if 0 // no-op needed anymore
		for ( auto& ast : _gui_interactions.app_styles )
		{
			if ( ast->name == _gui_interactions.active_app_style_name )
			{
				TZK_LOG_FORMAT(LogLevel::Debug, "Updating implementation style to '%s'", ast->name.c_str());
				my_impl->nl_style = &ast->nl_style;
				break;
			}
		}
#endif
	}
}


void
ImGuiWorkspace::HandleLoadedNode(
	app::EventData::loaded_node loaded
)
{
	if ( my_workspace != nullptr )
	{
		return;
	}

	my_loading_wksp_data.nodes.push_back(loaded.node);

	std::stringstream  linf;
	char  indent[] = "  ";
	char  tbuf[32] = { '\0' };
	
	core::aux::get_time_format(loaded.node->added, tbuf, sizeof(tbuf), "%F %T");

	linf << "Node loaded: " << loaded.node->id.GetCanonical() << " '" << loaded.node->name << "'";
	linf << "\n" << indent << "Added: " << loaded.node->added << " (" << tbuf << ")";
	linf << "\n" << indent << "Components: " << loaded.node->components.size();
	linf << "\n" << indent << "Pins: " << loaded.node->graph.pins.size();
	linf << "\n" << indent << "Style: " << loaded.node->graph.style;
	linf << "\n" << indent << "Position: " << loaded.node->graph.position.x << "," << loaded.node->graph.position.y;
	linf << "\n" << indent << "Size: " << loaded.node->graph.size.x << "," << loaded.node->graph.size.y;
	linf << "\n" << indent << "Data Length: " << loaded.node->graph.datastr.length();

	std::lock_guard<std::mutex>  lock(my_loading_entries_mutex);
	my_loading_entries.emplace_back(linf.str());
}


void
ImGuiWorkspace::HandleLoadedLink(
	app::EventData::loaded_link loaded
)
{
	if ( my_workspace != nullptr )
	{
		// not for this workspace instance
		// no ID to check against at this stage either
		return;
	}

	my_loading_wksp_data.links.emplace(loaded.lnk);

	std::stringstream  linf;
	char  indent[] = "  ";

	linf << "Link loaded: " << loaded.lnk->id.GetCanonical();
	linf << "\n" << indent << "Source: " << loaded.lnk->source.GetCanonical();
	linf << "\n" << indent << "Target: " << loaded.lnk->target.GetCanonical();
	linf << "\n" << indent << "Text: " << loaded.lnk->text;
	linf << "\n" << indent << "Text Offset: " << loaded.lnk->offset.x << "," << loaded.lnk->offset.y;
	linf << "\n" << indent << "Method: " << static_cast<int>(loaded.lnk->method);

	std::lock_guard<std::mutex>  lock(my_loading_entries_mutex);
	my_loading_entries.emplace_back(linf.str());
}


void
ImGuiWorkspace::HandleLoadedNodeStyle(
	app::EventData::loaded_nodestyle loaded
)
{
	if ( my_workspace != nullptr )
	{
		return;
	}

	my_loading_wksp_data.node_styles.emplace_back(std::make_pair<>(loaded.name, loaded.style));

	std::stringstream  linf;
	//char  indent[] = "  ";

	linf << "Node Style loaded: " << loaded.name;
	// really don't want to do all members...

	std::lock_guard<std::mutex>  lock(my_loading_entries_mutex);
	my_loading_entries.emplace_back(linf.str());
}


void
ImGuiWorkspace::HandleLoadedPinStyle(
	app::EventData::loaded_pinstyle loaded
)
{
	if ( my_workspace != nullptr )
	{
		return;
	}

	my_loading_wksp_data.pin_styles.emplace_back(std::make_pair<>(loaded.name, loaded.style));

	std::stringstream  linf;
	//char  indent[] = "  ";

	linf << "Pin Style loaded: " << loaded.name;
	// really don't want to do all members...

	std::lock_guard<std::mutex>  lock(my_loading_entries_mutex);
	my_loading_entries.emplace_back(linf.str());
}


void
ImGuiWorkspace::HandleLoadedService(
	app::EventData::loaded_service loaded
)
{
	if ( my_workspace != nullptr )
	{
		return;
	}

	my_loading_wksp_data.services.emplace_back(loaded.svc);

	std::stringstream  linf;
	char  indent[] = "  ";

	linf << "Service loaded: " << loaded.svc->name;
	linf << "\n" << indent << "Comment: " << loaded.svc->comment;
	linf << "\n" << indent << "Protocol: " << loaded.svc->protocol;
	linf << "\n" << indent << "Port: " << loaded.svc->port;
	linf << "\n" << indent << "High-Port: " << loaded.svc->high_port;

	std::lock_guard<std::mutex>  lock(my_loading_entries_mutex);
	my_loading_entries.emplace_back(linf.str());
}


void
ImGuiWorkspace::HandleLoadedServiceGroup(
	app::EventData::loaded_service_group loaded
)
{
	if ( my_workspace != nullptr )
	{
		return;
	}

	my_loading_wksp_data.service_groups.emplace_back(loaded.svcgrp);

	std::stringstream  linf;
	char  indent[] = "  ";

	linf << "Service Group loaded: " << loaded.svcgrp->name;
	linf << "\n" << indent << "Comment: " << loaded.svcgrp->comment;
	linf << "\n" << indent << "Services: ";
	for ( auto& svc : loaded.svcgrp->services )
		linf << svc << ", ";

	std::string  str = linf.str();

	if ( !loaded.svcgrp->services.empty() )
	{
		// remove excess space and comma
		str.pop_back();
		str.pop_back();
	}

	std::lock_guard<std::mutex>  lock(my_loading_entries_mutex);
	my_loading_entries.emplace_back(str);
}


void
ImGuiWorkspace::HandleLoadedSetting(
	app::EventData::loaded_setting loaded
)
{
	if ( my_workspace != nullptr )
	{
		return;
	}

	my_loading_wksp_data.settings[loaded.name] = loaded.value;

	std::stringstream  linf;
	char  indent[] = "  ";

	linf << "Setting loaded: " << loaded.name;
	linf << "\n" << indent << "Value: " << loaded.value;

	std::lock_guard<std::mutex>  lock(my_loading_entries_mutex);
	my_loading_entries.emplace_back(linf.str());
}


void
ImGuiWorkspace::HandleNodegraphUpdate(
	trezanik::imgui::EventData::node_graph_update update
)
{
	using namespace trezanik::core;
	using namespace trezanik::imgui;

	/*
	 * Not interested in updates to the nodegraph until AFTER the initial load
	 * of data and object creations.
	 */
	if ( my_workspace == nullptr || my_topology == nullptr || update.node_graph != &my_topology->my_nodegraph )
	{
		return;
	}

#if TZK_IS_DEBUG_BUILD
	update.validate();
#endif

	TZK_LOG_FORMAT(LogLevel::Trace,
		"Nodegraph Update: %u:%s (node=%s, pin=%s, link=%s, vec2=(%.9g,%.9g))",
		static_cast<uint16_t>(update.update), imgui::TConverter<NodeGraphUpdate>::ToString(update.update).c_str(),
		update.opt.node_uuid == blank_uuid ? "none" : update.opt.node_uuid.GetCanonical(),
		update.opt.pin_uuid == blank_uuid ? "none" : update.opt.pin_uuid.GetCanonical(),
		update.opt.link_uuid == blank_uuid ? "none" : update.opt.link_uuid.GetCanonical(),
		update.opt.vec2.x, update.opt.vec2.y
	);

	std::shared_ptr<IsochroneNode>   iso_node;

	if ( update.opt.node_uuid != blank_uuid )
	{
		for ( auto& n : my_topology->my_nodes )
		{
			if ( n->GetID() == update.opt.node_uuid )
			{
				iso_node = n;
				break;
			}
		}
		if ( iso_node == nullptr )
		{
			TZK_LOG_FORMAT(LogLevel::Warning, "Couldn't find node associated with update: %s", update.opt.node_uuid.GetCanonical());
			return;
		}
	}

	/*
	 * Reiterating here:
	 * Application objects (app::graph_node, app::pin, app:link) are always created FIRST.
	 * They contain the raw underlying data that is written back to disk. The imgui::
	 * equivalents only hold pointers to this data.
	 * 
	 * When it comes to deletions however, all the interaction is done from the imgui side.
	 * So those are deleted, and THEN the evt dispatch needs to be performed, and the app
	 * handles those actions, updating the associated data - right here and now.
	 * 
	 * NodeGraphUpdates therefore NEVER have NodeCreated/LinkCreated/PinCreated.
	 * Triggers for those may come from imgui (and may not), but the app handler handles
	 * this and the addition to imgui presentation. This is good as it supports
	 * modifications from the command line, undo/redo functionality, etc.
	 */
	switch ( update.update )
	{
	case NodeGraphUpdate::LinkAssigned:
		// no-op
		break;
	case NodeGraphUpdate::LinkDeleted:
		{
			auto&  links = my_wksp_data.links;
			auto res = std::find_if(links.begin(), links.end(), [&update](auto&& l){
				return l->id == update.opt.link_uuid;
			});
			if ( res == links.end() )
			{
				return;
			}
			links.erase(res);
		}
		break;
	case NodeGraphUpdate::LinkUnassigned:
		// no-op
		break;
	case NodeGraphUpdate::NodeData:
		break;
	case NodeGraphUpdate::NodeDeleting:
		{
			auto&  nodes = my_wksp_data.nodes;
			auto res = std::find_if(nodes.begin(), nodes.end(), [&update](auto&& n){
				return n->id == update.opt.node_uuid;
			});
			if ( res == nodes.end() )
			{
				return;
			}

			if ( (*res)->has_component(cth_cmpt_online_track) )
			{
				if ( my_pingmon->TargetExists((*res)->pingmonitor_target_uuid) )
				{
					my_pingmon->RemoveTarget((*res)->pingmonitor_target_uuid);
				}
				(*res)->destroy_component(cth_cmpt_online_track);
			}

			// special handling for links, so we avoid warnings (harmless, but indicates fault)
			for ( auto& p : (*res)->graph.pins )
			{
				for ( ;; )
				{
					bool  any_found = false;

					for ( auto& link : my_wksp_data.links )
					{
						bool  is_source = link->source == p.id;
						bool  is_target = link->target == p.id;
						if ( is_source || is_target )
						{
							TZK_LOG_FORMAT(LogLevel::Debug, "Node %s has live link: %s", (*res)->id.GetCanonical(), link->id.GetCanonical());
							/*
							 * note: each of these will reinvoke this handler, if you
							 * have an excessive number of live links it could lead to
							 * a crash due to stack consumption
							 */
							my_topology->BreakLink(link->id);

							any_found = true;
							// iterator is invalid, deleted in event handler
							break;
						}
					}
					// break endless loop once no more are found
					if ( !any_found )
						break;
				}
			}

			// remove from grid and internal state
			my_topology->RemoveNode((*res));

			// we remove from the workspace data, nothing else
			nodes.erase(res);
		}
		break;
	case NodeGraphUpdate::NodeName:
		// no-op; is pointer to data, direct modification
		break;
	case NodeGraphUpdate::NodePosition:
		{
			//could also do: auto  pos = iso_node->GetPosition();
			iso_node->GetWorkspaceNode()->graph.position = update.opt.vec2;
		}
		break;
	case NodeGraphUpdate::NodeSelected:
		{
			// activate selection in nodelist

			auto  wn = iso_node->GetWorkspaceNode();
			my_impl->selected_lastframe.insert(wn);
		}
		break;
	case NodeGraphUpdate::NodeSize:
		{
			//could also do: auto  pos = iso_node->GetSize();
			iso_node->GetWorkspaceNode()->graph.size = update.opt.vec2;
		}
		break;
	case NodeGraphUpdate::NodeStyle:
		{
			// yeah, names in the objects seems much more intuitive to me - easier here too
			for ( auto& obj : my_wksp_data.node_styles )
			{
				if ( obj.second != update.opt.node_style )
					continue;
				
				// find object, extract sibling name
				std::string&  ref = obj.first;

				for ( auto& n : my_wksp_data.nodes )
				{
					if ( n->id != update.opt.node_uuid )
						continue;
					
					TZK_LOG_FORMAT(LogLevel::Trace, "Node %s style updated from %s -> %s",
						n->id.GetCanonical(),
						n->graph.style.c_str(), ref.c_str()
					);
					// assign name to node
					n->graph.style = ref;
					break;
				}
				break;
			}
		}
		break;
	case NodeGraphUpdate::NodeUnselected:
		{
			// deactivate selection in nodelist
			auto  wn = iso_node->GetWorkspaceNode();
			my_impl->unselected_lastframe.insert(wn);

			if ( my_impl->edit_current_node_name )
			{
				/*
				 * Node was being renamed when we clicked into the nodegraph,
				 * call completion so flag is unset.
				 * Selecting from the node list is already handled directly
				 */
				my_impl->CompleteNodeRename(wn);
			}
		}
		break;
	case NodeGraphUpdate::PinDeleted:
		{
			assert(update.opt.pin_uuid != blank_uuid);

			auto  oldc = iso_node->GetWorkspaceNode()->graph.pins.size();
			
			for ( auto iter = iso_node->GetWorkspaceNode()->graph.pins.begin(); iter != iso_node->GetWorkspaceNode()->graph.pins.end(); iter++ )
			{
				if ( iter->id != update.opt.pin_uuid )
				{
					continue;
				}

				// pin found in the graph data; check to make sure it's the one gone from live state
				bool  found = false;

				// this list will not have the deleted entry
				for ( auto& p : iso_node->GetPins() )
				{
					if ( iter->id == p->GetID() )
					{
						found = true;
						break;
					}
				}

				if ( found )
				{
					TZK_LOG_FORMAT(LogLevel::Warning, "Pin ID %s notified of deletion, but was found held within node", iter->id.GetCanonical());
					TZK_LOG_FORMAT(LogLevel::Error, "Potential data corruption or loss if file is saved, deleted pin is still retained");
				}
				else
				{
					iso_node->GetWorkspaceNode()->graph.pins.erase(iter);
					TZK_LOG(LogLevel::Trace, "Removed pin from workspace data"); // improve
					break;
				}
			}

			auto  newc = iso_node->GetWorkspaceNode()->graph.pins.size();

			if ( newc + 1 != oldc )
			{
				TZK_LOG_FORMAT(LogLevel::Warning, "Expecting new pin count of %zu, is instead %zu", oldc - 1, newc);
			}
		}
		break;
	case NodeGraphUpdate::PinPosition:
		{
			// not yet implemented
		}
		break;
	case NodeGraphUpdate::PinStyle:
		{
			// mirror NodeStyle update
			for ( auto& obj : my_wksp_data.pin_styles )
			{
				if ( obj.second != update.opt.pin_style )
					continue;
				
				std::string&  ref = obj.first;

				for ( auto& n : my_wksp_data.nodes )
				{
					if ( n->id != update.opt.node_uuid )
						continue;

					for ( auto& p : n->graph.pins )
					{
						if ( p.id != update.opt.pin_uuid )
							continue;
						
						TZK_LOG_FORMAT(LogLevel::Debug, "Trace %s style updated from %s -> %s",
							p.id.GetCanonical(),
							p.style.c_str(), ref.c_str()
						);
						p.style = ref;
						break;
					}
					break;
				}
				break;
			}
		}
		break;
	default:
		break;
	}
}


void
ImGuiWorkspace::HandleNodelistNodeUpdate(
	app::EventData::updated_node updated
)
{
	if ( updated.workspace_id != my_wksp_data.id )
	{
		return;
	}

	my_topology->SortNodes();
}


void
ImGuiWorkspace::HandleNodelistSelected(
	app::EventData::selected_node selected
)
{
	if ( selected.workspace_id != my_wksp_data.id )
	{
		return;
	}

	/*
	 * the old node selection is still set until the next frame within our
	 * implementation - ensure the parameter node is used for operations
	 */

	/*
	 * Find the node in the topology, and set it as selected.
	 * Must also clear any other selection in the nodegraph.
	 * 
	 * Could make this an invokable method within Topology
	 */
	for ( auto& n : my_topology->my_nodes )
	{
		n->Selected(n->GetID() == selected.node->id);
	}
}


void
ImGuiWorkspace::HandleNodelistSelectionCleared(
	app::EventData::clear_selected_nodes cleared
)
{
	if ( cleared.workspace_id != my_wksp_data.id )
	{
		return;
	}

	/*
	 * the old node selection is still set until the next frame within our
	 * implementation - ensure the parameter node is used for operations
	 */

	// to decide if we feed in to anything else
}


void
ImGuiWorkspace::HandleNodeTargetState(
	app::EventData::node_target_state state
)
{
	/*
	 * no workspace_id check, relies on UUIDs being unique across multiple
	 * workspaces; easy to add, but means redundant wksp id addition to a node
	 * target (which is only held within a node beyond this 'feature')
	 */
	for ( auto& n : my_wksp_data.nodes )
	{
		for ( auto& t : n->targets )
		{
			if ( t.uuid == state.target_id )
			{
				t.up_state = state.up_state;
				return;
			}
		}
	}
}


void
ImGuiWorkspace::UpdateTrackingState(
	bool enabled
)
{
	using namespace trezanik::core;

	if ( enabled )
	{
		if ( my_pingmon == nullptr )
		{
			icmp_echo_monitor_config  config;
			// apply custom config

			my_pingmon = std::make_shared<PingMonitor>(config);

			// add every node with online track & a valid target
			for ( auto& n : my_wksp_data.nodes )
			{
				if ( !n->has_component(cth_cmpt_online_track) )
				{
#if 0  // Code Disabled: allows auto-addition of components to all nodes for tracking in bulk
					auto  ncmpt = std::make_unique<node_component_online_tracker>();
					n->components.push_back(std::move(ncmpt));
#else
					// if no component, user doesn't want tracking
					continue;
#endif
				}
				
				// no ping monitor target set
				if ( n->pingmonitor_target_uuid == core::blank_uuid )
					continue;

				/*
				 * allow pingmonitor window to identify the pingmonitor instance;
				 * this can be omitted if it gets obtained via an alternative
				 * means
				 */
				auto  cmpt = dynamic_cast<node_component_online_tracker*>(n->get_component(cth_cmpt_online_track));
				if ( cmpt != nullptr )
				{
					cmpt->pingmon_instance = my_pingmon;
				}

				for ( auto& t : n->targets )
				{
					if ( n->pingmonitor_target_uuid == t.uuid )
					{
						my_pingmon->AddTarget(&t);
						break;
					}
				}
			}
		}

		my_impl->task_runner.AddTask(my_pingmon);
		my_impl->task_runner.Sync();
	}
	else
	{
		for ( auto& n : my_wksp_data.nodes )
		{
			if ( !n->has_component(cth_cmpt_online_track) )
			{
				continue;
			}

			auto  cmpt = dynamic_cast<node_component_online_tracker*>(n->get_component(cth_cmpt_online_track));
			if ( cmpt != nullptr )
			{
				cmpt->pingmon_instance.reset();
			}
		}

		my_impl->task_runner.StopTask(my_pingmon);
		// drop our reference, task runner should hold remainder and auto-delete it
		my_pingmon.reset();
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
	
	// acquire data copy used for all operations moving forward
	my_wksp_data = my_workspace->WorkspaceData();

	my_title = my_workspace->GetName();
	// use UUID as the only imgui ID for this window (dear imgui #251)
	my_title += "###";
	my_title += my_wksp_data.id.GetCanonical();

	// sanity check
	if ( my_wksp_data.links.size() != my_loading_wksp_data.links.size()
	  || my_wksp_data.nodes.size() != my_loading_wksp_data.nodes.size()
	  || my_wksp_data.node_styles.size() != (my_loading_wksp_data.node_styles.size() + num_inbuilt_nodestyles)
	  || my_wksp_data.pin_styles.size() != (my_loading_wksp_data.pin_styles.size() + num_inbuilt_pinstyles)
	  || my_wksp_data.services.size() != my_loading_wksp_data.services.size()
	  || my_wksp_data.service_groups.size() != my_loading_wksp_data.service_groups.size()
	  || my_wksp_data.settings.size() != my_loading_wksp_data.settings.size()
	  // additional as needed
	)
	{
		TZK_LOG(LogLevel::Error, "Data mismatch in loading vs live; missing content");
	}

	// replace interim loading data
	my_loading_wksp_data = my_wksp_data;

	// create subclasses that interact with the data; are also extra tab navigations
	//my_definition = std::make_unique<ImGuiWkspDefinition>(_gui_interactions, this);
	my_forensics = std::make_unique<ImGuiWkspForensics>(_gui_interactions, this);
	my_settings  = std::make_unique<ImGuiWkspSettings>(_gui_interactions, this);
	my_topology  = std::make_unique<ImGuiWkspTopology>(_gui_interactions, this);

	// settings application requires all other tabs/dependents loaded first
	bool  force = true;
	for ( auto& setting : my_wksp_data.settings )
	{
		ApplySetting(setting.first.c_str(), setting.second.c_str(), force);
	}

	my_topology->SortNodes();

	/*
	 * any modifications within the nodegraph (those that dispatch events) will
	 * now be handled by this class, and can be used for modification detection
	 */

	return retval;
}


std::tuple<std::shared_ptr<DrawClient>, WindowLocation>
ImGuiWorkspace::UpdateDrawClientDockLocation(
	const trezanik::core::UUID& drawclient_id,
	WindowLocation newloc
)
{
	using namespace trezanik::core;

	/// @todo map lookup for future expansion without touching this method
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
	/*
	 * caution: client children have pointers to this
	 * Their pointers remain valid but the data could change mid-iteration unless
	 * this is only actioned at deemed safe points
	 */
	my_wksp_data = my_workspace->GetWorkspaceData();

	// so many ways of doing this.. for now, lazy update until we know the full chain
	
	//my_forensics->UpdateWorkspaceData();
	//my_settings->UpdateWorkspaceData();
	my_topology->UpdateWorkspaceData();
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI
