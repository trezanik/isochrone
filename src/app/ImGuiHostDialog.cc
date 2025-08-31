/**
 * @file        src/app/ImGuiHostDialog.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#if TZK_USING_IMGUI

#include "app/ImGuiHostDialog.h"
#include "app/AppConfigDefs.h"

#include "engine/services/ServiceLocator.h"
#include "engine/resources/ResourceCache.h"
#include "engine/resources/ResourceLoader.h"
#include "engine/Context.h"

#include "core/util/string/string.h"
#include "core/services/log/Log.h"
#include "core/error.h"


namespace trezanik {
namespace app {


ImGuiHostDialog::ImGuiHostDialog(
	GuiInteractions& gui_interactions
)
: IImGui(gui_interactions)
, my_context(engine::Context::GetSingleton())
{
	using namespace trezanik::core;
	using namespace trezanik::engine;

	TZK_LOG(LogLevel::Trace, "Constructor starting");
	{
		_gui_interactions.host_dialog = this;

		// we need to receive resource load notifications
		// pending addition

		std::string  fpath_win2k  = aux::BuildPath(my_context.AssetPath() + assetdir_images, "icon_win2k.png");
		std::string  fpath_winxp  = aux::BuildPath(my_context.AssetPath() + assetdir_images, "icon_winxp.png");
		std::string  fpath_winvista7  = aux::BuildPath(my_context.AssetPath() + assetdir_images, "icon_winvista_7.png");
		std::string  fpath_win8   = aux::BuildPath(my_context.AssetPath() + assetdir_images, "icon_win8.png");
		std::string  fpath_win10  = aux::BuildPath(my_context.AssetPath() + assetdir_images, "icon_win10.png");
		std::string  fpath_win11  = aux::BuildPath(my_context.AssetPath() + assetdir_images, "icon_win11.png");
		my_icon_win2k_rid  = my_context.GetResourceCache().GetResourceID(fpath_win2k.c_str());
		my_icon_winxp_rid  = my_context.GetResourceCache().GetResourceID(fpath_winxp.c_str());
		my_icon_winvista7_rid  = my_context.GetResourceCache().GetResourceID(fpath_winvista7.c_str());
		my_icon_win8_rid   = my_context.GetResourceCache().GetResourceID(fpath_win8.c_str());
		my_icon_win10_rid  = my_context.GetResourceCache().GetResourceID(fpath_win10.c_str());
		my_icon_win11_rid  = my_context.GetResourceCache().GetResourceID(fpath_win11.c_str());

		auto& ldr = my_context.GetResourceLoader();

		auto ResourceLoad = [&ldr, this](
			trezanik::core::UUID& id,
			std::shared_ptr<trezanik::engine::Resource_Image>& icon,
			std::string& fpath
		)
		{
			if ( icon != nullptr )
				return;

			if ( id == null_id )
			{
				auto res = std::make_shared<Resource_Image>(fpath);
				if ( ldr.AddResource(std::dynamic_pointer_cast<Resource>(res)) == ErrNONE )
				{
					// track the resource so we can assign it when ready
					id = res->GetResourceID();
				}
			}
			else
			{
				icon = std::dynamic_pointer_cast<Resource_Image>(my_context.GetResourceCache().GetResource(id));
			}
		};

		ResourceLoad(my_icon_win2k_rid, my_icon_win2k, fpath_win2k);
		ResourceLoad(my_icon_winxp_rid, my_icon_winxp, fpath_winxp);
		ResourceLoad(my_icon_winvista7_rid, my_icon_winvista7, fpath_winvista7);
		ResourceLoad(my_icon_win8_rid, my_icon_win8, fpath_win8);
		ResourceLoad(my_icon_win10_rid, my_icon_win10, fpath_win10);
		ResourceLoad(my_icon_win11_rid, my_icon_win11, fpath_win11);

		ldr.Sync();
	}
	TZK_LOG(LogLevel::Trace, "Constructor finished");
}


ImGuiHostDialog::~ImGuiHostDialog()
{
	using namespace trezanik::core;

	TZK_LOG(LogLevel::Trace, "Destructor starting");
	{
		_gui_interactions.host_dialog = nullptr;


	}
	TZK_LOG(LogLevel::Trace, "Destructor finished");
}


void
ImGuiHostDialog::Draw()
{
	const char*  host = "PLACEHOLDER";
	const char*  domain = "PLACEHOLDER";
	const char*  ipv4 = "PLACEHOLDER";
	const char*  mac = "PLACEHOLDER";
	const char*  os_str = "PLACEHOLDER";
	const char*  god_state = "PLACEHOLDER";

	if ( ImGui::Begin("Host") )
	{
		ImGui::Text("Hostname: %s", host);
		ImGui::Text("Domain: %s", domain);

		// right
		ImGui::Text("IPv4.........: %s", ipv4);
		ImGui::Text("MAC Address..: %s", mac);

		// left
		ImGui::Text("Operating System:\n%s", os_str);
		//ImGui::Image(osicon);

		ImGuiTableFlags  table_flags = ImGuiTableFlags_Resizable
			| ImGuiTableFlags_NoSavedSettings
			| ImGuiTableFlags_RowBg
			| ImGuiTableFlags_SizingStretchProp
			| ImGuiTableFlags_ScrollY
			| ImGuiTableFlags_HighlightHoveredColumn;

		if ( ImGui::BeginTable("loggedon##", 2, table_flags) )
		{
			ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
			ImGui::TableSetupColumn("User", col_flags, 0.3f);
			ImGui::TableSetupColumn("Session", col_flags, 0.7f);
			ImGui::TableHeadersRow();
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			/*
			 * C:\Windows\system32>query user
			 *  USERNAME              SESSIONNAME        ID  STATE   IDLE TIME  LOGON TIME
			 * >user                  console             1  Active    5+01:29  26/07/2024 18:56
			 * netwkstauserenum
			 */
			ImGui::NextColumn();
			ImGui::Text("<username>");
			ImGui::NextColumn();
			ImGui::Text("<source>");

			ImGui::EndTable();
		}

		if ( ImGui::CollapsingHeader("Anomalies") )
		{
			/*
			 *  Baseline                   | Current
			 * ----------------------------|-------------------
			 *  Path               Item    | Path        Item
			 * ----------------------------|-------------------
			 * <path>             <item>   | <path>     <item>
			 * ...
			 */
		}

		if ( ImGui::CollapsingHeader("Autostarts") )
		{
			ImGuiTableFlags  autostarts_table_flags = ImGuiTableFlags_Resizable
				| ImGuiTableFlags_NoSavedSettings
				| ImGuiTableFlags_RowBg
				| ImGuiTableFlags_SizingStretchProp
				| ImGuiTableFlags_ScrollY
				| ImGuiTableFlags_HighlightHoveredColumn;

			if ( ImGui::BeginTable("autostarts##", 2, autostarts_table_flags) )
			{
				ImGuiTableColumnFlags  col_flags = ImGuiTableColumnFlags_NoHeaderWidth | ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_PreferSortDescending;
				ImGui::TableSetupColumn("Path", col_flags, 0.3f);
				ImGui::TableSetupColumn("Autostart", col_flags, 0.7f);
				ImGui::TableHeadersRow();
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
	
				ImGui::NextColumn();
				ImGui::Text("<location>");
				ImGui::NextColumn();
				ImGui::Text("<executed>");

				ImGui::EndTable();
			}
		}


		ImGui::Text("GOD Connection: %s", god_state);

		if ( ImGui::Button("Close") )
		{
			// close this window
		}
	}

	ImGui::End();
}


} // namespace app
} // namespace trezanik

#endif  // TZK_USING_IMGUI
