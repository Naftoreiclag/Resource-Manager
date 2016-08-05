#include "Convert.hpp"

#include <iostream>
#include <vector>

#include "vorbis/vorbisfile.h"
#include "vorbis/vorbisenc.h"
#include "vorbis/codec.h"
#include "FLAC/stream_decoder.h"

// TODO: add support for multiple channels

struct FlacUserData {
    
};

FLAC__StreamDecoderWriteStatus flacDecoderWriteCallback(
    const FLAC__StreamDecoder* inputFlacDecoder,
    const FLAC__Frame* inputFrame,
    const FLAC__int32* const inputBuffer[],
    void* userData) {
    
}

void flacDecoderMetadataCallback(
    const FLAC__StreamDecoder* inputFlacDecoder,
    const FLAC__StreamMetadata* inputMetadata,
    void* userData) {
    
}

void flacDecoderErrorCallback(
    const FLAC__StreamDecoder* inputFlacDecoder,
    const FLAC__StreamDecoderErrorStatus errorStatus,
    void* userData) {
    
}

enum WaveformEncodingMode {
    VBR,
    ABR,
    CBR
};

enum WaveformInputFileType {
    OGG_VORBIS,
    FLAC
};

struct TagPair {
    TagPair(std::string t, std::string d)
    : tag(t)
    , data(d) {
    }
    
    std::string tag;
    std::string data;
};

void convertWaveform(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {
    
    // Get data from params
    WaveformEncodingMode paramWaveformEncodingMode = WaveformEncodingMode::VBR;
    float paramVbrQuality = 1.f;
    std::vector<TagPair> paramTags;
    if(!params.isNull()) {
        const Json::Value& encMode = params["encoding-mode"];
        if(!encMode.isNull()) {
            std::string choice = encMode.asString();
            if(choice == "VBR") {
                paramWaveformEncodingMode = WaveformEncodingMode::VBR;
            } else if(choice == "ABR") {
                paramWaveformEncodingMode = WaveformEncodingMode::ABR;
            } else if(choice == "CBR") {
                paramWaveformEncodingMode = WaveformEncodingMode::CBR;
            }
        }
        
        const Json::Value& vbrQuality = params["vbr-quality"];
        if(!vbrQuality.isNull()) {
            paramVbrQuality = vbrQuality.asFloat();
            if(paramVbrQuality > 1.f) {
                paramVbrQuality = 1.f;
                std::cout << "\tVBR quality clamped to maximum of 1.0" << std::endl;
            } else if(paramVbrQuality < -0.1f) {
                paramVbrQuality = -0.1f;
                std::cout << "\tVBR quality clamped to minimum of -0.1" << std::endl;
            }
        }
        
        const Json::Value& tags = params["metadata"];
        if(!tags.isNull()) {
            for(Json::Value::const_iterator iter = tags.begin(); iter != tags.end(); ++ iter) {
                const Json::Value& key = iter.key();
                const Json::Value& value = *iter;
                
                paramTags.push_back(TagPair(key.asString(), value.asString()));
            }
        }
    }
    
    // Input file type, ogg-vorbis by default
    // If the vorbis reader determins that the file is not an ogg-vorbis, assume it is FLAC instead
    WaveformInputFileType paramWaveformInputFileType = WaveformInputFileType::OGG_VORBIS;
    
    // Attempt to read it as ogg-vorbis
    OggVorbis_File inputVorbisFile;
    int errorCode;
    errorCode = ov_fopen(fromFile.string().c_str(), &inputVorbisFile);
    if(errorCode) {
        if(errorCode == OV_ENOTVORBIS) {
            // Assume it is FLAC instead
            paramWaveformInputFileType = WaveformInputFileType::FLAC;
            ov_clear(&inputVorbisFile);
        } else {
            // Something else (fatal) went wrong
            std::cout << "\tFailed to read ogg-vorbis file!" << std::endl;
            return;
        }
    }
    
    
    switch(paramWaveformInputFileType) {
        case WaveformInputFileType::OGG_VORBIS: {
            const vorbis_info* inputVorbisInfo = ov_info(&inputVorbisFile, -1);
            // Print various file data
            {
                // comments
                {
                    const vorbis_comment* commentData = ov_comment(&inputVorbisFile, -1);
                    char** comments = commentData->user_comments;
                    int numComments = commentData->comments;
                    std::cout << "\tVorbis comments:" << std::endl;
                    for(int i = 0; i < numComments; ++ i) {
                        std::cout << "\t" << i << ".\t" << comments[i] << std::endl;
                    }
                }
                
                // other
                {
                    std::cout << "\tVorbis encoder version: " << inputVorbisInfo->version << std::endl;
                    std::cout << "\tNumber of channels: " << inputVorbisInfo->channels << std::endl;
                    std::cout << "\tSampling rate: " << inputVorbisInfo->rate << "hz" << std::endl;
                }
            }
            
            // Write intialization
            vorbis_info outputVorbisInfo;
            vorbis_info_init(&outputVorbisInfo);
            int outputNumChannels = 1;
            errorCode = vorbis_encode_init_vbr(&outputVorbisInfo, outputNumChannels, inputVorbisInfo->rate, paramVbrQuality);
            if(errorCode) {
                std::cout << "Failed to initialize encoding for VBR!" << std::endl;
                return;
            }
            
            vorbis_dsp_state outputVorbisDspState;
            errorCode = vorbis_analysis_init(&outputVorbisDspState, &outputVorbisInfo);
            if(errorCode) {
                std::cout << "Failed to initialize vorbis dsp state!" << std::endl;
                return;
            }
            
            vorbis_comment outputVorbisMetadata;
            vorbis_comment_init(&outputVorbisMetadata);
            
            for(std::vector<TagPair>::iterator iter = paramTags.begin(); iter != paramTags.end(); ++ iter) {
                TagPair& tagPair = *iter;
                vorbis_comment_add_tag(&outputVorbisMetadata, tagPair.tag.c_str(), tagPair.data.c_str());
            }
            
            //
            vorbis_block outputVorbisBlock;
            vorbis_block_init(&outputVorbisDspState, &outputVorbisBlock);
            
            int cerealNumber = 1337; // Why?
            
            ogg_stream_state outputOggStream;
            ogg_stream_init(&outputOggStream, cerealNumber);
            
            ogg_page outputOggPage;
            
            std::ofstream outputData(outputFile.string().c_str(), std::ios::out | std::ios::binary);
            
            {
                ogg_packet streamIdentity;
                ogg_packet serializedMetadata;
                ogg_packet codebooks;
                
                vorbis_analysis_headerout(&outputVorbisDspState, &outputVorbisMetadata, &streamIdentity, &serializedMetadata, &codebooks);
                
                ogg_stream_packetin(&outputOggStream, &streamIdentity);
                ogg_stream_packetin(&outputOggStream, &serializedMetadata);
                ogg_stream_packetin(&outputOggStream, &codebooks);
                
                while(true) {
                    int ret = ogg_stream_flush(&outputOggStream, &outputOggPage);
                    if(ret == 0) {
                        break;
                    }
                    outputData.write(reinterpret_cast<char*>(outputOggPage.header), outputOggPage.header_len);
                    outputData.write(reinterpret_cast<char*>(outputOggPage.body), outputOggPage.body_len);
                }
            }
            
            // Reading information
            int logicalBitstreamPtr;
            int bigEndian = 0; // 0 = little endian, 1 = big endian
            int wordSize = 2; // number of bytes per word
            int signage = 1; // 0 = unsigned, 1 = signed
            char inputBuffer[4096 * wordSize];
            
            // Debug info
            int totalSampleNum = 0;
            
            while(true) {
                int ovRet = ov_read(&inputVorbisFile, inputBuffer, sizeof(inputBuffer), bigEndian, wordSize, signage, &logicalBitstreamPtr);
                
                // End of file
                if(ovRet == 0) {
                    // Mark this as the end of the stream
                    vorbis_analysis_wrote(&outputVorbisDspState, 0);
                    
                    break;
                }
                // Error
                else if(ovRet < 0) {
                    std::cout << "\tRead error in ogg-vorbis decoder!" << std::endl;
                    return;
                }
                // Positive return values are the number of bytes read
                else {
                    // (Past tense "read")
                    int numSamplesRead = ovRet / wordSize;
                    
                    // Get a buffer which we must write to
                    float** outputBuffers = vorbis_analysis_buffer(&outputVorbisDspState, numSamplesRead);
                    
                    for(int i = 0; i < numSamplesRead; ++ i) {
                        int16_t sampleData = 0;
                        
                        // Little-endian decoding
                        sampleData = (inputBuffer[i * 2] & 0x00ff) | (inputBuffer[i * 2 + 1] << 8);
                        
                        // only one channel is supported right now
                        // 32768 = 2^15
                        outputBuffers[0][i] = sampleData / 32768.f;
                    }
                    
                    // Tell vorbis that we are done writing to the buffer gained previously
                    // (Necessary because we technically could have submitted less data than was requested,
                    // and also because we need to notify vorbis that we are done writing for now)
                    vorbis_analysis_wrote(&outputVorbisDspState, numSamplesRead);
                    
                    // Debug stuff
                    totalSampleNum += numSamplesRead;
                }
                
                //
                while(vorbis_analysis_blockout(&outputVorbisDspState, &outputVorbisBlock) == 1) {
                    vorbis_analysis(&outputVorbisBlock, NULL);
                    vorbis_bitrate_addblock(&outputVorbisBlock);
                    
                    ogg_packet outputOggPacket;
                    while(vorbis_bitrate_flushpacket(&outputVorbisDspState, &outputOggPacket)) {
                        ogg_stream_packetin(&outputOggStream, &outputOggPacket);
                        while(true) {
                            int ret = ogg_stream_pageout(&outputOggStream, &outputOggPage);
                            if(ret == 0) {
                                break;
                            }
                            outputData.write(reinterpret_cast<char*>(outputOggPage.header), outputOggPage.header_len);
                            outputData.write(reinterpret_cast<char*>(outputOggPage.body), outputOggPage.body_len);
                        }
                    }
                }
            }
            std::cout << "\tNumber of samples: " << totalSampleNum << std::endl;
            
            // Reading cleanup
            ov_clear(&inputVorbisFile);
            
            // Writing cleanup
            ogg_stream_clear(&outputOggStream);
            vorbis_block_clear(&outputVorbisBlock);
            vorbis_comment_clear(&outputVorbisMetadata);
            vorbis_dsp_clear(&outputVorbisDspState);
            vorbis_info_clear(&outputVorbisInfo);
            return;
        }
        case WaveformInputFileType::FLAC: {
            FLAC__StreamDecoder* inputFlacDecoder = FLAC__stream_decoder_new();
            
            if(!inputFlacDecoder) {
                std::cout << "\tFailed to allocate FLAC decoder!" << std::endl;
                return;
            }
            
            FLAC__stream_decoder_set_md5_checking(inputFlacDecoder, true); // Why not?
            
            FlacUserData userData;
            
            FLAC__StreamDecoderInitStatus inputDecoderStatus = 
                FLAC__stream_decoder_init_file(inputFlacDecoder, fromFile.string().c_str(),
                flacDecoderWriteCallback, flacDecoderMetadataCallback, flacDecoderErrorCallback, &userData);
            
            if(inputDecoderStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
                std::cout << "\tFailure during FLAC decoder initialization: "
                    << FLAC__StreamDecoderInitStatusString[inputDecoderStatus] << std::endl;
                FLAC__stream_decoder_delete(inputFlacDecoder);
                return;
            }
            
            FLAC__stream_decoder_process_until_end_of_stream(inputFlacDecoder);
            
            std::cout << "\tFLAC decoder status: "
                << FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(inputFlacDecoder)] << std::endl;
                
            FLAC__stream_decoder_delete(inputFlacDecoder);
            
            return;
        }
        default: {
            std::cout << "\tUnknown file type" << std::endl;
            return;
        }
    }
}
