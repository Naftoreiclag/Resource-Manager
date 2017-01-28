#include "Convert.hpp"

#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <sstream>

//#include <spirv-tools/libspirv.h>

#include "StreamWrite.hpp"

/* Bytecode is compiled using SPIRV
 * 
 * TODO:
 * Convert a human-readable glsl file into bytecode, compilable source, or both (in that order).
 * The combination is determined by the header's magic words:
 *      ASSEMB
 *      SOURCE
 *      SOUEMB (followed by one 32-bit unsigned integer describing the length of the assembly)
 * 
 */
void convertGlsl(const boost::filesystem::path& inputFilename, const boost::filesystem::path& outputFilename, const Json::Value& params, bool modifyFilename) {
    bool paramBytecodeInclude = true;
    //paramBytecodeOptimizationLevel = 1;
    
    bool paramSourceInclude = false;
    bool paramSourceStripComments = false;
    
    {
        {
            const Json::Value& jsonBytecodeParams = params["assemble"];
            if(jsonBytecodeParams.isObject()) {
                {
                    const Json::Value& jsonInclude = jsonBytecodeParams["include"];
                    if(jsonInclude.isBool()) paramBytecodeInclude = jsonInclude.asBool();
                }
            }
        }
        
        {
            const Json::Value& jsonSourceParams = params["source"];
            if(jsonSourceParams.isObject()) {
                {
                    const Json::Value& jsonInclude = jsonSourceParams["include"];
                    if(jsonInclude.isBool()) paramSourceInclude = jsonInclude.asBool();
                }
                {
                    const Json::Value& jsonStrip = jsonSourceParams["strip-comments"];
                    if(jsonStrip.isBool()) paramSourceStripComments = jsonStrip.asBool();
                }
            }
        }
    }
    
    if(!paramBytecodeInclude && !paramSourceInclude) {
        std::cout << "\tError! Neither bytecode nor source is being exported!" << std::endl;
        return;
    }
    
    std::stringstream ss;
    ss << "glslangValidator ";
    ss << inputFilename.string();
    ss << " -V -o ";
    ss << outputFilename.string();
    //std::cout << ss.str() << std::endl;
    std::system(ss.str().c_str());
    
    /* SPV_ENV_UNIVERSAL_1_0
     * SPV_ENV_VULKAN_1_0
     * SPV_ENV_UNIVERSAL_1_1
     */
     
     /*
    spv_context iContext = spvContextCreate(SPV_ENV_VULKAN_1_0);
    spv_binary oBinary;
    spv_diagnostic oDiagnostic;
    
    std::ifstream inputFile(inputFilename.string().c_str(), std::ios::in | std::ios::binary);
    std::vector<char> inputData;
    char byte;
    while(true) {
        inputFile.read(&byte, 1);
        if(inputFile.eof()) {
            break;
        }
        inputData.push_back(byte);
    }
    
    spvTextToBinary(iContext, inputData.data(), inputData.size(), &oBinary, &oDiagnostic);
    spvDiagnosticPrint(oDiagnostic);
    
    std::ofstream outputFile(outputFilename.string().c_str(), std::ios::out | std::ios::binary);
    
    for(uint32_t i = 0; i < oBinary->wordCount; ++ i) {
        writeU32(outputFile, oBinary->code[i]);
    }
    
    spvContextDestroy(iContext);
    */
    
    
    
    // TODO: something with the params
    
}
