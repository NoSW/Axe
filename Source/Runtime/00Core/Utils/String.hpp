/*
    Optimization for std::string based on length (on 64bit machine)
        - small_string   [0, 23] characters,          SSO(Small-String-Optimization)
        - medium_string  (23, 255] characters,        EC (Eager-Copy)
        - large_string   (255, +infinity) characters, COW(Copy-On-Write)

    Optimization for std::string based on read-write-attributes
        - ro_string   (read only)
        - wo_string   (write only)
        - rw_string   (read write)
        - text_string (used for display)
*/

#pragma once

#include <string>

namespace axe::utils
{

}
