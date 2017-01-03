#ifndef STREAMWRITE_HPP
#define STREAMWRITE_HPP

#include <fstream>
#include <string>
#include <stdint.h>

void writeU8(std::ofstream& output, uint8_t value);
void readU8(std::ifstream& input, uint8_t& value);
uint8_t readU8(std::ifstream& input);

void writeU16(std::ofstream& output, uint16_t value);
void readU16(std::ifstream& input, uint16_t& value);
uint16_t readU16(std::ifstream& input);

void writeU32(std::ofstream& output, uint32_t value);
void readU32(std::ifstream& input, uint32_t& value);
uint32_t readU32(std::ifstream& input);

void writeU64(std::ofstream& output, uint64_t value);
void readU64(std::ifstream& input, uint64_t& value);
uint64_t readU64(std::ifstream& input);

void writeF32(std::ofstream& output, float value);
void readF32(std::ifstream& input, float& value);
float readF32(std::ifstream& input);

void writeF64(std::ofstream& output, double value);
void readF64(std::ifstream& input, double& value);
double readF64(std::ifstream& input);

void writeString(std::ofstream& output, const std::string& value);
void readString(std::ifstream& input, std::string& value);
std::string readString(std::ifstream& input);

void writeBool(std::ofstream& output, bool value);
void readBool(std::ifstream& input, bool& value);
bool readBool(std::ifstream& input);

#endif // STREAMWRITE_HPP
