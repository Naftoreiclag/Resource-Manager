#include "Convert.hpp"

#include <fstream>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

std::string convertImage(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {

    int width;
    int height;
    int components;
    unsigned char* image = stbi_load(fromFile.c_str(), &width, &height, &components, 0);

    if(!image) {
        std::cout << "\tFailed to read image!" << std::endl;
        return "";
    }

    bool writeAsDebug = false;
    bool deleteFinalImage = false;

    if(!params.isNull()) {

        if(!params["debug"].isNull()) {
            writeAsDebug = params["debug"].asBool();
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
                    delete image;
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
                        delete image;
                    } else {
                        stbi_image_free(image);
                    }
                    deleteFinalImage = true;
                    image = newImageData;
                    components = 3;
                }
                else if(alphaCleave == "lazy") {
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
                        delete image;
                    } else {
                        stbi_image_free(image);
                    }
                    deleteFinalImage = true;
                    image = newImageData;
                    components = 3;
                }
                /*
                else if(alphaCleave == "shell") {
                    int size = width * height * 3;
                    unsigned char* newImageData = new unsigned char[size];

                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {

                            unsigned char an = image[(x + (y * width)) * components + 3];

                            if(an > 127) {
                                newImageData[(x + (y * width)) * 3 + 0] = image[(x + (y * width)) * components + 0];
                                newImageData[(x + (y * width)) * 3 + 1] = image[(x + (y * width)) * components + 1];
                                newImageData[(x + (y * width)) * 3 + 2] = image[(x + (y * width)) * components + 2];
                            }
                            else {
                                bool first = true;
                                double closest;
                                unsigned char rn, gn, bn;
                                for(int y2 = 0; y2 < height; ++ y2) {
                                    for(int x2 = 0; x2 < width; ++ x2) {

                                        unsigned char alphaTest = image[(x2 + (y2 * width)) * components + 3];

                                        if(alphaTest > 127) {
                                            if(first) {
                                                first = false;
                                                closest = ((x - x2) * (x - x2)) + ((y - y2) * (y - y2));
                                            }
                                            else {
                                                double maybe = ((x - x2) * (x - x2)) + ((y - y2) * (y - y2));
                                                if(maybe < closest) {
                                                    closest = maybe;
                                                    newImageData[(x + (y * width)) * 3 + 0] = image[(x2 + (y2 * width)) * components + 0];
                                                    newImageData[(x + (y * width)) * 3 + 1] = image[(x2 + (y2 * width)) * components + 1];
                                                    newImageData[(x + (y * width)) * 3 + 2] = image[(x2 + (y2 * width)) * components + 2];
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if(deleteFinalImage) {
                        delete image;
                    } else {
                        stbi_image_free(image);
                    }
                    deleteFinalImage = true;
                    image = newImageData;
                    components = 3;
                }
                */
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
            return outputFile.filename().c_str();
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
        delete image;
    }
    else {
        stbi_image_free(image);
    }

    return outputFile.filename().c_str();
}
