#ifndef JSONUTIL_HPP
#define JSONUTIL_HPP

#include <string>

#include <json/json.h>

Json::Value readJsonFile(std::string filename);
void writeJsonFile(std::string filename, Json::Value& value, bool compact = true);

#endif // JSONUTIL_HPP
