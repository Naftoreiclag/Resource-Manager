/*
 *  Copyright 2016-2017 James Fong
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

#include "Convert.hpp"

#include <iostream>
#include <vector>

#include "vorbis/vorbisfile.h"
#include "vorbis/vorbisenc.h"
#include "vorbis/codec.h"
#include "FLAC/stream_decoder.h"

namespace resman {

// TODO: add support for multiple channels

struct FlacUserData {
    const boost::filesystem::path* outputFile;
    std::ofstream* outputData;
    
    unsigned int inputSampleRate;
    unsigned int inputBitsPerSample;
    unsigned int inputNumChannels;
    
    float paramVbrQuality;
    
    vorbis_info outputVorbisInfo;
    int outputNumChannels;
    vorbis_dsp_state outputVorbisDspState;
    vorbis_block outputVorbisBlock;
    ogg_stream_state outputOggStream;
    ogg_page outputOggPage;
    vorbis_comment outputVorbisMetadata;
};

FLAC__StreamDecoderWriteStatus flacDecoderWriteCallback(
    const FLAC__StreamDecoder* inputFlacDecoder,
    const FLAC__Frame* inputFrame,
    const FLAC__int32* const inputBuffer[],
    void* userData) {
    
    FlacUserData& p = *(reinterpret_cast<FlacUserData*>(userData));
    int errorCode;
    
    // Called only once
    if(inputFrame->header.number.sample_number == 0) {
        // Write intialization
        vorbis_info_init(&p.outputVorbisInfo);
        p.outputNumChannels = 1;
        errorCode = vorbis_encode_init_vbr(&p.outputVorbisInfo, p.outputNumChannels, p.inputSampleRate, p.paramVbrQuality);
        if(errorCode) {
            std::cout << "\tFailed to initialize encoding for VBR!" << std::endl;
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
        
        errorCode = vorbis_analysis_init(&p.outputVorbisDspState, &p.outputVorbisInfo);
        if(errorCode) {
            std::cout << "\tFailed to initialize vorbis dsp state!" << std::endl;
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
        }
        
        //
        vorbis_block_init(&p.outputVorbisDspState, &p.outputVorbisBlock);
        
        int cerealNumber = 1337; // Why?
        
        ogg_stream_init(&p.outputOggStream, cerealNumber);
        
        
        p.outputData = new std::ofstream(p.outputFile->string().c_str(), std::ios::out | std::ios::binary);
        
        {
            ogg_packet streamIdentity;
            ogg_packet serializedMetadata;
            ogg_packet codebooks;
            
            vorbis_analysis_headerout(&p.outputVorbisDspState, &p.outputVorbisMetadata, &streamIdentity, &serializedMetadata, &codebooks);
            
            ogg_stream_packetin(&p.outputOggStream, &streamIdentity);
            ogg_stream_packetin(&p.outputOggStream, &serializedMetadata);
            ogg_stream_packetin(&p.outputOggStream, &codebooks);
            
            while(true) {
                int ret = ogg_stream_flush(&p.outputOggStream, &p.outputOggPage);
                if(ret == 0) {
                    break;
                }
                p.outputData->write(reinterpret_cast<char*>(p.outputOggPage.header), p.outputOggPage.header_len);
                p.outputData->write(reinterpret_cast<char*>(p.outputOggPage.body), p.outputOggPage.body_len);
            }
        }
    }
    
    // (Past tense "read")
    unsigned int numSamplesRead = inputFrame->header.blocksize;
    
    // Get a buffer which we must write to
    float** outputBuffers = vorbis_analysis_buffer(&p.outputVorbisDspState, numSamplesRead);
    
    float maxAbsVal = (1 << (p.inputBitsPerSample - 1));
    
    for(unsigned int i = 0; i < numSamplesRead; ++ i) {
        // only one channel is supported right now
        outputBuffers[0][i] = inputBuffer[0][i] / maxAbsVal;
    }
    
    // Tell vorbis that we are done writing to the buffer gained previously
    // (Necessary because we technically could have submitted less data than was requested,
    // and also because we need to notify vorbis that we are done writing for now)
    vorbis_analysis_wrote(&p.outputVorbisDspState, numSamplesRead);

    //
    while(vorbis_analysis_blockout(&p.outputVorbisDspState, &p.outputVorbisBlock) == 1) {
        vorbis_analysis(&p.outputVorbisBlock, NULL);
        vorbis_bitrate_addblock(&p.outputVorbisBlock);
        
        ogg_packet outputOggPacket;
        while(vorbis_bitrate_flushpacket(&p.outputVorbisDspState, &outputOggPacket)) {
            ogg_stream_packetin(&p.outputOggStream, &outputOggPacket);
            while(true) {
                int ret = ogg_stream_pageout(&p.outputOggStream, &p.outputOggPage);
                if(ret == 0) {
                    break;
                }
                p.outputData->write(reinterpret_cast<char*>(p.outputOggPage.header), p.outputOggPage.header_len);
                p.outputData->write(reinterpret_cast<char*>(p.outputOggPage.body), p.outputOggPage.body_len);
            }
        }
    }
    
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void flacDecoderMetadataCallback(
    const FLAC__StreamDecoder* inputFlacDecoder,
    const FLAC__StreamMetadata* inputMetadata,
    void* userData) {
    
    FlacUserData& p = *(reinterpret_cast<FlacUserData*>(userData));
    
    // Print file metadata
    std::cout << "\tInput file info:" << std::endl;
    std::cout << "\t\tType: FLAC" << std::endl;
    
    // comments
    if(inputMetadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT) {
        FLAC__StreamMetadata_VorbisComment_Entry* comments = inputMetadata->data.vorbis_comment.comments;
        int numComments = inputMetadata->data.vorbis_comment.num_comments;
        std::cout << "\t\tMetadata:" << std::endl;
        for(int i = 0; i < numComments; ++ i) {
            std::cout << "\t\t" << i << ".\t" << comments[i].entry << std::endl;
        }
        const FLAC__StreamMetadata_VorbisComment_Entry& vendorString = inputMetadata->data.vorbis_comment.vendor_string;
        std::cout << "\t\tVendor string: " << vendorString.entry << std::endl;
    }
    
    // other
    if(inputMetadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        p.inputSampleRate = inputMetadata->data.stream_info.sample_rate;
        p.inputBitsPerSample = inputMetadata->data.stream_info.bits_per_sample;
        p.inputNumChannels = inputMetadata->data.stream_info.channels;
        
        std::cout << "\t\tChannel count: " << p.inputNumChannels << std::endl;
        std::cout << "\t\tSampling rate: " << p.inputSampleRate << "hz" << std::endl;
        std::cout << "\t\tTotal samples: " << inputMetadata->data.stream_info.total_samples << std::endl;
        std::cout << "\t\tSample type: signed " << p.inputBitsPerSample << "-bit" << std::endl;
    }
    
}

void flacDecoderErrorCallback(
    const FLAC__StreamDecoder* inputFlacDecoder,
    const FLAC__StreamDecoderErrorStatus errorStatus,
    void* userData) {
    
    std::cout << "\t\tFLAC error callback: " << FLAC__StreamDecoderErrorStatusString[errorStatus];
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

/* TODO: I'm sure there are plenty of memory leaks occuring during errors (return's without proper cleanup). These need fixing eventually.
 * But then again, another reason I made this resource manager is to allow this kind of carelessness without effecting the end application
 * (whatever will be using the output resources).
 */
 
/* Dear past self,
 * What?
 * Sincerely, current self
 */

void convertWaveform(const Convert_Args& args) {
    
    // Get data from params
    WaveformEncodingMode paramWaveformEncodingMode = WaveformEncodingMode::VBR;
    float paramVbrQuality = 1.f;
    std::vector<TagPair> paramTags;
    if(!args.params.isNull()) {
        const Json::Value& encMode = args.params["encoding-mode"];
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
        
        const Json::Value& vbrQuality = args.params["vbr-quality"];
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
        
        const Json::Value& tags = args.params["metadata"];
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
    errorCode = ov_fopen(args.fromFile.string().c_str(), &inputVorbisFile);
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
    
    // Prepare metadata for output file
    vorbis_comment outputVorbisMetadata;
    vorbis_comment_init(&outputVorbisMetadata);
    
    for(std::vector<TagPair>::iterator iter = paramTags.begin(); iter != paramTags.end(); ++ iter) {
        TagPair& tagPair = *iter;
        vorbis_comment_add_tag(&outputVorbisMetadata, tagPair.tag.c_str(), tagPair.data.c_str());
    }
    
    
    switch(paramWaveformInputFileType) {
        case WaveformInputFileType::OGG_VORBIS: {
            const vorbis_info* inputVorbisInfo = ov_info(&inputVorbisFile, -1);
            
            // Print file metadata
            {
                std::cout << "\tInput file info:" << std::endl;
                std::cout << "\t\tType: ogg-vorbis" << std::endl;
                
                // comments
                {
                    const vorbis_comment* commentData = ov_comment(&inputVorbisFile, -1);
                    char** comments = commentData->user_comments;
                    int numComments = commentData->comments;
                    std::cout << "\t\tMetadata:" << std::endl;
                    for(int i = 0; i < numComments; ++ i) {
                        std::cout << "\t\t" << i << ".\t" << comments[i] << std::endl;
                    }
                }
                
                // other
                {
                    std::cout << "\t\tEncoder version: " << inputVorbisInfo->version << std::endl;
                    std::cout << "\t\tChannel count: " << inputVorbisInfo->channels << std::endl;
                    std::cout << "\t\tSampling rate: " << inputVorbisInfo->rate << "hz" << std::endl;
                }
            }
            
            // Write intialization
            vorbis_info outputVorbisInfo;
            vorbis_info_init(&outputVorbisInfo);
            int outputNumChannels = 1;
            errorCode = vorbis_encode_init_vbr(&outputVorbisInfo, outputNumChannels, inputVorbisInfo->rate, paramVbrQuality);
            if(errorCode) {
                std::cout << "\tFailed to initialize encoding for VBR!" << std::endl;
                return;
            }
            
            vorbis_dsp_state outputVorbisDspState;
            errorCode = vorbis_analysis_init(&outputVorbisDspState, &outputVorbisInfo);
            if(errorCode) {
                std::cout << "\tFailed to initialize vorbis dsp state!" << std::endl;
                return;
            }
            
            //
            vorbis_block outputVorbisBlock;
            vorbis_block_init(&outputVorbisDspState, &outputVorbisBlock);
            
            int cerealNumber = 1337; // Why?
            
            ogg_stream_state outputOggStream;
            ogg_stream_init(&outputOggStream, cerealNumber);
            
            ogg_page outputOggPage;
            
            std::ofstream outputData(args.outputFile.string().c_str(), std::ios::out | std::ios::binary);
            
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
            
            FlacUserData p;
            p.paramVbrQuality = paramVbrQuality;
            p.outputFile = &args.outputFile;
            p.outputVorbisMetadata = outputVorbisMetadata;
            
            FLAC__StreamDecoderInitStatus inputDecoderStatus = 
                FLAC__stream_decoder_init_file(inputFlacDecoder, args.fromFile.string().c_str(),
                flacDecoderWriteCallback, flacDecoderMetadataCallback, flacDecoderErrorCallback, &p);
            
            if(inputDecoderStatus != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
                std::cout << "\tFailure during FLAC decoder initialization: "
                    << FLAC__StreamDecoderInitStatusString[inputDecoderStatus] << std::endl;
                FLAC__stream_decoder_delete(inputFlacDecoder);
                return;
            }
            
            FLAC__stream_decoder_process_until_end_of_stream(inputFlacDecoder);
            
            std::cout << "\tFLAC decoder status: "
                << FLAC__StreamDecoderStateString[FLAC__stream_decoder_get_state(inputFlacDecoder)] << std::endl;
            
            // Reading cleanup
            FLAC__stream_decoder_delete(inputFlacDecoder);
            
            // Writing cleanup
            delete p.outputData;
            ogg_stream_clear(&p.outputOggStream);
            vorbis_block_clear(&p.outputVorbisBlock);
            vorbis_comment_clear(&p.outputVorbisMetadata);
            vorbis_dsp_clear(&p.outputVorbisDspState);
            vorbis_info_clear(&p.outputVorbisInfo);
            
            return;
        }
        default: {
            std::cout << "\tUnknown file type" << std::endl;
            return;
        }
    }
}

} // namespace resman
