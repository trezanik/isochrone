/**
 * @file        src/app/Command.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2025
 */


#include "app/definitions.h"

#include "app/Command.h"



namespace trezanik {
namespace app {


Command::Command(
	Cmd type,
	command_data&& before,
	command_data&& after
)
: my_type(type)
, my_original(std::move(before))
, my_updated(std::move(after))
{
}


Command::~Command()
{
}


} // namespace app
} // namespace trezanik

