#include "Convert.hpp"

#include <iostream>

#include "vorbis/vorbisfile.h"
#include "vorbis/codec.h"

void convertWaveform(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {
    
    OggVorbis_File ovFile;
    
    int error = ov_fopen(fromFile.string().c_str(), &ovFile);
    
    if(error) {
        std::cout << "\tFailed to read waveform!" << std::endl;
        return;
    }
    
    // Print various file data
    {
        // comments
        {
            const vorbis_comment* commentData = ov_comment(&ovFile, -1);
            char** comments = commentData->user_comments;
            int numComments = commentData->comments;
            std::cout << "\tVorbis comments:" << std::endl;
            for(int i = 0; i < numComments; ++ i) {
                std::cout << "\t\t" << comments[i] << std::endl;
            }
        }
        
        // other
        {
            const vorbis_info* vorbisInfo = ov_info(&ovFile, -1);
            std::cout << "\tVorbis encoder version: " << vorbisInfo->version << std::endl;
            std::cout << "\tNumber of channels: " << vorbisInfo->channels << std::endl;
            std::cout << "\tSampling rate: " << vorbisInfo->rate << "hz" << std::endl;
        }
    }
    
    ov_clear(&ovFile);
    
    boost::filesystem::copy_file(fromFile, outputFile);
}
