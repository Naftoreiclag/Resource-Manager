#ifndef RESMAN_MAIN_EXPAND_HPP
#define RESMAN_MAIN_EXPAND_HPP

#include <string>
#include <functional>
#include <vector>

#include <boost/filesystem.hpp>
#include <json/json.h>

#include "Common.hpp"

namespace resman {

struct Expansion {
    Object m_obj;
    std::string m_subtype;
};

std::vector<Expansion> expand_bgfx_shader(const Object& obj);

typedef std::function<std::vector<Expansion> (const Object&)> Expand_Func;
    
} // namespace resman

#endif // RESMAN_MAIN_EXPAND_HPP
