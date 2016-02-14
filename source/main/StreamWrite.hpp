#ifndef STREAMWRITE_HPP
#define STREAMWRITE_HPP

#include <fstream>
#include <string>
#include <stdint.h>

void writeU32(std::ofstream& output, const uint32_t& value);

void writeBool(std::ofstream& output, const bool& value);

void writeU16(std::ofstream& output, const uint16_t& value);

void writeU8(std::ofstream& output, const uint8_t& value);

void writeF32(std::ofstream& output, const float& value);

#endif // STREAMWRITE_HPP
