#ifndef CONVERT_HPP
#define CONVERT_HPP

#include <string>

#include <boost/filesystem.hpp>

#include "json/json.h"

std::string convertImage(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params);
std::string convertMiscellaneous(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params);


#endif // CONVERT_HPP
