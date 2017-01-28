#include "Convert.hpp"

#include <string>
#include <fstream>
#include <iostream>
#include <stdint.h>

#include "stb_image.h"

#include <json/json.h>

#include "StreamWrite.hpp"

struct FontDesc {
    float baseline;
    float padding;
    float glyphWidth[256];
    float glyphStartX[256];

    std::string texture;
};

void generateMetrics(FontDesc& font, const boost::filesystem::path& imgFile) {
    int width;
    int height;
    int components;
    unsigned char* image = stbi_load(imgFile.string().c_str(), &width, &height, &components, 0);

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

    uint32_t cellWidth = width / nColumns;
    uint32_t cellHeight = height / nRows;

    if(cellWidth == 0 || cellHeight == 0) {
        std::cout << "\tError: Invalid image area!" << std::endl;
        return;
    }

    uint32_t index = 0;
    for(uint32_t cy = 0; cy < nRows; ++ cy) {
        for(uint32_t cx = 0; cx < nColumns; ++ cx) {

            bool foundBegin = false;
            uint32_t xBegin = 0;
            for(uint32_t x = 0; x < cellWidth; ++ x) {
                for(uint32_t y = 0; y < cellHeight; ++ y) {
                    uint32_t absX = (cx * cellWidth) + x;
                    uint32_t absY = (cy * cellHeight) + y;
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
                font.glyphWidth[index] = 0.f;
                font.glyphStartX[index] = 0.f;
            }

            // There is a beginning, so find the ending
            else {
                bool foundEnd = false;
                uint32_t xEnd = 0;
                for(uint32_t x = cellWidth - 1; x >= 0; -- x) {
                    for(uint32_t y = 0; y < cellHeight; ++ y) {
                        uint32_t absX = (cx * cellWidth) + x;
                        uint32_t absY = (cy * cellHeight) + y;
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
                    float diff = (xEnd - xBegin) + 1;
                    float startX = xBegin;

                    font.glyphWidth[index] = diff / ((float) cellWidth);
                    font.glyphStartX[index] = startX / ((float) cellWidth);
                }

            }

            //std::cout << "\t" << ((char) index) << "\tAdv: " << font.advances[index] << std::endl;

            ++ index;
        }
    }
}

void convertFont(const boost::filesystem::path& fromFile, const boost::filesystem::path& outputFile, const Json::Value& params, bool modifyFilename) {

    Json::Value fontData;
    {
        std::ifstream fileStream(fromFile.string().c_str());
        fileStream >> fontData;
        fileStream.close();
    }

    const Json::Value& metricsData = fontData["metrics"];

    if(metricsData.isNull()) {
        std::cout << "\tError: Font metrics specification absent!" << std::endl;
        return;
    }

    const Json::Value& renderingData = fontData["rendering"];
    if(renderingData.isNull()) {
        std::cout << "\tError: Font rendering specification absent!" << std::endl;
    }

    FontDesc font;

    font.baseline = metricsData["baseline"].asFloat();
    font.padding = metricsData["padding"].asFloat();
    font.texture = renderingData["texture"].asString();

    generateMetrics(font, fromFile.parent_path() / (metricsData["imageFile"].asString()));

    // Load special cases
    const Json::Value& manualData = metricsData["manual"];
    if(!manualData.isNull())
    {
        for(Json::Value::const_iterator it = manualData.begin(); it != manualData.end(); ++ it) {
            const Json::Value& glyphData = *it;

            uint32_t gCode = glyphData["code"].asUInt();
            float gWidth = glyphData["width"].asFloat();

            font.glyphWidth[gCode] = gWidth;

        }
    }

    {
        std::ofstream outputData(outputFile.string().c_str(), std::ios::out | std::ios::binary);

        writeString(outputData, font.texture);
        writeF32(outputData, font.baseline);
        writeF32(outputData, font.padding);
        for(uint32_t i = 0; i < 256; ++ i) {
            writeF32(outputData, font.glyphWidth[i]);
            writeF32(outputData, font.glyphStartX[i]);
        }

        outputData.close();
    }
}
