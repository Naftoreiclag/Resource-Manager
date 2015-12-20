#ifndef CONVERT_HPP
#define CONVERT_HPP

#include <string>

#include <boost/filesystem.hpp>

#include "json/json.h"

std::string convertImage(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename = true);
std::string convertMiscellaneous(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename = true);


#endif // CONVERT_HPP
