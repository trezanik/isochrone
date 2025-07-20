#pragma once

/**
 * @file        src/imgui/ImGuiImpl_Base.h
 * @brief       Base class for ImGui implementations
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"

#include "imgui/IImGuiImpl.h"
#include "imgui/dear_imgui/imgui.h"
#include "core/util/time.h"

#include <functional>
#include <string>
#include <vector>


/*
 * Shamelessly ripped from here:
 * https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
 */
inline void hash_combine(std::size_t& TZK_UNUSED(seed))
{
}

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
{
	std::hash<T> hasher;
	seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	hash_combine(seed, rest...);
}



namespace trezanik {
namespace imgui {


/**
 * Base class for ImGui implementations
 */
class TZK_IMGUI_API ImGuiImpl_Base : public IImGuiImpl
{
private:
protected:

	/// Flag to force rendering the next frame
	bool      _force_render;
	/// The number of frames to force rendering
	uint8_t   _forced_frames;
	/// The last render accumulation hash
	size_t    _last_draw_hash;
	/// The time the last render was performed at
	uint64_t  _last_want_render;

	/// The ImGui context all operations are based around
	ImGuiContext*  _context;

public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] context
	 *  The imgui context in service
	 */
	ImGuiImpl_Base(
		ImGuiContext* context
	)
	: _context(context)
	{
		_force_render = false;
		_forced_frames = 0;
		_last_draw_hash = 0;
		_last_want_render = 0;
	}


	/**
	 * Standard destructor
	 */
	virtual ~ImGuiImpl_Base()
	{
		// do not destroy the context, we don't own it
	}


	/**
	 * Implementation of IImGuiImpl::ForceRenderNextFrames
	 */
	virtual void
	ForceRenderNextFrames(
		uint8_t count
	) override
	{
		if ( _forced_frames == 0 || count < _forced_frames )
			_forced_frames += count;
	}


	/**
	 * Implementation of IImGuiImpl::WantRender
	 *
	 * @bug 2
	 *  Currently non-functional, always returns true
	 */
	virtual bool
	WantRender() final override
	{
		//ImGuiIO&    io = ImGui::GetIO();
		ImDrawData* draw_data = ImGui::GetDrawData();

		// implementation overrules anything in this base
		if ( _force_render )
			return true;
		if ( _forced_frames > 0 )
		{
			_forced_frames--;
			return true;
		}

		// safety and check; nothing to draw, no desire to render
		//if ( draw_data == nullptr || draw_data->CmdListsCount == 0 )
			//return false;

		// force a redraw each second
		uint64_t  now = core::aux::get_ms_since_epoch();
		if ( _last_want_render == 0 || (_last_want_render - now) > 1000 )
		{
			_last_want_render = now;
			return true;
		}

		// otherwise, calculate the draw data hash
		//float   w = (draw_data->DisplaySize.x * io.DisplayFramebufferScale.x);
		//float   h = (draw_data->DisplaySize.y * io.DisplayFramebufferScale.y);
		size_t  hash = 0;

		//hash_combine(hash, &w);
		//hash_combine(hash, &h);
		hash_combine(hash, &draw_data->CmdListsCount);

		for ( int i = 0; i < draw_data->CmdListsCount; i++ )
		{
			hash_combine(hash, &(draw_data->CmdLists[i]->VtxBuffer[0]), draw_data->CmdLists[i]->VtxBuffer.size() * sizeof(ImDrawVert));
			hash_combine(hash, &(draw_data->CmdLists[i]->IdxBuffer[0]), draw_data->CmdLists[i]->IdxBuffer.size() * sizeof(ImDrawIdx));
			hash_combine(hash, &(draw_data->CmdLists[i]->CmdBuffer[0]), draw_data->CmdLists[i]->CmdBuffer.size() * sizeof(ImDrawCmd));
		}

		// if the draw data has not changed, we don't want to render anything
		if ( hash == _last_draw_hash )
		{
			return false;
		}

		_last_draw_hash = hash;
		_last_want_render = now;

		return true;
	}
};


} // namespace imgui
} // namespace trezanik
