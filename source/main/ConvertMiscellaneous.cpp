#include "Convert.hpp"

std::string convertMiscellaneous(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {
    boost::filesystem::copy_file(fromFile, outputFile);
    return outputFile.filename().c_str();
}
