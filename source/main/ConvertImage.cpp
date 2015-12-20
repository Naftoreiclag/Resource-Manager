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
                        delete image;
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
                        delete image;
                    } else {
                        stbi_image_free(image);
                    }
                    deleteFinalImage = true;
                    image = newImageData;
                    components = 3;
                }
                else if(alphaCleave == "shell") {
                    std::cout << "\tAlpha cleave: shell" << std::endl;
                    int size = width * height * 3;
                    unsigned char* newImageData = new unsigned char[size];

                    unsigned char opp = 127;

                    for(int y = 0; y < height; ++ y) {
                        for(int x = 0; x < width; ++ x) {

                            unsigned char an = image[(x + (y * width)) * components + 3];

                            unsigned char& finalR = newImageData[(x + (y * width)) * 3 + 0];
                            unsigned char& finalG = newImageData[(x + (y * width)) * 3 + 1];
                            unsigned char& finalB = newImageData[(x + (y * width)) * 3 + 2];

                            if(an > opp) {
                                finalR = image[(x + (y * width)) * components + 0];
                                finalG = image[(x + (y * width)) * components + 1];
                                finalB = image[(x + (y * width)) * components + 2];
                            }
                            else {
                                std::cout << x << "\t" << y << std::endl;
                                bool first = true;
                                bool recalc = false;
                                double closest;
                                int end = height > width ? height : width;
                                for(int rad = 1; rad < end; ++ rad) {
                                    for(int index = 0; index < 4; ++ index) {
                                        /*
                                         * 0  >  1
                                         *
                                         * ^     v
                                         *
                                         * 3  <  2
                                         */

                                        int x2 = (index == 0 || index == 3) ? (x - rad) : (x + rad);
                                        int y2 = (index == 0 || index == 1) ? (y - rad) : (y + rad);

                                        for(int lear = 0; lear < rad; ++ lear) {
                                            if(index == 0) {
                                                ++ x2;
                                            } else if(index == 1) {
                                                ++ y2;
                                            } else if(index == 2) {
                                                -- x2;
                                            } else if(index == 3) {
                                                -- y2;
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
                                                }
                                                else {
                                                    if(dist < closest) {
                                                        closest = dist;
                                                        finalR = image[(x2 + (y2 * width)) * components + 0];
                                                        finalG = image[(x2 + (y2 * width)) * components + 1];
                                                        finalB = image[(x2 + (y2 * width)) * components + 2];
                                                    }
                                                }
                                            }
                                        }
                                    }

                                    if(!first && !recalc) {
                                        end = (2 * rad < end) ? 2 * rad : end;
                                        recalc = true;
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
