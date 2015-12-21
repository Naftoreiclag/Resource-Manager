#include "Convert.hpp"

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
    bool deleteFinalImage = false;

    if(!params.isNull()) {

        if(!params["debug"].isNull()) {
            writeAsDebug = params["debug"].asBool();
        }

        const Json::Value& distanceField = params["distanceField"];
        if(!distanceField.isNull()) {
            bool absolute = true;
            int nWidth = width;
            int nHeight = height;
        }

        const Json::Value& resize = params["resize"];
        if(!resize.isNull()) {
            bool absolute = true;
            int nWidth = width;
            int nHeight = height;

            if(!resize["absolute"].isNull()) {
                absolute = resize["absolute"].asBool();
            }

            if(!resize["width"].isNull()) {
                if(absolute) {
                    nWidth = resize["width"].asInt();
                }
                else {
                    nWidth = ((double) width) * resize["width"].asDouble();
                }
            }

            if(!resize["height"].isNull()) {
                if(absolute) {
                    nHeight = resize["height"].asInt();
                }
                else {
                    nHeight = ((double) height) * resize["height"].asDouble();
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
                if(deleteFinalImage) {
                    delete[] image;
                } else {
                    stbi_image_free(image);
                }
                deleteFinalImage = true;
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
                    unsigned char* newImageData = new unsigned char[size];

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

                            newImageData[(x + (y * width)) * 3 + 0] = rf;
                            newImageData[(x + (y * width)) * 3 + 1] = gf;
                            newImageData[(x + (y * width)) * 3 + 2] = bf;
                        }
                    }

                    if(deleteFinalImage) {
                        delete[] image;
                    } else {
                        stbi_image_free(image);
                    }
                    deleteFinalImage = true;
                    image = newImageData;
                    components = 3;
                }
                else if(alphaCleave == "lazy") {
                    std::cout << "\tAlpha cleave: lazy" << std::endl;
                    int size = width * height * 3;
                    unsigned char* newImageData = new unsigned char[size];

                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {
                            newImageData[(x + (y * width)) * 3 + 0] = image[(x + (y * width)) * components + 0];
                            newImageData[(x + (y * width)) * 3 + 1] = image[(x + (y * width)) * components + 1];
                            newImageData[(x + (y * width)) * 3 + 2] = image[(x + (y * width)) * components + 2];
                        }
                    }

                    if(deleteFinalImage) {
                        delete[] image;
                    } else {
                        stbi_image_free(image);
                    }
                    deleteFinalImage = true;
                    image = newImageData;
                    components = 3;
                }
                else if(alphaCleave == "clamp") {
                    std::cout << "\tAlpha cleave: clamp" << std::endl;
                    int size = width * height * 3;
                    unsigned char* newImageData = new unsigned char[size];

                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {
                            if(image[(x + (y * width)) * components + 3] > 0) {
                                newImageData[(x + (y * width)) * 3 + 0] = image[(x + (y * width)) * components + 0];
                                newImageData[(x + (y * width)) * 3 + 1] = image[(x + (y * width)) * components + 1];
                                newImageData[(x + (y * width)) * 3 + 2] = image[(x + (y * width)) * components + 2];
                            } else {
                                newImageData[(x + (y * width)) * 3 + 0] = 255;
                                newImageData[(x + (y * width)) * 3 + 1] = 0;
                                newImageData[(x + (y * width)) * 3 + 2] = 255;
                            }
                        }
                    }

                    if(deleteFinalImage) {
                        delete[] image;
                    } else {
                        stbi_image_free(image);
                    }
                    deleteFinalImage = true;
                    image = newImageData;
                    components = 3;
                }
                else if(alphaCleave == "mask") {
                    std::cout << "\tAlpha cleave: mask" << std::endl;
                    int size = width * height * 3;
                    unsigned char* newImageData = new unsigned char[size];

                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {
                            newImageData[(x + (y * width)) * 3 + 0] = image[(x + (y * width)) * components + 3];
                            newImageData[(x + (y * width)) * 3 + 1] = image[(x + (y * width)) * components + 3];
                            newImageData[(x + (y * width)) * 3 + 2] = image[(x + (y * width)) * components + 3];
                        }
                    }

                    if(deleteFinalImage) {
                        delete[] image;
                    } else {
                        stbi_image_free(image);
                    }
                    deleteFinalImage = true;
                    image = newImageData;
                    components = 3;
                }
                else if(alphaCleave == "shell" || alphaCleave == "shellhq") {
                    bool highQuality = (alphaCleave == "shellhq");
                    std::cout << "\tAlpha cleave: shell" << (highQuality ? "hq" : "") << std::endl;
                    int size = width * height * 3;
                    unsigned char* newImageData = new unsigned char[size];

                    unsigned char opp = 127;

                    double total = width * height;
                    int percentDone = 0;
                    double progress = 0;
                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {

                            unsigned char& finalR = newImageData[(x + (y * width)) * 3 + 0];
                            unsigned char& finalG = newImageData[(x + (y * width)) * 3 + 1];
                            unsigned char& finalB = newImageData[(x + (y * width)) * 3 + 2];
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

                                        int x2 = (index == 0 || index == 3) ? (x - expand) : (x + expand);
                                        int y2 = (index == 0 || index == 1) ? (y - expand) : (y + expand);

                                        for(int lear = 0; lear <= expand; ++ lear) {
                                            if(lear > 0) {
                                                if(index == 0) {
                                                    ++ x2;
                                                } else if(index == 1) {
                                                    ++ y2;
                                                } else if(index == 2) {
                                                    -- x2;
                                                } else if(index == 3) {
                                                    -- y2;
                                                }
                                            }

                                            if(x2 < 0 || x2 >= width || y2 < 0 || y2 >= height) {
                                                continue;
                                            }

                                            unsigned char alphaTest = image[(x2 + (y2 * width)) * components + 3];

                                            if(alphaTest > opp) {
                                                double dist = ((x - x2) * (x - x2)) + ((y - y2) * (y - y2));
                                                if(first) {
                                                    first = false;
                                                    closest = dist;
                                                    closestX = x2;
                                                    closestY = y2;
                                                }
                                                else {
                                                    if(dist < closest) {
                                                        closest = dist;
                                                        closestX = x2;
                                                        closestY = y2;
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

                    if(deleteFinalImage) {
                        delete[] image;
                    } else {
                        stbi_image_free(image);
                    }
                    deleteFinalImage = true;
                    image = newImageData;
                    components = 3;
                }
            }
        }
    }

    std::cout << "\tWidth: " << width << std::endl;
    std::cout << "\tHeight: " << height << std::endl;
    std::cout << "\tComponents: " << components << std::endl;
    std::cout << "\tRaw array size: " << (width * height * components) << std::endl;

    if(writeAsDebug) {
        int result = stbi_write_png(outputFile.c_str(), width, height, components, image, 0);

        if(result > 0) {
            std::cout << "\tWritten as debug!" << std::endl;
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

    if(deleteFinalImage) {
        delete[] image;
    }
    else {
        stbi_image_free(image);
    }

    return;
}
