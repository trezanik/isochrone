#pragma once

/**
 * @file        src/imgui/ImNodeGraphLink.h
 * @brief       Link definition
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "imgui/definitions.h"
#include "imgui/dear_imgui/imgui.h"

#include "core/UUID.h"

#include <memory>


namespace trezanik {
namespace imgui {


class ImNodeGraph;
class Pin;


/**
 * A link between two pins that will be displayed on the node graph
 * 
 * Text and its offset are liable to change, as adding anchor points would be a
 * better (optional) way of implementing it; and intended to be added alongside
 * alternate line types.
 */
class IMGUI_API Link
{
	TZK_NO_CLASS_ASSIGNMENT(Link);
	TZK_NO_CLASS_COPY(Link);
	TZK_NO_CLASS_MOVEASSIGNMENT(Link);
	TZK_NO_CLASS_MOVECOPY(Link);

private:
	/// Unique link ID
	trezanik::core::UUID  my_uuid;
	
	/// Source pin for this link
	std::shared_ptr<Pin>  my_source;
	/// Target pin for this link
	std::shared_ptr<Pin>  my_target;

	/// Raw pointer to the nodegraph this link resides in
	ImNodeGraph*  my_ctx;
	/// Text displayed on the link line
	std::string*  my_text;
	/// Offset the text is displayed at on the link line
	ImVec2*  my_text_offset;

	/// Is the link hovered
	bool  my_hovered;
	/// Is the link selected
	bool  my_selected;

protected:
public:
	/**
	 * Standard constructor
	 * 
	 * @param[in] uuid
	 *  The UUID of the link
	 * @param[in] source
	 *  The shared_ptr Pin source
	 * @param[in] target
	 *  The shared_ptr Pin target
	 * @param[in] context
	 *  Pointer to the node graph that contains this link
	 * @param[in] text
	 *  Pointer to the text to display on the link
	 * @param[in] text_offset
	 *  (Optional) Relative offset of the text from the center-point of the link,
	 *  where {0,0} is the center
	 */
	explicit Link(
		const trezanik::core::UUID& uuid,
		std::shared_ptr<Pin> source,
		std::shared_ptr<Pin> target,
		ImNodeGraph* context,
		std::string* text,
		ImVec2* text_offset
	)
	: my_uuid(uuid)
	, my_source(source)
	, my_target(target)
	, my_ctx(context)
	, my_text(text)
	, my_text_offset(text_offset)
	, my_hovered(false)
	, my_selected(false)
	{
		assert(uuid != core::blank_uuid);
		assert(source != nullptr);
		assert(target != nullptr);
		assert(context != nullptr);
		assert(text != nullptr);
		assert(text_offset != nullptr);
	}


	/**
	 * Standard destructor
	 */
	~Link()
	{
	}


	/**
	 * Gets the unique identifier for this link
	 *
	 * @return
	 *  A const-reference to the UUID
	 */
	const trezanik::core::UUID&
	GetID()
	{
		return my_uuid;
	}


	/**
	 * Gets the string pointer for this links text
	 *
	 * @return
	 *  The string pointer 
	 */
	std::string*
	GetText()
	{
		return my_text;
	}


	/**
	 * Gets the vector offset for this links text
	 *
	 * @return
	 *  A pointer to the text offset vector
	 */
	ImVec2*
	GetTextOffset()
	{
		return my_text_offset;
	}


	/**
	 * Gets a copy of this links unique ID
	 *
	 * @sa GetID
	 * @return
	 *  The link UUID in a new object
	 */
	trezanik::core::UUID
	ID() const
	{
		return my_uuid;
	}


	/**
	 * Gets the hovering status of this link
	 * 
	 * @return
	 *  true if the link is hovered in the current frame
	 */
	bool
	IsHovered() const
	{
		return my_hovered;
	}


	/**
	 * Gets the selected status of this link
	 * 
	 * @return
	 *  true if the link is selected in the current frame
	 */
	bool
	IsSelected() const
	{
		return my_selected;
	}


	/**
	 * Gets the source pin of the link
	 * 
	 * @return
	 *  The shared_ptr to the source Pin
	 */
	std::shared_ptr<Pin>
	Source() const
	{
		return my_source;
	}


	/**
	 * Gets the target pin of the link
	 * 
	 * @return
	 *  The shared_ptr to the target Pin
	 */
	std::shared_ptr<Pin>
	Target() const
	{
		return my_target;
	}


	/**
	 * Per-frame update
	 * 
	 * Performs drawing, and handling for deletion and status changes.
	 */
	void
	Update();
};


} // namespace imgui
} // namespace trezanik
