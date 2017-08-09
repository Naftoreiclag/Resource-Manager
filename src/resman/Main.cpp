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
#include "main/Common.hpp"
#include "main/Expand.hpp"

namespace resman {

// Useful for debug information, but significantly slows down packaging
bool n_verbose = true;
bool n_clean_output = false;

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
    
    {"bgfx-shader", convert_bgfx_shader},
    
    {"vertex-shader", convertGlsl},
    {"tess-control-shader", convertGlsl},
    {"tess-evaluation-shader", convertGlsl},
    {"geometry-shader", convertGlsl},
    {"fragment-shader", convertGlsl},
    {"compute-shader", convertGlsl},
    
    {"script", convertMiscellaneous},
    {"string", convertMiscellaneous}
};

std::map<OType, Expand_Func> n_expanders = {
    {"bgfx-shader", expand_bgfx_shader}
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
    bool m_obfuscate = false;
    std::vector<boost::filesystem::path> m_ignores;
    boost::filesystem::path m_output_dir;
    boost::filesystem::path m_interm_dir;
};

void translateData(const Object& object, bool modifyFilename) {
    if (!boost::filesystem::exists(object.m_src_file)) {
        std::stringstream sss;
        sss << "File does not exist: "
            << object.m_src_file.filename();
        throw std::runtime_error(sss.str());
    }
    
    auto pair_ptr = n_converters.find(object.m_type);
    
    if (pair_ptr == n_converters.end()) {
        std::stringstream sss;
        sss << "Unknown type: \""
            << object.m_type
            << "\"";
        throw std::runtime_error(sss.str());
    }
    
    Convert_Args args;
    args.fromFile = object.m_src_file;
    args.outputFile = object.m_interm_file;
    args.modifyFilename = modifyFilename;
    args.params = object.m_params;
    
    Convert_Func& converter = pair_ptr->second;
    converter(args);
    
    if (boost::filesystem::exists(object.m_dest_file)) {
        boost::filesystem::remove(object.m_dest_file);
    }
    boost::filesystem::copy(object.m_interm_file, object.m_dest_file);
}

void recursiveSearch(const boost::filesystem::path& root, 
        const std::string& extension, 
        std::vector<boost::filesystem::path>& results, 
        const std::vector<boost::filesystem::path>& ignore_list) {
        
    if (!boost::filesystem::exists(root)) {
        return;
    }
    
    boost::filesystem::directory_iterator iter_end;
    for (boost::filesystem::directory_iterator iter(root); 
            iter != iter_end; ++iter) {
        boost::filesystem::path subdir = *iter;
        if (boost::filesystem::is_directory(subdir)) {
            bool ignore = false;
            for (const boost::filesystem::path& ignored_dir : ignore_list) {
                if (boost::filesystem::equivalent(subdir, ignored_dir)) {
                    ignore = true;
                    break;
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

class Project {
private:
    Config m_conf;
    
    boost::filesystem::path m_package_file;
    boost::filesystem::path m_package_dir;
    
    Json::Value m_package_json;
    
    std::vector<Object> m_objects;
    
    void parse_config(boost::filesystem::path file_config) {
        m_conf.m_output_dir = m_package_dir / "__output__";
        m_conf.m_interm_dir = m_package_dir / "__interm__";
        
        if (!boost::filesystem::exists(file_config)) {
            return;
        }
        Json::Value json_config = readJsonFile(file_config.string());
        
        Json::Value& json_output = json_config["output"];
        if (!json_output.isNull()) {                
            m_conf.m_output_dir = m_package_dir / (json_output.asString());
        }
        
        Json::Value& json_obfuscate = json_config["obfuscate"];
        if (!json_obfuscate.isNull()) {
            m_conf.m_obfuscate = json_obfuscate.asBool();
        }

        Json::Value& json_interm = json_config["intermediate"];
        if (!json_interm.isNull()) {
            m_conf.m_interm_dir = m_package_dir / (json_interm.asString());
        }
        
        Json::Value& json_ignore_list = json_config["ignore"];
        if (!json_ignore_list.isNull()) {
            for (Json::Value& ignore : json_ignore_list) {
                m_conf.m_ignores.push_back(m_package_dir / (ignore.asString()));
            }
        }
    }
    
    void verify_obj_field(const char* field,
            const Json::Value& json_obj, 
            const boost::filesystem::path& resdef_file) {
        if (json_obj[field].isNull()) {
            std::stringstream sss;
            sss << "\""
                << field
                << "\" not specified in resource declared in "
                << resdef_file
                << " (value = "
                << json_obj.toStyledString()
                << ")";
            throw std::runtime_error(sss.str());
        }
    }
    
    void process_resource(const Json::Value& json_obj, 
            const boost::filesystem::path& resdef_file) {
        Object object;
        object.m_dbg_resdef = resdef_file;
        
        verify_obj_field("name", json_obj, resdef_file);
        verify_obj_field("type", json_obj, resdef_file);
        verify_obj_field("file", json_obj, resdef_file);
        
        object.m_name = json_obj["name"].asString();
        object.m_type = json_obj["type"].asString();
        object.m_src_file = resdef_file.parent_path()
                / json_obj["file"].asString();
        object.m_params = json_obj["parameters"];
        if (object.m_params.isNull()) {
            object.m_params = json_obj["params"];
        }
        
        const Json::Value& json_retrans = json_obj["always-retranslate"];
        if (!json_retrans.isNull()) {
            object.m_force_retrans = json_retrans.asBool();
        }
        if (isWorkInProgressType(object.m_type)) {
            object.m_force_retrans = true;
        }
        
        m_objects.push_back(object);
        
        if (n_verbose) {
            Logger::log()->info("Resource: name = %v", object.m_name);
            Logger::log()->info("\ttype = %v", object.m_type);
            Logger::log()->info("\tfile = %v", object.m_src_file);
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
        parse_config(m_package_dir / "compile.config");
        
        // Print configuration data
        Logger::log()->info("Configuration:");
        Logger::log()->info("\tOutput dir: %v", m_conf.m_output_dir);
        if (!m_conf.m_interm_dir.empty()) {
            Logger::log()->info("\tIntermediate dir: %v", 
                    m_conf.m_interm_dir);
        } else {
            Logger::log()->info("\tIntermediate data not used");
        }
        if (m_conf.m_obfuscate) {
            Logger::log()->info("\tObfuscation: enabled");
        } else {
            Logger::log()->info("\tObfuscation: disabled");
        }
    }
    
    void prepare_output_dir() {
        if (boost::filesystem::exists(m_conf.m_output_dir)) {
            if (n_clean_output) {
                boost::filesystem::directory_iterator directoryEnd;
                for (boost::filesystem::directory_iterator 
                        dirIter(m_conf.m_output_dir);
                        dirIter != directoryEnd; ++dirIter) {
                    boost::filesystem::remove_all(*dirIter);
                }
            }
        }
        boost::filesystem::create_directories(m_conf.m_output_dir);
    }
    
    void parse_resource_declaration_files() {
        std::vector<boost::filesystem::path> decl_files;
        
        Logger::log()->info("Searching for resources...");
        recursiveSearch(m_package_dir, 
                ".resource", decl_files, m_conf.m_ignores);
        recursiveSearch(m_package_dir, 
                ".resources", decl_files, m_conf.m_ignores);
        
        Logger::log()->info("Found %v resource declaration files.",
                decl_files.size());
        
        for (boost::filesystem::path& res_decl_file : decl_files) {
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
                    process_resource(json_obj, res_decl_file);
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
    
    void expand_resources() {
        std::vector<Object> added_objects;
        for(Object& obj : m_objects) {
            auto expander_iter = n_expanders.find(obj.m_type);
            if (expander_iter == n_expanders.end()) {
                continue;
            }
            
            std::vector<Expansion> expansions = expander_iter->second(obj);
            
            if (expansions.size() == 0) {
                continue;
            }
            obj.m_expanded = true;
            
            for (Expansion& expansion : expansions) {
                Object& exp_obj = expansion.m_obj;
                
                std::stringstream sss;
                sss << exp_obj.m_name
                    << '#'
                    << expansion.m_subtype;
                
                exp_obj.m_name = sss.str();
                
                added_objects.emplace_back(std::move(exp_obj));
            }
        }
        m_objects.erase(std::remove_if(m_objects.begin(), m_objects.end(), 
                [](const Object& obj)->bool{
                        return obj.m_expanded;
                }), m_objects.end());
        std::copy(added_objects.begin(), added_objects.end(), 
                std::back_inserter(m_objects));
    }
    
    void detect_naming_conflicts() {
        typedef std::vector<boost::filesystem::path> PathList;
        typedef std::pair<std::string, PathList> NameObjectListPair;
        typedef std::vector<NameObjectListPair> ObjectMap;
        ObjectMap objectMap;
        
        bool foundErrors = false;
        for (Object& exam : m_objects) {
            PathList* pathList = nullptr;
            for (NameObjectListPair& pair : objectMap) {
                
                // Oh no!
                if (pair.first == exam.m_name) {
                    pathList = &pair.second;
                    break;
                }
            }
            
            if (pathList) {
                foundErrors = true;
                pathList->push_back(exam.m_dbg_resdef);
            } else {
                NameObjectListPair nolp;
                nolp.first = exam.m_name;
                nolp.second.push_back(exam.m_dbg_resdef);
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
        for (Object& object : m_objects) {
            
            boost::filesystem::path outputObjectFile = m_conf.m_output_dir;
            std::stringstream ss;
            if (m_conf.m_obfuscate) {
                ss << seqName;
                ++seqName;
            }
            else {
                ss << object.m_name;
            }
            outputObjectFile /= ss.str();

            object.m_dest_file = outputObjectFile;
        }
    }
    
    void hash_file(const boost::filesystem::path& file, uint32_t& src_hash) {
        std::ifstream sizeTest(file.string().c_str(), 
                std::ios::binary | std::ios::ate);
        int src_size = sizeTest.tellg();

        char* totalData = new char[src_size];
        sizeTest.seekg(0, std::ios::beg);
        sizeTest.read(totalData, src_size);
        sizeTest.close();

        MurmurHash3_x86_32(totalData, src_size, 0xdaff0d11, &src_hash);

        delete[] totalData;
    }
    
    /**
     * @brief Not a perfect hash (For some inputs, equal jsons does not imply 
     * equal hashes and unequal hashes does not imply unequal jsons) However,
     * for most cases this works good enough.
     * @param val
     * @param hash
     */
    void hash_json(const Json::Value& val, uint32_t& hash) {
        std::string string_json = val.toStyledString();
        MurmurHash3_x86_32(string_json.c_str(), string_json.size(), 0xdaff0d11,
                &hash);
    }
    
    std::string generate_interm_code(const Object& object) {
        std::stringstream sss;
        sss << object.m_name;
        sss << "|||";
        sss << object.m_type;
        sss << "|||";
        sss << object.m_src_hash;
        sss << "|||";
        sss << object.m_params_hash;
        return sss.str();
    }
    
    Json::Value m_json_interm;
    boost::filesystem::path m_interm_file;
    void use_previous_intermediates() {
        if (!boost::filesystem::exists(m_conf.m_interm_dir)) {
            boost::filesystem::create_directories(m_conf.m_interm_dir);
        }
        
        m_interm_file = m_conf.m_interm_dir / "intermediate.data";
        if (!boost::filesystem::exists(m_interm_file)) {
            m_json_interm["next-idx"] = 0;
        } else {
            m_json_interm = readJsonFile(m_interm_file.string());
        }

        Logger::log()->info("Checking for pre-compiled data...");

        Json::Value& json_metadatas = m_json_interm["metadata"];
        Json::Value& json_next_idx = m_json_interm["next-idx"];
        uint64_t next_idx = json_next_idx.asUInt64();
        for (Object& object : m_objects) {
            hash_file(object.m_src_file, object.m_src_hash);
            hash_json(object.m_params, object.m_params_hash);
            
            /*
             * If a pre-compiled file exists with exactly the same:
             *  Src file hash
             *  Resource type
             *  Compilation params hash
             *  Compilation params
             *
             * Then simply copy the data into the output folder rather 
             * than translate it again. (Therefore remove that object 
             * from the list of objects to translate.)
             */
            
            std::string interm_code = generate_interm_code(object);
            Json::Value& json_metadata = json_metadatas[interm_code.c_str()];
            
            if (!object.m_force_retrans && !json_metadata.isNull()
                    && equivalentJson(
                            object.m_params, json_metadata["params"])) {
                if (n_verbose) {
                    Logger::log()->info("\tCopy: %v", object.m_name);
                }
                
                object.m_skip_retrans = true;
                object.m_interm_file = m_conf.m_interm_dir 
                        / (json_metadata["file"].asString());
                
            } else {
                if (object.m_force_retrans) {
                    boost::filesystem::path old_interm = m_conf.m_interm_dir 
                            / (json_metadata["file"].asString());
                    boost::filesystem::remove(old_interm);
                }
                
                std::stringstream sss;
                sss << next_idx << ".r";
                
                object.m_interm_file = m_conf.m_interm_dir / sss.str();
                
                ++next_idx;
            }
        }
        
        json_next_idx = next_idx;
    }
    
    void process_all_resources() {
        Logger::log()->info("Processing all resources...");

        uint64_t totalSize = 0;
        uint32_t num_skips = 0;
        uint32_t num_converts = 0;
        uint32_t num_fails = 0;

        // Append the file provided by user
        Json::Value json_output_pkg = m_package_json;
        Json::Value& json_res_list = json_output_pkg["resources"];
        uint32_t jsonListIndex = 0;

        Json::Value& json_interm_metadatas = m_json_interm["metadata"];
        for (Object& object : m_objects) {

            if (n_verbose) {
                std::cout << object.m_name << " [" << object.m_type << "]" << std::endl;
            }

            if (!object.m_skip_retrans) {
                std::cout << "\t->";
                
                // If verbose output is enabled, then the object name has already been printed
                if (!n_verbose) {
                    std::cout << " " << object.m_name << " [" << object.m_type << "]";
                }
                std::cout << "..." << std::endl;
                try {
                    translateData(object, !m_conf.m_obfuscate);
                }
                catch (std::runtime_error e) {
                    ++num_fails;
                    Logger::log()->warn("%v failed to translate: %v", 
                            object.m_name, e.what());
                    continue;
                }
                ++num_converts;
            } else {
                ++num_skips;
            }
            
            std::string interm_code = generate_interm_code(object);
            Json::Value& json_interm_metadata = 
                    json_interm_metadatas[interm_code.c_str()];
            json_interm_metadata["params"] = object.m_params;
            json_interm_metadata["file"] = 
                    object.m_interm_file.filename().string().c_str();
            
            

            if (boost::filesystem::exists(object.m_dest_file)) {
                boost::filesystem::remove(object.m_dest_file);
            }
            boost::filesystem::copy(object.m_interm_file, 
                    object.m_dest_file);
            std::ifstream sizeTest(
                    object.m_dest_file.string().c_str(), 
                    std::ios::binary | std::ios::ate);
            object.m_dest_size = sizeTest.tellg();
            //
            Json::Value& json_obj_def = json_res_list[jsonListIndex];
            json_obj_def["name"] = object.m_name;
            json_obj_def["type"] = object.m_type;
            json_obj_def["file"] = object.m_dest_file.filename().string().c_str();
            json_obj_def["size"] = object.m_dest_size;

            totalSize += object.m_dest_size;

            ++jsonListIndex;
        }
        std::cout << std::endl;
        std::cout << num_skips << " file(s) already built" << std::endl;
        std::cout << num_converts << " file(s) translated" << std::endl;
        std::cout << num_fails << " file(s) failed" << std::endl;
        std::cout << std::endl;

        std::cout << "Exporting intermediate.data... ";
        writeJsonFile(m_interm_file.string(), m_json_interm, true);
        std::cout << "Done!" << std::endl;
        std::cout << std::endl;

        std::cout << "Exporting data.package... ";

        Json::Value& metricsData = json_output_pkg["metrics"];
        metricsData["size"] = (Json::UInt64) totalSize;
        writeJsonFile((m_conf.m_output_dir / "data.package").string(), 
                json_output_pkg, true);
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
            parse_resource_declaration_files();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while parsing resource declaration files: "
                << e.what();
            throw std::runtime_error(sss.str());
        }
        try {
            expand_resources();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while expanding resources files: "
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
            process_all_resources();
        } catch (std::runtime_error e) {
            std::stringstream sss;
            sss << "Error while processing resources: "
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
