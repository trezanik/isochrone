/**
 * @file        src/app/ImGuiWkspSettings.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiWkspSettings.h"
#include "app/ImGuiWkspForensics.h" // settings access only
#include "app/ImGuiWkspTopology.h" // settings access only (+sorting without refactor)
#include "app/ImGuiWorkspace.h"
#include "app/TConverter.h"

#include "imgui/CustomImGui.h"
#include "imgui/TConverter.h"

#include "core/services/log/Log.h"
#include "core/TConverter.h"


namespace trezanik {
namespace app {


ImGuiWkspSettings::ImGuiWkspSettings(
	GuiInteractions& gui_interactions,
	ImGuiWorkspace* wksp
)
: IImGui(gui_interactions)
, my_wksp(wksp)
, my_wksp_data(&wksp->my_wksp_data)
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiWkspSettings::~ImGuiWkspSettings()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{

	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiWkspSettings::Draw()
{
	using namespace trezanik::core;

	ImGuiTableFlags  table_flags = ImGuiTableFlags_NoSavedSettings
		| ImGuiTableFlags_SizingFixedFit
		| ImGuiTableFlags_BordersV
		| ImGuiTableFlags_BordersOuterH
		| ImGuiTableFlags_Resizable
		| ImGuiTableFlags_RowBg
		| ImGuiTableFlags_NoBordersInBody;
	ImGuiTreeNodeFlags  tree_node_flags = ImGuiTreeNodeFlags_SpanAllColumns
		| ImGuiTreeNodeFlags_AllowOverlap // table row selected otherwise, not the cell widget
		| ImGuiTreeNodeFlags_Leaf
		| ImGuiTreeNodeFlags_FramePadding
		| ImGuiTreeNodeFlags_NoTreePushOnOpen;
		// | ImGuiTreeNodeFlags_DefaultOpen // yay or nay?

	char  column1[] = "Setting";
	char  column2[] = "Value";
	int   num_columns = 2;

	auto   topology = my_wksp->GetTopology();
	
	if ( topology == nullptr )
	{
		return;
	}


	enum class OpenState
	{
		NoChange = 0,
		ExpandAll,
		CollapseAll
	};
	OpenState  open_state = OpenState::NoChange;

	if ( ImGui::SmallButton("Expand All") )
	{
		open_state = OpenState::ExpandAll;
		TZK_LOG(LogLevel::Debug, "Expanding all treenodes");
	}
	ImGui::SameLine();
	if ( ImGui::SmallButton("Collapse All") )
	{
		open_state = OpenState::CollapseAll;
		TZK_LOG(LogLevel::Debug, "Collapsing all treenodes");
	}

	//auto&   forensics_settings = my_wksp->GetForensics()->settings;
	auto&   ng_settings = topology->my_nodegraph.settings;
	auto&   topology_settings = topology->settings;
	ImVec2  table_size(_gui_interactions.tabchild_size.x, 0.f); // full width, needed height
	
	if ( ImGui::BeginTable("Settings##", num_columns, table_flags, table_size) )
	{
		ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_WidthStretch
			| ImGuiTableColumnFlags_NoSort
			| ImGuiTableColumnFlags_NoHide;
		ImGui::TableSetupColumn(column1, col_flags);
		ImGui::TableSetupColumn(column2, col_flags);
		ImGui::TableHeadersRow();
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		if ( open_state != OpenState::NoChange )
			ImGui::SetNextItemOpen(open_state == OpenState::ExpandAll ? true : false);
		if ( ImGui::TreeNode("Workspace") )
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("Name", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::InputTextWithHint("##WorkspaceName", "Workspace Name", &my_wksp_data->name) )
			{
				/// @todo if empty, do not permit save, held within common handler
				if ( !my_wksp_data->name.empty() )
				{
					my_wksp->my_title = my_wksp_data->name;
					// use UUID as the only imgui ID for this window (dear imgui #251)
					my_wksp->my_title += "###";
					my_wksp->my_title += my_wksp_data->id.GetCanonical();
				}
			}
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("Auto-Save", tree_node_flags);
			ImGui::TableNextColumn();
			ImGui::TextDisabled("<False>");
			ImGui::TableNextColumn();
			ImGui::TreeNodeEx("Setting", tree_node_flags);
			ImGui::TableNextColumn();
			ImGui::Text("Value");
			ImGui::TableNextColumn();

			ImGui::TreePop();
		}
		if ( open_state != OpenState::NoChange )
			ImGui::SetNextItemOpen(open_state == OpenState::ExpandAll ? true : false);
		if ( ImGui::TreeNode("Dock Windows") )
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			auto  wndloc = [&tree_node_flags, this](const char* text, const char* name, WindowLocation& loc) {
				ImGui::TreeNodeEx(text, tree_node_flags);
				ImGui::TableNextColumn();
				ImGui::PushID(text);

				// old api better for this, but dislike amendment handling..
				int  cur = static_cast<int>(loc);
				int  index = (cur - 1);
				if ( ImGui::Combo("##Location", &index, "Hidden\0Top\0Left\0Bottom\0Right\0\0") )
				{
					if ( index != (cur - 1) )
					{
						loc = TConverter<WindowLocation>::FromUint8(static_cast<uint8_t>((index + 1)));
						my_wksp->ApplySetting(name, TConverter<WindowLocation>::ToString(loc).c_str());
					}
				}

				ImGui::PopID();
				ImGui::TableNextColumn();
			};

			wndloc("Canvas Debug", settingname_dock_canvasdbg, settings.dock_canvasdbg);
			wndloc("Property View", settingname_dock_propview, settings.dock_propview);

			ImGui::TreePop();
		}
		if ( open_state != OpenState::NoChange )
			ImGui::SetNextItemOpen(open_state == OpenState::ExpandAll ? true : false);
		if ( ImGui::TreeNode("Grid [Topology]") )
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Draw", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::Checkbox("##Grid.Draw", &ng_settings.grid_style.draw) )
			{
				my_wksp->ApplySetting(settingname_grid_draw, core::TConverter<bool>::ToString(ng_settings.grid_style.draw).c_str());
			}
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Draw Origin", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::Checkbox("##Grid.DrawOrigin", &ng_settings.grid_style.draw_origin) )
			{
				my_wksp->ApplySetting(settingname_grid_draworigin, core::TConverter<bool>::ToString(ng_settings.grid_style.draw_origin).c_str());
			}
			ImGui::TableNextColumn();

			ImVec4  fb = ImGui::ColorConvertU32ToFloat4(ng_settings.grid_style.colours.background);
			ImVec4  fp = ImGui::ColorConvertU32ToFloat4(ng_settings.grid_style.colours.primary);
			ImVec4  fs = ImGui::ColorConvertU32ToFloat4(ng_settings.grid_style.colours.secondary);
			ImVec4  fo = ImGui::ColorConvertU32ToFloat4(ng_settings.grid_style.colours.origins);
			ImVec4  fr = ImGui::ColorConvertU32ToFloat4(ng_settings.grid_style.colours.selector_rect);
			ImVec4  fl = ImGui::ColorConvertU32ToFloat4(ng_settings.grid_style.colours.link);

			ImGui::TreeNodeEx("Primary Colour", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::ColorEdit4("##Grid.Primary", &fp.x, ImGuiColorEditFlags_None) )
			{
				my_wksp->ApplySetting(settingname_grid_colour_primary, std::to_string(ImGui::ColorConvertFloat4ToU32(fp)).c_str());
			}
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Secondary Colour", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::ColorEdit4("##Grid.Secondary", &fs.x, ImGuiColorEditFlags_None) )
			{
				my_wksp->ApplySetting(settingname_grid_colour_secondary, std::to_string(ImGui::ColorConvertFloat4ToU32(fs)).c_str());
			}
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Origin Colour", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::ColorEdit4("##Grid.Origins", &fo.x, ImGuiColorEditFlags_None) )
			{
				my_wksp->ApplySetting(settingname_grid_colour_origin, std::to_string(ImGui::ColorConvertFloat4ToU32(fo)).c_str());
			}
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Background Colour", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::ColorEdit4("##Grid.Background", &fb.x, ImGuiColorEditFlags_None) )
			{
				/// @todo determine if only one needed now
				topology->my_nodegraph.GetCanvas().configuration.colour = ImGui::ColorConvertFloat4ToU32(fb);
				ng_settings.grid_style.colours.background = topology->my_nodegraph.GetCanvas().configuration.colour;
				my_wksp->ApplySetting(settingname_grid_colour_background, std::to_string(ImGui::ColorConvertFloat4ToU32(fb)).c_str());
			}
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Selector Rect Colour", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::ColorEdit4("##Grid.SelectorRect", &fr.x, ImGuiColorEditFlags_None) )
			{
				my_wksp->ApplySetting(settingname_grid_colour_selector, std::to_string(ImGui::ColorConvertFloat4ToU32(fr)).c_str());
			}
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Link Colour", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::ColorEdit4("##Grid.Link", &fl.x, ImGuiColorEditFlags_None) )
			{
				my_wksp->ApplySetting(settingname_grid_colour_link, std::to_string(ImGui::ColorConvertFloat4ToU32(fl)).c_str());
			}
			ImGui::TableNextColumn();
			
			ImGui::TreeNodeEx("Size", tree_node_flags);
			ImGui::TableNextColumn();
			int  sz = ng_settings.grid_style.size;
			if ( ImGui::SliderInt("##Grid.Size", &sz, 10, 100) )
			{
				// duplication but don't want warning log as user has no choice (unless we convert to combo or custom type)
				if ( sz % 10 == 0 )
					my_wksp->ApplySetting(settingname_grid_size, std::to_string(sz).c_str());
			}
			ImGui::SameLine();
			ImGui::HelpMarker("Valid values are bases of 10");
			ImGui::TableNextColumn();
			
			ImGui::TreeNodeEx("Subdivisions", tree_node_flags);
			ImGui::TableNextColumn();
			int  subdiv = ng_settings.grid_style.subdivisions;
			if ( ImGui::SliderInt("##Grid.Subdivisions", &subdiv, 1, 10) )
			{
				// duplication but don't want warning log as user has no choice (unless we convert to combo or custom type)
				if ( subdiv == 1 || subdiv == 2 || subdiv == 5 || subdiv == 10 )
					my_wksp->ApplySetting(settingname_grid_subdivisions, std::to_string(subdiv).c_str());
			}
			ImGui::SameLine();
			ImGui::HelpMarker("Valid values are: 1, 2, 5, or 10");
			ImGui::TableNextColumn();

			ImGui::TreePop();
		}
		if ( open_state != OpenState::NoChange )
			ImGui::SetNextItemOpen(open_state == OpenState::ExpandAll ? true : false);
		if ( ImGui::TreeNode("Links [Topology]") )
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Link Connection Method", tree_node_flags);
			ImGui::TableNextColumn();
			int  link_method = ng_settings.link_default_method;
			ImGui::RadioButton("Direct", &link_method, static_cast<int>(imgui::LinkMethod::Direct));
			ImGui::RadioButton("Cubic Bezier", &link_method, static_cast<int>(imgui::LinkMethod::CubicBezier));
			ImGui::RadioButton("Quadratic Bezier", &link_method, static_cast<int>(imgui::LinkMethod::QuadraticBezier));
			ImGui::RadioButton("Multi-line Auto", &link_method, static_cast<int>(imgui::LinkMethod::MultiLineAuto));
			ImGui::RadioButton("Multi-line Hybrid", &link_method, static_cast<int>(imgui::LinkMethod::MultiLineHybrid));
			if ( link_method != ng_settings.link_default_method )
			{
				my_wksp->ApplySetting(settingname_link_defaultmethod,
					imgui::TConverter<imgui::LinkMethod>::ToString(static_cast<imgui::LinkMethod>(link_method)).c_str()
				);
			}
			ImGui::TableNextColumn();

			ImGui::TreePop();
		}
		if ( open_state != OpenState::NoChange )
			ImGui::SetNextItemOpen(open_state == OpenState::ExpandAll ? true : false);
		if ( ImGui::TreeNode("Nodes") )
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Draw Headers [Topology]", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::Checkbox("##Node.DrawHeaders", &ng_settings.node_draw_headers) )
			{
				my_wksp->ApplySetting(settingname_node_drawheaders, core::TConverter<bool>::ToString(ng_settings.node_draw_headers).c_str());
			}
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Drag from Headers only [Topology]", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::Checkbox("##Node.DragViaHeaders", &ng_settings.node_drag_from_headers_only) )
			{
				my_wksp->ApplySetting(settingname_node_dragfromheadersonly, core::TConverter<bool>::ToString(ng_settings.node_drag_from_headers_only).c_str());
			}
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Track Online State", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::Checkbox("##Node.TrackOnlineState", &topology_settings.node_track_online_state) )
			{
				my_wksp->ApplySetting(settingname_node_trackonlinestate, core::TConverter<bool>::ToString(topology_settings.node_track_online_state).c_str());
			}
			ImGui::TableNextColumn();

			ImGui::TreeNodeEx("Node List Sort Method", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::Combo("##NodeList.SortMethod", &settings.nodelist_sort_order, nodelist_sortstrs) )
			{
				my_wksp->ApplySetting(settingname_nodelist_sortorder, nodelist_sortstrmap[nodelist_sortstrs.at(settings.nodelist_sort_order)].c_str());
			}
			ImGui::TableNextColumn();

			/*
			 * Note:
			 * Ensure this is last. The optional, dynamic tree entries breaks
			 * the cell selection, so a following checkbox appears alongside
			 * this row, not the following one like it should do.
			 * 
			 * This might be fixable by flag amendments, not looked into it;
			 * plain relocation good enough for now
			 */
			ImGui::TreeNodeEx("Override application Node List style", tree_node_flags);
			ImGui::TableNextColumn();
			if ( ImGui::Checkbox("##NodeList.OverrideApplicationStyle", &settings.nodelist_override_app_style) )
			{
				my_wksp->ApplySetting(settingname_nodelist_overrideappstyle, core::TConverter<bool>::ToString(settings.nodelist_override_app_style).c_str());
			}
			ImGui::TableNextColumn();

			if ( settings.nodelist_override_app_style )
			{
				if ( open_state != OpenState::NoChange )
					ImGui::SetNextItemOpen(open_state == OpenState::ExpandAll ? true : false);
				if ( ImGui::TreeNode("Node List Style") )
				{
					std::shared_ptr<nodelist_style>&  style = my_wksp_data->nlist_style;

					ImGui::SameLine();
					if ( ImGui::Button("Reset") )
					{
						// resets to the application default, not the current style
						nodelist_style  defaults;
						*style = defaults;
					}

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					
					ImGuiColorEditFlags  cef = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_None;

					ImGui::TreeNodeEx("Online Indicator Radius", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::SliderFloat("##NodeListStyle.OnlineIndicatorRadius", &style->online_indicator_radius, 4.0f, 8.0f, "%.0f");
					ImGui::SameLine();
					ImGui::HelpMarker("UNUSED");
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Background Colour", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::ColorEdit4("##NodeListStyle.BackgroundColour", (float*)&style->node_background_colour, cef);
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Background Colour - Selected", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::ColorEdit4("##NodeListStyle.BackgroundColourSelected", (float*)&style->node_background_colour_selected, cef);
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Node Colours Follow Status", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::Checkbox("##NodeListStyle.NodeColoursFollowStatus", &style->node_bg_follows_online_status);
					ImGui::SameLine();
					ImGui::HelpMarker("Overrides standard background colour, instead using status values. If is indeterminate or node is inactive, still uses the standard colours");
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Background Colour - Down", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::ColorEdit4("##NodeListStyle.BackgroundColourDown", (float*)&style->online_indicator_colour_down, cef);
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Background Colour - Mixed", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::ColorEdit4("##NodeListStyle.BackgroundColourMixed", (float*)&style->online_indicator_colour_mixed, cef);
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Background Colour - Up", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::ColorEdit4("##NodeListStyle.BackgroundColourUp", (float*)&style->online_indicator_colour_up, cef);
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Text Colour", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::ColorEdit4("##NodeListStyle.TextColour", (float*)&style->node_text_colour, cef);
					ImGui::TableNextColumn();

					// boolean, ShowActivityProgressBar

					ImGui::TreeNodeEx("Progress Colour 1", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::ColorEdit4("##NodeListStyle.ProgressColour1", (float*)&style->progress_colour1, cef);
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Progress Colour 2", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::ColorEdit4("##NodeListStyle.ProgressColour2", (float*)&style->progress_colour2, cef);
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Node Rounding", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::SliderFloat("##NodeListStyle.Rounding", &style->node_rounding, 0.0f, 20.0f, "%.0f");
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Node Internal Extras Width", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::SliderFloat("##NodeListStyle.InternalRPad", &style->node_internal_rwidth, 10.0f, 255.0f, "%.0f");
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Node Height", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::SliderFloat("##NodeListStyle.NodeHeight", &style->node_size.y, ImGui::GetTextLineHeightWithSpacing(), ImGui::GetTextLineHeightWithSpacing() * 10, "%.0f");
					ImGui::TableNextColumn();

					ImGui::TreeNodeEx("Node Width", tree_node_flags);
					ImGui::TableNextColumn();
					ImGui::SliderFloat("##NodeListStyle.NodeWidth", &style->node_size.x, 10.0f, 1024.0f, "%.0f");
					ImGui::TableNextColumn();

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}

		ImGui::EndTable();
	}

	ImGui::TextDisabled("For style editing, use Application > Style Editor");
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI
