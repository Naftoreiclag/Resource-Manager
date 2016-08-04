#ifndef CONVERT_HPP
#define CONVERT_HPP

#include <string>

#include <boost/filesystem.hpp>

#include "json/json.h"

void convertImage(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename = true);
void convertMiscellaneous(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename = true);
void convertGeometry(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename = true);
void convertFont(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename = true);
void convertWaveform(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename = true);

#endif // CONVERT_HPP
