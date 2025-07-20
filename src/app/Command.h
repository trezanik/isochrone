#pragma once

/**
 * @file        src/app/Command.h
 * @brief       Command pattern holder
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "core/UUID.h"
#include "imgui/dear_imgui/imgui.h"

#include <string>


namespace trezanik {
namespace app {


/**
 * Enumeration of all possible commands that can have undo/redo applied
 */
enum class Cmd
{
	NodeMove,
	NodeResize,
	NodeDelete,
	NodeCreate,
	PinNew,
	PinDelete,
	LinkNew,
	LinkDelete,
	TextEdit,
	FloatEdit,
	IntEdit,
	UintEdit,
	ColourEdit
};


/**
 * Structure containing all possible values for every command item
 * 
 * Possibility this could be a union, don't rule it out until all implemented
 * 
 * Designed such that there is a before and after copy of this struct. Naturally,
 * there is nothing 'before' for created items, so the only populated member
 * will be the id, so it can be a straight lookup-and-remove.
 *
 * Initializers provided so no need for construction
 */
struct command_data
{
	/*
	 * Valid for:
	 * - NodeMove
	 * - NodeResize (width and height)
	 */
	ImVec2   vec2 = { 0, 0 };

	/*
	 * Valid for:
	 * - NodeResize (TLRB from pos offset)
	 * - ColourEdit
	 */
	ImVec4   vec4 = { 0, 0, 0, 0 };

	/**
	 * Valid for:
	 * - TextEdit
	 */
	std::string  text;

	/**
	 * Valid for:
	 * - ColourEdit
	 */
	ImU32   colour = 0;
	// node??
	// pin?? link??

	/**
	 * Valid for:
	 * - All
	 */
	trezanik::core::UUID   id = trezanik::core::blank_uuid;

	/**
	 * Valid for:
	 * - LinkNew
	 * - LinkDelete
	 * - PinNew
	 * - PinDelete
	 */
	trezanik::core::UUID   source_id = trezanik::core::blank_uuid;

	/**
	 * Valid for:
	 * - LinkNew
	 * - LinkDelete
	 */
	trezanik::core::UUID   target_id = trezanik::core::blank_uuid;
};


/**
 * Class holding all required elements for a command
 * 
 * These hold details of an action that has already been performed; this is
 * essentially our record of it, in turn enabling the ability to rollback the
 * action.
 * 
 * @warning
 *  Documentation like the class is incomplete; not doing the former until I've
 *  actually gotten the design thought about for longer than a day
 */
class Command
{
	//TZK_NO_CLASS_ASSIGNMENT(Command);
	//TZK_NO_CLASS_COPY(Command);
	//TZK_NO_CLASS_MOVEASSIGNMENT(Command);
	//TZK_NO_CLASS_MOVECOPY(Command);

private:

	Cmd   my_type;
	command_data  my_original;
	command_data  my_updated;

protected:
public:
	Command(
		Cmd type,
		command_data&& before,
		command_data&& after
	);


	~Command();


	const command_data&
	GetAfter() const
	{
		return my_updated;
	}


	const command_data&
	GetBefore() const
	{
		return my_original;
	}


	Cmd
	Type() const
	{
		return my_type;
	}
};


} // namespace app
} // namespace trezanik
