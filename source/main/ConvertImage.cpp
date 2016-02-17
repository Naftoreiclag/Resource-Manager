#include "Convert.hpp"

#include <cmath>
#include <fstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

void convertImage(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {

    int width;
    int height;
    int components;
    unsigned char* image = stbi_load(fromFile.c_str(), &width, &height, &components, 0);

    if(!image) {
        std::cout << "\tFailed to read image!" << std::endl;
        return;
    }

    bool writeAsDebug = false;
    bool manuallyFreeImage = false;

    if(!params.isNull()) {

        if(!params["debug"].isNull()) {
            writeAsDebug = params["debug"].asBool();
        }

        const Json::Value& distanceFieldData = params["distanceField"];
        if(!distanceFieldData.isNull()) {
            int nWidth = width;
            int nHeight = height;

            std::cout << "\tDistance field" << std::endl;

            if(!distanceFieldData["width"].isNull()) {
                if(distanceFieldData["width"].asFloat() < 1) {
                    nWidth = ((float) width) * distanceFieldData["width"].asFloat();
                } else {
                    nWidth = distanceFieldData["width"].asInt();
                }
            }
            if(!distanceFieldData["height"].isNull()) {
                if(distanceFieldData["height"].asFloat() < 1) {
                    nHeight = ((float) height) * distanceFieldData["height"].asFloat();
                } else {
                    nHeight = distanceFieldData["height"].asInt();
                }
            }

            float insideSize = width;
            float outsideSize = height;
            if(!distanceFieldData["inside"].isNull()) {
                if(distanceFieldData["inside"].asFloat() < 1) {
                    // Use width, why not?
                    insideSize = ((float) width) * distanceFieldData["inside"].asFloat();
                } else {
                    insideSize = distanceFieldData["inside"].asInt();
                }
            }
            if(!distanceFieldData["outside"].isNull()) {
                if(distanceFieldData["outside"].asFloat() < 1) {
                    outsideSize = ((float) width) * distanceFieldData["outside"].asFloat();
                } else {
                    outsideSize = distanceFieldData["outside"].asInt();
                }
            }

            float edgeValue = 0.5f;
            if(!distanceFieldData["edgeValue"].isNull()) {
                float romeo = distanceFieldData["edgeValue"].asFloat();
                if(romeo > 0.f && romeo < 1.f) {
                    edgeValue = romeo;
                } else {
                    std::cout << "\tWarning: Edge value must be in the range (0.0, 1.0). Setting to default (0.5)." << std::endl;
                }
            }

            // Which channel to detect the edge on
            // Example: 0 = red channel, 3 = alpha channel
            uint32_t channel = 0;
            if(!distanceFieldData["channel"].isNull()) {
                channel = distanceFieldData["channel"].asUInt();

                if(channel >= components) {
                    std::cout << "\tWarning: Channel cannot be " << channel << std::endl;
                    channel = 0;
                }
            }

            if(nWidth < 1 || nHeight < 1) {
                std::cout << "\tFailed to calculate distance field: illegal dimensions" << std::endl;
            }
            else {
                std::cout << "\tResize to: " << nWidth << ", " << nHeight << std::endl;

                // If for some reason future image files can only support >=3 channels, this is what needs to change
                uint32_t nComponents = 1;

                // New image
                unsigned char* nImage = new unsigned char[nWidth * nHeight * nComponents];

                // Curious data
                uint64_t numPixelsChecked = 0;

                // Converting coordinates
                uint32_t scaleX = width / nWidth;
                uint32_t scaleY = height / nHeight;

                // For each of the new pixels
                for(int y = 0; y < nHeight; ++ y) {
                    for(int x = 0; x < nWidth; ++ x) {
                        // Determine if this is inside or outside
                        bool isInside = image[(x * scaleX + (y * scaleY * width)) * components + channel] > 0;

                        // Determining the shortest distance squared
                        bool pixelFound = false;
                        float shortestDistanceSq;

                        // The farthest distance (in original pixels) that should be scanned
                        uint32_t maxDist = std::ceil(isInside ? insideSize : outsideSize);

                        for(uint32_t radius = 1; radius < maxDist; ++ radius) {
                            // Begin of spiral algorithm
                            for(uint8_t direction = 0; direction < 4; ++ direction) {
                                // 0  >  1
                                //
                                // ^     v
                                //
                                // 3  <  2

                                // The coordinates of the pixel that will be checked in the original image
                                // (Yes these are supposed to be signed)
                                int32_t scanX = (direction == 0 || direction == 3) ? (x * scaleX - radius) : (x * scaleX + radius);
                                int32_t scanY = (direction == 0 || direction == 1) ? (y * scaleY - radius) : (y * scaleY + radius);

                                // Stepping
                                for(uint32_t step = 0; step <= radius * 2; ++ step) {

                                    // Move the scanning coordinates based on direction
                                    if(step > 0) {
                                        if(direction == 0) {
                                            ++ scanX;
                                        } else if(direction == 1) {
                                            ++ scanY;
                                        } else if(direction == 2) {
                                            -- scanX;
                                        } else if(direction == 3) {
                                            -- scanY;
                                        }
                                    }

                                    // Do not check pixels that are outside the bounds
                                    if(scanX < 0 || scanX >= width || scanY < 0 || scanY >= height) {
                                        continue;
                                    }

                                    // Check this pixel
                                    ++ numPixelsChecked;

                                    // If this pixel is of a different side
                                    if((image[(scanX + (scanY * width)) * components + channel] > 0) ^ isInside) {
                                        float distanceSq =
                                            (((x * scaleX) - scanX) * ((x * scaleX) - scanX)) +
                                            (((y * scaleY) - scanY) * ((y * scaleY) - scanY));

                                        // This is the first different pixel located
                                        if(!pixelFound) {
                                            // Then it is the closest so far
                                            shortestDistanceSq = distanceSq;

                                            // Slight optimization: reduce maximum scanning distance
                                            float nMaxDist = std::ceil(std::sqrt(shortestDistanceSq));
                                            if(nMaxDist < maxDist) { maxDist = nMaxDist; }

                                            // Remember that the first pixel was already located
                                            pixelFound = true;
                                        }

                                        // This is yet another pixel located
                                        else if(distanceSq < shortestDistanceSq) {
                                            // Update shortest distance
                                            shortestDistanceSq = distanceSq;

                                            // Slight optimization: reduce maximum scanning distance
                                            float nMaxDist = std::ceil(std::sqrt(shortestDistanceSq));
                                            if(nMaxDist < maxDist) { maxDist = nMaxDist; }

                                        }
                                    }
                                }
                            }
                        }

                        /*
                        // use spiraling instead. THIS IS WAY TOO SLOW!!!
                        for(int oy = 0; oy < height; ++ oy) {
                            for(int ox = 0; ox < width; ++ ox) {
                                // If this pixel is of a different side
                                if((image[(ox + (oy * width)) * components + channel] > 0) ^ isInside) {
                                    float distanceSq = (((x * scaleX) - ox) * ((x * scaleX) - ox)) + (((y * scaleY) - oy) * ((y * scaleY) - oy));
                                    if(!pixelFound) {
                                        shortestDistanceSq = distanceSq;
                                        pixelFound = true;
                                    }
                                    else if(shortestDistanceSq > distanceSq) {
                                        shortestDistanceSq = distanceSq;
                                    }
                                }
                            }
                        }
                        */

                        // 1 = deep within positive area
                        // 0 = deep within negative area
                        float intensity;

                        if(pixelFound) {
                            float shortestDistance = std::sqrt(shortestDistanceSq);

                            if(isInside) {
                                shortestDistance /= insideSize;
                                if(shortestDistance > 1.0f) {
                                    intensity = 1.f;
                                }
                                else {
                                    intensity = (edgeValue + (shortestDistance * (1.f - edgeValue)));
                                }
                            } else {
                                shortestDistance /= outsideSize;
                                if(shortestDistance > 1.0f) {
                                    intensity = 0.f;
                                }
                                else {
                                    intensity = (edgeValue - (shortestDistance * (edgeValue)));
                                }

                            }
                        }

                        // This is actually possible
                        else {
                            intensity = isInside ? 1.f : 0.f;
                        }

                        // If for some reason nComponents > 1, this is necessary
                        for(unsigned int juliet = 0; juliet < nComponents; ++ juliet) {
                            nImage[((x + (y * nWidth)) * nComponents) + juliet] = 255.f * intensity;
                        }
                    }
                }

                // Curiosity
                {
                    uint64_t bruteForce = 1;
                    bruteForce *= nWidth;
                    bruteForce *= nHeight;
                    bruteForce *= width;
                    bruteForce *= height;

                    double savings = 1.0 - (((double) numPixelsChecked) / ((double) bruteForce));

                    std::cout << "\tCompleted with " << numPixelsChecked << " array calls" << std::endl;
                    std::cout << "\tBrute force would have used " << bruteForce << " array calls (" << savings << " reduction)" << std::endl;
                }

                // Swap out image
                if(manuallyFreeImage) {
                    delete[] image;
                } else {
                    stbi_image_free(image);
                }
                manuallyFreeImage = true;
                width = nWidth;
                height = nHeight;
                image = nImage;
                components = nComponents;
            }
        }

        const Json::Value& resizeData = params["resize"];
        if(!resizeData.isNull()) {
            int nWidth = width;
            int nHeight = height;

            if(!resizeData["width"].isNull()) {
                if(resizeData["width"].asFloat() < 1) {
                    nWidth = ((float) width) * resizeData["width"].asFloat();
                } else {
                    nWidth = resizeData["width"].asInt();
                }
            }
            if(!resizeData["height"].isNull()) {
                if(resizeData["height"].asFloat() < 1) {
                    nHeight = ((float) height) * resizeData["height"].asFloat();
                } else {
                    nHeight = resizeData["height"].asInt();
                }
            }

            if(nWidth < 1 || nHeight < 1) {
                std::cout << "\tFailed to resize image: illegal dimensions" << std::endl;
            }
            else {
                std::cout << "\tResize to: " << nWidth << ", " << nHeight << std::endl;

                int size = nWidth * nHeight * components;

                unsigned char* tempImageData = new unsigned char[size];

                stbir_resize_uint8(image, width, height, 0, tempImageData, nWidth, nHeight, 0, components);
                if(manuallyFreeImage) {
                    delete[] image;
                } else {
                    stbi_image_free(image);
                }
                manuallyFreeImage = true;
                width = nWidth;
                height = nHeight;
                image = new unsigned char[size];
                // Copy because
                for(int i = 0; i < size; ++ i) {
                    image[i] = tempImageData[i];
                }
            }
        }

        if(components > 3) {
            if(!params["alphaCleave"].isNull()) {
                std::string alphaCleave = params["alphaCleave"].asString();

                if(alphaCleave == "premultiply") {
                    std::cout << "\tAlpha cleave: premultiply" << std::endl;
                    int size = width * height * 3;
                    unsigned char* nImage = new unsigned char[size];

                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {
                            unsigned char rn = image[(x + (y * width)) * components + 0];
                            unsigned char gn = image[(x + (y * width)) * components + 1];
                            unsigned char bn = image[(x + (y * width)) * components + 2];
                            unsigned char an = image[(x + (y * width)) * components + 3];
                            double rd = rn;
                            double gd = gn;
                            double bd = bn;
                            double ad = an;
                            rd /= 255.0;
                            gd /= 255.0;
                            bd /= 255.0;
                            ad /= 255.0;
                            unsigned char rf = rd * ad * 255;
                            unsigned char gf = gd * ad * 255;
                            unsigned char bf = bd * ad * 255;

                            nImage[(x + (y * width)) * 3 + 0] = rf;
                            nImage[(x + (y * width)) * 3 + 1] = gf;
                            nImage[(x + (y * width)) * 3 + 2] = bf;
                        }
                    }

                    if(manuallyFreeImage) {
                        delete[] image;
                    } else {
                        stbi_image_free(image);
                    }
                    manuallyFreeImage = true;
                    image = nImage;
                    components = 3;
                }
                else if(alphaCleave == "lazy") {
                    std::cout << "\tAlpha cleave: lazy" << std::endl;
                    int size = width * height * 3;
                    unsigned char* nImage = new unsigned char[size];

                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {
                            nImage[(x + (y * width)) * 3 + 0] = image[(x + (y * width)) * components + 0];
                            nImage[(x + (y * width)) * 3 + 1] = image[(x + (y * width)) * components + 1];
                            nImage[(x + (y * width)) * 3 + 2] = image[(x + (y * width)) * components + 2];
                        }
                    }

                    if(manuallyFreeImage) {
                        delete[] image;
                    } else {
                        stbi_image_free(image);
                    }
                    manuallyFreeImage = true;
                    image = nImage;
                    components = 3;
                }
                else if(alphaCleave == "clamp") {
                    std::cout << "\tAlpha cleave: clamp" << std::endl;
                    int size = width * height * 3;
                    unsigned char* nImage = new unsigned char[size];

                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {
                            if(image[(x + (y * width)) * components + 3] > 0) {
                                nImage[(x + (y * width)) * 3 + 0] = image[(x + (y * width)) * components + 0];
                                nImage[(x + (y * width)) * 3 + 1] = image[(x + (y * width)) * components + 1];
                                nImage[(x + (y * width)) * 3 + 2] = image[(x + (y * width)) * components + 2];
                            } else {
                                nImage[(x + (y * width)) * 3 + 0] = 255;
                                nImage[(x + (y * width)) * 3 + 1] = 0;
                                nImage[(x + (y * width)) * 3 + 2] = 255;
                            }
                        }
                    }

                    if(manuallyFreeImage) {
                        delete[] image;
                    } else {
                        stbi_image_free(image);
                    }
                    manuallyFreeImage = true;
                    image = nImage;
                    components = 3;
                }
                else if(alphaCleave == "mask") {
                    std::cout << "\tAlpha cleave: mask" << std::endl;
                    int size = width * height * 3;
                    unsigned char* nImage = new unsigned char[size];

                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {
                            nImage[(x + (y * width)) * 3 + 0] = image[(x + (y * width)) * components + 3];
                            nImage[(x + (y * width)) * 3 + 1] = image[(x + (y * width)) * components + 3];
                            nImage[(x + (y * width)) * 3 + 2] = image[(x + (y * width)) * components + 3];
                        }
                    }

                    if(manuallyFreeImage) {
                        delete[] image;
                    } else {
                        stbi_image_free(image);
                    }
                    manuallyFreeImage = true;
                    image = nImage;
                    components = 3;
                }
                else if(alphaCleave == "shell" || alphaCleave == "shellhq") {
                    bool highQuality = (alphaCleave == "shellhq");
                    std::cout << "\tAlpha cleave: shell" << (highQuality ? "hq" : "") << std::endl;
                    int size = width * height * 3;
                    unsigned char* nImage = new unsigned char[size];

                    unsigned char opp = 127;

                    double total = width * height;
                    int percentDone = 0;
                    double progress = 0;
                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {

                            unsigned char& finalR = nImage[(x + (y * width)) * 3 + 0];
                            unsigned char& finalG = nImage[(x + (y * width)) * 3 + 1];
                            unsigned char& finalB = nImage[(x + (y * width)) * 3 + 2];
                            finalR = 255;
                            finalG = 0;
                            finalB = 255;

                            if(image[(x + (y * width)) * components + 3] > opp) {
                                finalR = image[(x + (y * width)) * components + 0];
                                finalG = image[(x + (y * width)) * components + 1];
                                finalB = image[(x + (y * width)) * components + 2];
                            }
                            else {
                                bool first = true;

                                bool recalc = false;

                                double closest;
                                int closestX;
                                int closestY;

                                // Slightly more efficient

                                int end = height > width ? height : width;
                                if((end & 1) == 1) {
                                    ++ end;
                                }
                                end /= 2;

                                for(int expand = 1; expand < end; ++ expand) {


                                    // Begin of spiral algorithm
                                    for(int index = 0; index < 4; ++ index) {
                                        // 0  >  1
                                        //
                                        // ^     v
                                        //
                                        // 3  <  2

                                        int scanX = (index == 0 || index == 3) ? (x - expand) : (x + expand);
                                        int scanY = (index == 0 || index == 1) ? (y - expand) : (y + expand);

                                        for(int step = 0; step <= expand * 2; ++ step) {
                                            if(step > 0) {
                                                if(index == 0) {
                                                    ++ scanX;
                                                } else if(index == 1) {
                                                    ++ scanY;
                                                } else if(index == 2) {
                                                    -- scanX;
                                                } else if(index == 3) {
                                                    -- scanY;
                                                }
                                            }

                                            if(scanX < 0 || scanX >= width || scanY < 0 || scanY >= height) {
                                                continue;
                                            }

                                            unsigned char alphaTest = image[(scanX + (scanY * width)) * components + 3];

                                            if(alphaTest > opp) {
                                                double dist = ((x - scanX) * (x - scanX)) + ((y - scanY) * (y - scanY));
                                                if(first) {
                                                    first = false;
                                                    closest = dist;
                                                    closestX = scanX;
                                                    closestY = scanY;
                                                }
                                                else {
                                                    if(dist < closest) {
                                                        closest = dist;
                                                        closestX = scanX;
                                                        closestY = scanY;
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    // If a opaque pixel was found in that search
                                    if(!first && !recalc) {
                                        double newEnd = ((double) expand) * 1.5;
                                        end = (newEnd < end) ? (newEnd) : end;
                                        recalc = true;
                                    }
                                }

                                if(highQuality) {
                                    int sampleRad = (width > height ? width : height) / 20;
                                    if(sampleRad < 1) {
                                        sampleRad = 1;
                                    }

                                    double avgR = 0;
                                    double avgG = 0;
                                    double avgB = 0;
                                    int numSamples = 0;

                                    for(int dx = -sampleRad; dx <= sampleRad; ++ dx) {
                                        for(int dy = -sampleRad; dy <= sampleRad; ++ dy) {

                                            int sampleX = closestX + dx;
                                            int sampleY = closestY + dy;

                                            if(sampleX < 0 || sampleX >= width || sampleY < 0 || sampleY >= height) {
                                                continue;
                                            }

                                            if(image[(sampleX + (sampleY * width)) * components + 3] > opp) {
                                                avgR += image[(sampleX + (sampleY * width)) * components + 0];
                                                avgG += image[(sampleX + (sampleY * width)) * components + 1];
                                                avgB += image[(sampleX + (sampleY * width)) * components + 2];
                                                ++ numSamples;
                                            }
                                        }
                                    }

                                    avgR /= numSamples;
                                    avgG /= numSamples;
                                    avgB /= numSamples;

                                    finalR = (unsigned char) avgR;
                                    finalG = (unsigned char) avgG;
                                    finalB = (unsigned char) avgB;
                                }
                                else {
                                    finalR = image[(closestX + (closestY * width)) * components + 0];
                                    finalG = image[(closestX + (closestY * width)) * components + 1];
                                    finalB = image[(closestX + (closestY * width)) * components + 2];
                                }

                            }

                            progress += 1;

                            if(progress >= (total / 10.0)) {
                                percentDone += 10;
                                progress = 0;
                                std::cout << "\t" << percentDone << "%" << std::endl;
                            }
                        }
                    }

                    if(manuallyFreeImage) {
                        delete[] image;
                    } else {
                        stbi_image_free(image);
                    }
                    manuallyFreeImage = true;
                    image = nImage;
                    components = 3;
                }
            }
        }
    }

    std::cout << "\tWidth: " << width << std::endl;
    std::cout << "\tHeight: " << height << std::endl;
    std::cout << "\tComponents: " << components << std::endl;
    std::cout << "\tRaw array size: " << (width * height * components) << std::endl;

    //if(writeAsDebug) {
    if(true) {
        int result = stbi_write_png(outputFile.c_str(), width, height, components, image, 0);

        if(result > 0) {
            return;
        }
        else {
            std::cout << "\tFailed to write as debug!" << std::endl;
        }
    }

    std::ofstream outputData(outputFile.c_str(), std::ios::out | std::ios::binary);

    outputData << width;
    outputData << height;
    outputData << components;

    for(int y = 0; y < height; ++ y) {
        for(int x = 0; x < width; ++ x) {
            for(int n = 0; n < components; ++ n) {
                outputData << image[n + ((x + (y * width)) * components)];
            }
        }
    }

    outputData.close();

    if(manuallyFreeImage) {
        delete[] image;
    }
    else {
        stbi_image_free(image);
    }

    return;
}
