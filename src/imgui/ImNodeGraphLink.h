#pragma once

/**
 * @file        src/imgui/ImNodeGraphLink.h
 * @brief       Link definition
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2026
 */


#include "imgui/definitions.h"
#include "imgui/dear_imgui/imgui.h"

#include "core/UUID.h"

#include <memory>
#include <vector>


namespace trezanik {
namespace imgui {


class ImNodeGraph;
class Pin;


#if 0  // Code Disabled until used
static size_t   min_control_points = 2;
static size_t   max_control_points_direct = 2;
static size_t   max_control_points_quadbez = 3;
static size_t   max_control_points_cubbez = 4;
static size_t   max_control_points_multi = 16;
#endif
extern IMGUI_API float  control_point_radius;


enum class LinkMethod : int
{
	/** Invalid use only */
	Invalid = 0,
	/** Exact point-to-point straight line */
	Direct,
	/** Quadratic bezier curve (3 control points) - not implemented properly yet */
	QuadraticBezier,
	/** Cubic bezier curve (4 control points) */
	CubicBezier,
	/** Straight line navigation to target with user specified control points */
	MultiLinePoint
};


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
	/** Unique link ID **/
	trezanik::core::UUID  my_uuid;
	
	/** Source pin for this link */
	std::shared_ptr<Pin>  my_source;
	/** Target pin for this link */
	std::shared_ptr<Pin>  my_target;

	/** Raw pointer to the nodegraph this link resides in */
	ImNodeGraph*  my_ctx;
	/** Text displayed on the link line */
	std::string*  my_text;
	/** Offset the text is displayed at on the link line */
	ImVec2*  my_text_offset;

	/** Is the link hovered */
	bool  my_hovered;
	/** Is the link selected */
	bool  my_selected;


	/** Pointer to the link display method in use */
	LinkMethod*   my_method;

	/** Location of all explicit control points for the link */
	std::vector<ImVec2>*  my_control_points;


	/**
	 * Draws the link as a Cubic Bezier curve
	 *
	 * @sa DrawDirect, DrawMultiLinePoint, DrawQuadraticBezier
	 */
	void
	DrawCubicBezier();

	/**
	 * Draws the link as plain straight line
	 *
	 * @sa DrawCubicBezier, DrawMultiLinePoint, DrawQuadraticBezier
	 */
	void
	DrawDirect();

	/**
	 * Draws the link as multiple straight lines with manual control points
	 *
	 * @sa DrawCubicBezier, DrawDirect, DrawQuadraticBezier
	 */
	void
	DrawMultiLinePoint();

	/**
	 * Draws the link as a Quadratic Bezier curve - not yet implemented properly
	 *
	 * @sa DrawCubicBezier, DrawDirect, DrawMultiLinePoint
	 */
	void
	DrawQuadraticBezier();

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
	 *  Pointer to the relative offset of the text from the center-point of the
	 *  link, where {0,0} is the center
	 * @param[in] method
	 *  The display type method
	 * @param[in] control_points
	 *  Pointer to collection of control points
	 */
	Link(
		const trezanik::core::UUID& uuid,
		std::shared_ptr<Pin> source,
		std::shared_ptr<Pin> target,
		ImNodeGraph* context,
		std::string* text,
		ImVec2* text_offset,
		LinkMethod* method,
		std::vector<ImVec2>* control_points
	);


	/**
	 * Standard destructor
	 */
	~Link();


	/**
	 * Pushes a new control point to the back of the collection
	 *
	 * @param point
	 *  The point co-ordinates to add
	 */
	void
	AddControlPoint(
		ImVec2& point
	);


	/**
	 * Deletes an existing control point.
	 *
	 * If two control points happen to share the same position, then this will
	 * remove only the first one found
	 *
	 * @param point
	 *  The position to delete
	 */
	void
	DeleteControlPoint(
		ImVec2& point
	);


	/**
	 * Gets the collection of control points
	 *
	 * @return
	 *  A pointer to the vector of ImVec2 positions
	 */
	std::vector<ImVec2>*
	GetControlPoints()
	{
		return my_control_points;
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
	 * Gets the method used for connecting the source and target pins
	 *
	 * @return
	 *  A pointer to the display method
	 */
	LinkMethod*
	GetMethod()
	{
		return my_method;
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
