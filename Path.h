#pragma once
#include <string>

inline std::string appPath(const std::string& rel)
{
    return std::string(APP_ROOT) + "/" + rel;
}
