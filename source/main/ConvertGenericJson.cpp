#include "Convert.hpp"

#include "JsonUtil.hpp"

void convertGenericJson(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {
    Json::Value jsonFile = readJsonFile(fromFile.string());
    writeJsonFile(outputFile.string(), jsonFile);
}
