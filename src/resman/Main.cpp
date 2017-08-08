/*
 *  Copyright 2015-2017 James Fong
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <fstream>
#include <sstream>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

#include <boost/filesystem.hpp>
#include <json/json.h>
#include <MurmurHash3.h>

#include "logger/Logger.hpp"
#include "main/Convert.hpp"
#include "main/JsonUtil.hpp"

namespace resman {

// Useful for debug information, but significantly slows down packaging
bool n_verbose = true;

typedef std::string OType;

bool isWorkInProgressType(const OType& type) {
    return false;
}

std::map<OType, Convert_Func> n_converters = {
    {"material", convertGenericJson},
    {"model", convertGenericJson},
    {"shader-program", convertGenericJson},
    {"texture", convertGenericJson},
    
    {"image", convertImage},
    
    {"geometry", convertGeometry},
    
    {"font", convertFont},
    
    {"waveform", convertWaveform},
    
    {"vertex-shader", convertGlsl},
    {"tess-control-shader", convertGlsl},
    {"tess-evaluation-shader", convertGlsl},
    {"geometry-shader", convertGlsl},
    {"fragment-shader", convertGlsl},
    {"compute-shader", convertGlsl},
    
    {"script", convertMiscellaneous},
    {"string", convertMiscellaneous}
};

/**
 * @class Config
 * @brief Struct holding the config for a single resource project.
 * Default configuration:
 *      no obfuscation
 *      no ignored directories
 *      output directory is named "__output__" and is a sub-directory of 
 *          the project's root directory
 *      previous output not overwritten
 *      no intermediate directory / no intermediate data used
 */
struct Config {
    bool obfuscate = false;
    std::vector<boost::filesystem::path> ignoreDirs;
    boost::filesystem::path outputDir;
    bool forceOverwriteOutput = false;
    boost::filesystem::path intermediateDir;
};

/**
 * @class Object
 * @brief A single resource to be translated (or not)
 */
struct Object {
    std::string mName;
    OType mType;
    boost::filesystem::path mFile;
    boost::filesystem::path mDebugOrigin;
    Json::Value mParams;

    // Needed only by intermediate stuff
    uint32_t mOriginalSize;
    uint32_t mOriginalHash;
    bool mSkipTranslate = false;
    
    bool mAlwaysRetranslate = false;

    //
    boost::filesystem::path mOutputFile;
    uint32_t mOutputSize;
};

void translateData(const Object& object, bool modifyFilename) {
    if (!boost::filesystem::exists(object.mFile)) {
        std::stringstream sss;
        sss << "File does not exist: "
            << object.mFile.filename();
        throw std::runtime_error(sss.str());
    }
    
    auto pair_ptr = n_converters.find(object.mType);
    
    if (pair_ptr == n_converters.end()) {
        std::stringstream sss;
        sss << "Unknown type: \""
            << object.mType
            << "\"";
        throw std::runtime_error(sss.str());
    }
    
    Convert_Args args;
    args.fromFile = object.mFile;
    args.outputFile = object.mOutputFile;
    args.modifyFilename = modifyFilename;
    args.params = object.mParams;
    
    Convert_Func& converter = pair_ptr->second;
    converter(args);
}

class Project {
private:
    Config uconf;
    std::vector<boost::filesystem::path> objectFiles;
    
    Config parse_config(boost::filesystem::path file_config) {
        boost::filesystem::path outputDir = m_package_dir / "__output__";
        
        Config uconf;
        
        if (boost::filesystem::exists(file_config)) {
            Json::Value json_config = readJsonFile(file_config.string());
            uconf.outputDir = m_package_dir 
                    / (json_config["output"].asString());
            uconf.obfuscate = json_config["obfuscate"].asBool();
            uconf.forceOverwriteOutput = 
                    json_config["force-overwrite-output"].asBool();

            if (!json_config["intermediate"].isNull()) {
                uconf.intermediateDir = m_package_dir 
                        / (json_config["intermediate"].asString());
            }
            
            Json::Value& json_ignore_list = json_config["ignore"];
            
            for (Json::Value& ignore : json_ignore_list) {
                
                uconf.ignoreDirs.push_back(m_package_dir / (ignore.asString()));
            }
        }
        
        return uconf;
    }
    
    boost::filesystem::path m_package_file;
    boost::filesystem::path m_package_dir;
    
    Json::Value m_package_json;
    
    std::vector<Object> objects;
    
    void parseObject(const Json::Value& objectData, 
            const boost::filesystem::path& objectFile) {
        Object object;
        object.mDebugOrigin = objectFile;
        object.mName = objectData["name"].asString();
        object.mType = objectData["type"].asString();
        object.mAlwaysRetranslate = objectData["always-retranslate"].asBool();
        object.mParams = objectData["parameters"];
        
        boost::filesystem::path newPath = objectFile;
        object.mFile = newPath.remove_filename() 
                / objectData["file"].asString();
        
        objects.push_back(object);
        
        if (n_verbose) {
            Logger::log()->info("Resource: name = %v", object.mName);
            Logger::log()->info("\ttype = %v", object.mType);
            Logger::log()->info("\tfile = %v", object.mFile);
        }
    }
    
    void recursiveSearch(const boost::filesystem::path& root, 
            const std::string& extension, 
            std::vector<boost::filesystem::path>& results, 
            const std::vector<boost::filesystem::path>* ignore_list = nullptr) {
            
        if (!boost::filesystem::exists(root)) {
            return;
        }
        
        boost::filesystem::directory_iterator iter_end;
        for (boost::filesystem::directory_iterator iter(root); 
                iter != iter_end; ++iter) {
            boost::filesystem::path subdir = *iter;
            if (boost::filesystem::is_directory(subdir)) {
                bool ignore = false;
                if (ignore_list) {
                    for (const boost::filesystem::path& ignored_dir : 
                            *ignore_list) {
                        if (boost::filesystem::equivalent(subdir, 
                                ignored_dir)) {
                            ignore = true;
                            break;
                        }
                    }
                }
                if (!ignore) {
                    recursiveSearch(subdir, extension, results, ignore_list);
                }
            }
            else {
                if (subdir.has_filename() && subdir.extension() == extension) {
                    results.push_back(subdir);
                }
            }
        }
    }
    
    void load_package(const std::string& filename) {
        Logger::log()->info("Processing: %v", filename);
        m_package_file = filename;
        if (!boost::filesystem::exists(m_package_file)) {
            std::stringstream sss;
            sss << "Package file \""
                << m_package_file << "\" does not exist!";
            throw std::runtime_error(sss.str());
        }
        m_package_dir = m_package_file.parent_path();
        m_package_json = readJsonFile(m_package_file.string());
    }
    
    void load_config() {
        // Try read configuration
        uconf = parse_config(m_package_dir / "compile.config");
        
        // Print configuration data
        Logger::log()->info("Configuration:");
        Logger::log()->info("\tOutput dir: %v", uconf.outputDir);
        if (!uconf.intermediateDir.empty()) {
            Logger::log()->info("\tIntermediate dir: %v", 
                    uconf.intermediateDir);
        } else {
            Logger::log()->info("\tIntermediate data not used");
        }
        if (uconf.obfuscate) {
            Logger::log()->info("\tObfuscation: enabled");
        } else {
            Logger::log()->info("\tObfuscation: disabled");
        }
    }
    
    void prepare_output_dir() {
        if (boost::filesystem::exists(uconf.outputDir)) {
            if (!uconf.forceOverwriteOutput) {
                Logger::log()->warn("Output directory %v already exists!", 
                        uconf.outputDir);
                bool decided = false;
                bool decision;
                while (!decided) {
                    Logger::log()->info("Overwrite? (y/n)");
                    std::string input;
                    std::cin >> input;
                    char a = *input.begin();
                    if (a == 'y') {
                        decided = true;
                        decision = true;
                    }
                    else if (a == 'n') {
                        decided = true;
                        decision = false;
                    }
                }
                
                // Overwrite
                if (!decision) {
                    throw std::runtime_error("Chose not to erase output dir");
                }
            }

            boost::filesystem::directory_iterator directoryEnd;
            for (boost::filesystem::directory_iterator dirIter(uconf.outputDir);
                    dirIter != directoryEnd; ++ dirIter) {
                boost::filesystem::remove_all(*dirIter);
            }
        }
        boost::filesystem::create_directories(uconf.outputDir);
    }
    
    void locate_resources() {
        Logger::log()->info("Searching for resources...");
        recursiveSearch(m_package_dir, 
                ".resource", objectFiles, &uconf.ignoreDirs);
        recursiveSearch(m_package_dir, 
                ".resources", objectFiles, &uconf.ignoreDirs);
        
        Logger::log()->info("Found %v resource declaration files.",
                objectFiles.size());
    }
    
    void parse_resource_declaration_files() {
        for (boost::filesystem::path& res_decl_file : objectFiles) {
            Json::Value json_res_decl = readJsonFile(res_decl_file.string());
            
            std::vector<const Json::Value*> to_parse;
            if (json_res_decl.isArray()) {
                for (const Json::Value& subData : json_res_decl) {
                    if (!subData.isObject()) {
                        Logger::log()->warn("Resource declared in %v is not "
                                "valid, (value = %v)",
                                res_decl_file,
                                subData.toStyledString());
                        continue;
                    }
                    to_parse.push_back(&subData);
                }
            }
            else if (json_res_decl.isObject()) {
                to_parse.push_back(&json_res_decl);
            }
            else {
                Logger::log()->warn("Resource declared in %v is not "
                        "valid, (value = %v)",
                        res_decl_file,
                        json_res_decl.toStyledString());
            }
            
            for (const Json::Value* json_obj_ptr : to_parse) {
                const Json::Value& json_obj = *json_obj_ptr;
                try {
                    parseObject(json_obj, res_decl_file);
                } catch (std::runtime_error e) {
                    std::stringstream sss;
                    sss << "Error while parsing resource delcared in "
                        << res_decl_file << ": "
                        << e.what();
                    Logger::log()->warn("Could not parse resource "
                            "declared in %v, (value = %v): %v",
                            res_decl_file,
                            json_obj.toStyledString(),
                            e.what());
                }
            }
        }
    }
    
    void detect_naming_conflicts() {
        typedef std::vector<boost::filesystem::path> PathList;
        typedef std::pair<std::string, PathList> NameObjectListPair;
        typedef std::vector<NameObjectListPair> ObjectMap;
        ObjectMap objectMap;
        
        bool foundErrors = false;
        for (Object& exam : objects) {
            PathList* pathList = nullptr;
            for (NameObjectListPair& pair : objectMap) {
                
                // Oh no!
                if (pair.first == exam.mName) {
                    pathList = &pair.second;
                    break;
                }
            }
            
            if (pathList) {
                foundErrors = true;
                pathList->push_back(exam.mDebugOrigin);
            } else {
                NameObjectListPair nolp;
                nolp.first = exam.mName;
                nolp.second.push_back(exam.mDebugOrigin);
                objectMap.push_back(nolp);
            }
        }
        
        if (foundErrors) {
            std::stringstream sss;
            for (NameObjectListPair& pair : objectMap) {
                
                if (pair.second.size() > 1) {
                    sss << "Fatal! Detected naming conflict for resource \"" 
                        << pair.first << "\"\n\tOffending declaration files:\n";
                    for (boost::filesystem::path& file : pair.second) {
                        sss << "\t" << file << "\n";
                    }
                }
            }
            throw std::runtime_error(sss.str());
        }
        else {
            Logger::log()->info("No naming conflicts detected!");
        }
    }
    
    void determine_final_output_names() {
        uint32_t seqName = 0;
        for (Object& object : objects) {

            boost::filesystem::path outputObjectFile = uconf.outputDir;
            std::stringstream ss;
            if (uconf.obfuscate) {
                ss << seqName;
            }
            else {
                ss << object.mName;
            }
            outputObjectFile /= ss.str();

            object.mOutputFile = outputObjectFile;
            
            seqName ++;
        }
    }
    
    void use_previous_intermediates() {
        if (!uconf.intermediateDir.empty()) {
            if (!boost::filesystem::exists(uconf.intermediateDir)) {
                boost::filesystem::create_directories(uconf.intermediateDir);
            }
            
            Logger::log()->info("Calculating hashes...");
            for (Object& object : objects) {
                
                if (object.mAlwaysRetranslate) {
                    continue;
                }

                std::ifstream sizeTest(object.mFile.string().c_str(), 
                        std::ios::binary | std::ios::ate);
                object.mOriginalSize = sizeTest.tellg();

                char* totalData = new char[object.mOriginalSize];
                sizeTest.seekg(0, std::ios::beg);
                sizeTest.read(totalData, object.mOriginalSize);
                sizeTest.close();

                MurmurHash3_x86_32(totalData, object.mOriginalSize, 1337, 
                        &(object.mOriginalHash));

                delete[] totalData;
            }

            Logger::log()->info("Checking for pre-compiled data...");

            boost::filesystem::path intermediateFile = uconf.intermediateDir 
                    / "intermediate.data";

            if (!boost::filesystem::exists(intermediateFile)) {
                Logger::log()->info("None exists!");
            }
            else {
                Json::Value previousCompilation = 
                        readJsonFile(intermediateFile.string());

                const Json::Value& metadataList = 
                        previousCompilation["metadata"];
                for (Object& object : objects) {
                    /*
                     * If a pre-compiled file exists with exactly the same:
                     *  Input file hash
                     *  Configuration:
                     *      Type
                     *      Parameters
                     *
                     * Then simply copy the data into the output folder rather 
                     * than translate it again. (Therefore remove that object 
                     * from the list of objects to translate.)
                     */
                    
                    if (object.mAlwaysRetranslate) {
                        continue;
                    }

                    for (const Json::Value& metadata : metadataList) {

                        uint32_t checkHash = 
                                (uint32_t) (metadata["hash"].asInt64());
                        OType checkType = metadata["type"].asString();
                        const Json::Value& checkParams = metadata["params"];
                        
                        if (checkHash == object.mOriginalHash 
                                && checkType == object.mType 
                                && equivalentJson(checkParams, object.mParams) 
                                && !isWorkInProgressType(object.mType)) {
                            
                            if (n_verbose) {
                                Logger::log()->info("\tCopy: %v", object.mName);
                            }
                            object.mSkipTranslate = true;

                            boost::filesystem::copy(uconf.intermediateDir 
                                    / (metadata["file"].asString()), 
                                    object.mOutputFile);
                            std::ifstream sizeTest(
                                    object.mOutputFile.string().c_str(), 
                                    std::ios::binary | std::ios::ate);
                            object.mOutputSize = sizeTest.tellg();

                            break;
                        }
                    }
                }
            }
            std::cout << std::endl;
        }
    }
    
    void translate_all_data() {
        Logger::log()->info("Translating all resources...");
        boost::filesystem::path intermediateFile = uconf.intermediateDir 
                / "intermediate.data";
        Json::Value intermediateData;
        uint32_t metadataIndex = 0;
        if (!uconf.intermediateDir.empty()) {
            if (boost::filesystem::exists(intermediateFile)) {
                intermediateData = readJsonFile(intermediateFile.string());
                metadataIndex = intermediateData["metadata"].size();
            }
        }

        uint64_t totalSize = 0;
        uint32_t numUpdates = 0;
        uint32_t numFails = 0;

        // Append the file provided by user
        Json::Value outputPackageData = m_package_json;
        Json::Value& objectListData = outputPackageData["resources"];
        uint32_t jsonListIndex = 0;

        for (Object& object : objects) {

            if (n_verbose) {
                std::cout << object.mName << " [" << object.mType << "]" << std::endl;
            }

            if (!object.mSkipTranslate || isWorkInProgressType(object.mType)) {
                std::cout << "\t->";
                
                // If verbose output is enabled, then the object name has already been printed
                if (!n_verbose) {
                    std::cout << " " << object.mName << " [" << object.mType << "]";
                }
                std::cout << "..." << std::endl;

                if (isWorkInProgressType(object.mType)) {
                    std::cout << "\t(WIP)" << std::endl;
                }
                try {
                    translateData(object, !uconf.obfuscate);
                }
                catch (std::runtime_error e) {
                    ++ numFails;
                    Logger::log()->warn("%v failed to translate: %v", 
                            object.mName, e.what());
                }
                
                std::cout << std::endl;
                std::ifstream sizeTest(object.mOutputFile.string().c_str(), 
                        std::ios::binary | std::ios::ate);
                object.mOutputSize = sizeTest.tellg();
                ++ numUpdates;
                if (!uconf.intermediateDir.empty()) {
                    Json::Value& objectMetadata = 
                            intermediateData["metadata"][metadataIndex];

                    objectMetadata["hash"] = 
                            (Json::Int64) (object.mOriginalHash);
                    objectMetadata["type"] = object.mType;
                    objectMetadata["params"] = object.mParams;

                    std::stringstream ss;
                    ss << object.mName;
                    ss << object.mType;
                    ss << "-";
                    ss << ((uint32_t) (object.mOriginalHash));
                    ss << ".i";
                    std::string intermFilename = ss.str();

                    objectMetadata["file"] = intermFilename;

                    // copy file
                    boost::filesystem::path copyTo = uconf.intermediateDir 
                            / intermFilename;
                    if (boost::filesystem::exists(copyTo)) {
                        boost::filesystem::remove(copyTo);
                    }
                    boost::filesystem::copy(object.mOutputFile, copyTo);

                    ++ metadataIndex;
                }
            }
            //
            Json::Value& objectDef = objectListData[jsonListIndex];
            objectDef["name"] = object.mName;
            objectDef["type"] = object.mType;
            objectDef["file"] = object.mOutputFile.filename().string().c_str();
            objectDef["size"] = object.mOutputSize;

            totalSize += object.mOutputSize;

            ++ jsonListIndex;
        }
        std::cout << std::endl;
        std::cout << numUpdates << " file(s) translated" << std::endl;
        std::cout << numFails << " file(s) failed" << std::endl;
        std::cout << std::endl;

        if (!uconf.intermediateDir.empty()) {
            std::cout << "Exporting intermediate.data... ";
            writeJsonFile(intermediateFile.string(), intermediateData, true);
            std::cout << "Done!" << std::endl;
            std::cout << std::endl;
        }

        std::cout << "Exporting data.package... ";

        Json::Value& metricsData = outputPackageData["metrics"];
        metricsData["size"] = (Json::UInt64) totalSize;
        writeJsonFile((uconf.outputDir / "data.package").string(), 
                outputPackageData);
        std::cout << "Done!" << std::endl;
        std::cout << std::endl;
    }

public:

    bool process(std::string filename) {
        try {
            load_package(filename);
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while loading package file: "
                << e.what();
            throw std::runtime_error(sss.str());
        }
        try {
            load_config();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while loading config: "
                << e.what();
            throw std::runtime_error(sss.str());
        }
        try {
            prepare_output_dir();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while preparing output dir: "
                << e.what();
            throw std::runtime_error(sss.str());
        }
        try {
            locate_resources();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while locating resource definitions: "
                << e.what();
            throw std::runtime_error(sss.str());
        }
        try {
            parse_resource_declaration_files();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while parsing resource declaration files: "
                << e.what();
            throw std::runtime_error(sss.str());
        }
        try {
            detect_naming_conflicts();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while detecting naming conflicts: "
                << e.what();
            throw std::runtime_error(sss.str());
        }
        try {
            determine_final_output_names();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while determining final output names: "
                << e.what();
            throw std::runtime_error(sss.str());
        }
        try {
            use_previous_intermediates();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while using intermediates: "
                << e.what();
            throw std::runtime_error(sss.str());
        }
        try {
            translate_all_data();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while loading package file: "
                << e.what();
            throw std::runtime_error(sss.str());
        }
    }
};

} // namespace resman

using namespace resman;

int main(int argc, char* argv[]) {
    Logger::initialize();
    if (argc <= 1) {
        Logger::log()->warn(
                "Error: must supply path to package definition file!");
        Logger::log()->warn("Usage: %v <path to .package file>", argv[0]);
        return 0;
    }
    
    Project project;
    try {
        project.process(argv[1]);
    } catch (std::runtime_error e) {
        Logger::log()->fatal(e.what());
    }
    Logger::cleanup();
    return 0;
}
