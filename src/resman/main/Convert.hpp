#ifndef CONVERT_HPP
#define CONVERT_HPP

#include <string>
#include <functional>

#include <boost/filesystem.hpp>

#include <json/json.h>

namespace resman {
    
struct Convert_Args {
    boost::filesystem::path fromFile;
    boost::filesystem::path outputFile;
    Json::Value params;
    bool modifyFilename = true;
};

void convertImage(const Convert_Args& args);
void convertMiscellaneous(const Convert_Args& args);
void convertGeometry(const Convert_Args& args);
void convertFont(const Convert_Args& args);
void convertWaveform(const Convert_Args& args);
void convertGenericJson(const Convert_Args& args);
void convertGlsl(const Convert_Args& args);

typedef std::function<void(const Convert_Args&)> Convert_Func;

} // namespace resman

#endif // CONVERT_HPP
