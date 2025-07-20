/**
 * @file        sys/linux/src/core/debugger.cc
 * @license     zlib (view the LICENSE file for details)
 * @copyright   Trezanik Developers, 2014-2021
 */


#include "common_definitions.h"

#include "core/debugger.h"
#include "core/util/filesystem/file.h"

#include <sys/stat.h>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>


namespace trezanik {
namespace core {


bool
is_debugger_attached()
{
    char  buf[4096];

    const int status_fd = open("/proc/self/status", O_RDONLY);

    if ( status_fd == -1 )
        return false;

    const ssize_t num_read = read(status_fd, buf, sizeof(buf) - 1);
    close(status_fd);

    if ( num_read <= 0 )
        return false;

    buf[num_read] = '\0';
    constexpr char tracer_pid[] = "TracerPid:";
    const auto tracer_pid_ptr = strstr(buf, tracer_pid);

    if ( !tracer_pid_ptr )
        return false;

    for ( const char* cptr = tracer_pid_ptr + sizeof(tracer_pid) - 1; cptr <= buf + num_read; ++cptr)
    {
        if ( isspace(*cptr) )
            continue;
        else
            return isdigit(*cptr) != 0 && *cptr != '0';
    }

    return false;
}


}  // namespace core
}  // namespace trezanik
