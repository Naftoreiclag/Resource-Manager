#include "Convert.hpp"

#include <fstream>
#include <iostream>
#include <stdint.h>

#include "stb_image.h"

#include "json/json.h"

struct FontDesc {
    float baseline;
    float advances[256];
};

void generateMetrics(FontDesc& font, const boost::filesystem::path& imgFile) {
    int width;
    int height;
    int components;
    unsigned char* image = stbi_load(imgFile.c_str(), &width, &height, &components, 0);

    if(!image) {
        std::cout << "\tError: Failed to read metrics image file!" << std::endl;
        std::cout << "\t\t" << imgFile << std::endl;
        return;
    }

    uint32_t nRows = 16;
    uint32_t nColumns = 16;

    if(width % nColumns != 0) {
        std::cout << "\tWarning: Metrics image width (" << width << ") not evenly divided by " << nColumns << std::endl;
    }
    if(height % nRows != 0) {
        std::cout << "\tWarning: Metrics image height (" << height << ") not evenly divided by " << nRows << std::endl;
    }

    uint32_t glyphWidth = width / nColumns;
    uint32_t glyphHeight = height / nRows;

    if(glyphWidth == 0 || glyphHeight == 0) {
        std::cout << "\tError: Invalid image area!" << std::endl;
        return;
    }

    uint32_t index = 0;
    for(uint32_t cy = 0; cy < nRows; ++ cy) {
        for(uint32_t cx = 0; cx < nColumns; ++ cx) {

            bool foundBegin = false;
            uint32_t xBegin = 0;
            for(uint32_t x = 0; x < glyphWidth; ++ x) {
                for(uint32_t y = 0; y < glyphHeight; ++ y) {
                    uint32_t absX = (cx * glyphWidth) + x;
                    uint32_t absY = (cy * glyphHeight) + y;
                    uint32_t absIndex = (absX + (absY * width)) * components;

                    if(image[absIndex] > 0) {
                        xBegin = x;
                        foundBegin = true;
                        break;
                    }
                }
                if(foundBegin) {
                    break;
                }
            }

            // There is no beginning, then the width of this glyph is zero.
            if(!foundBegin) {
                font.advances[index] = 0;
            }

            // There is a beginning, so find the ending
            else {
                bool foundEnd = false;
                uint32_t xEnd = 0;
                for(uint32_t x = glyphWidth - 1; x >= 0; -- x) {
                    for(uint32_t y = 0; y < glyphHeight; ++ y) {
                        uint32_t absX = (cx * glyphWidth) + x;
                        uint32_t absY = (cy * glyphHeight) + y;
                        uint32_t absIndex = (absX + (absY * width)) * components;

                        if(image[absIndex] > 0) {
                            xEnd = x;
                            foundEnd = true;
                            break;
                        }
                    }
                    if(foundEnd) {
                        break;
                    }
                }

                if(!foundEnd) {
                    // Should not be possible to get here
                }

                else {
                    font.advances[index] = (xEnd - xBegin) + 1;
                }

            }

            std::cout << "\t" << ((char) index) << "\tAdv: " << font.advances[index] << std::endl;

            ++ index;
        }
    }
}

void convertFont(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {

    Json::Value fontData;
    {
        std::ifstream fileStream(fromFile.c_str());
        fileStream >> fontData;
        fileStream.close();
    }

    Json::Value& metricsData = fontData["metrics"];

    if(metricsData.isNull()) {
        std::cout << "\tError: Font metrics specification absent!" << std::endl;
        return;
    }

    Json::Value& renderingData = fontData["rendering"];
    if(renderingData.isNull()) {
        std::cout << "\tError: Font rendering specification absent!" << std::endl;
    }

    FontDesc font;

    font.baseline = metricsData["baseline"].asFloat();

    generateMetrics(font, fromFile.parent_path() / (metricsData["imageFile"].asString()));

    boost::filesystem::copy_file(fromFile, outputFile);
}
