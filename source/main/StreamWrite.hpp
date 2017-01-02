#ifndef STREAMWRITE_HPP
#define STREAMWRITE_HPP

#include <fstream>
#include <string>
#include <stdint.h>

void writeU32(std::ofstream& output, uint32_t value);

void writeU16(std::ofstream& output, uint16_t value);

void writeU8(std::ofstream& output, uint8_t value);

void writeBool(std::ofstream& output, bool value);

void writeF32(std::ofstream& output, float value);

void writeString(std::ofstream& output, const std::string& value);

#endif // STREAMWRITE_HPP
