#include "StreamWrite.hpp"

// TODO: allow endianness to be specified to allow for reinterpret_cast<char*>
// Little endian is enforced for integer types

void writeU8(std::ofstream& output, uint8_t value) {
    output.write(reinterpret_cast<char*>(&value), 1);
}
void readU8(std::ifstream& input, uint8_t& value) {
    input.read(reinterpret_cast<char*>(&value), 1);
}
uint8_t readU8(std::ifstream& input) {
    uint8_t value;
    readU8(input, value);
    return value;
}

void writeU16(std::ofstream& output, uint16_t value) {
    uint8_t out[2];
    out[0] = value;
    out[1] = value >> 8;
    output.write(reinterpret_cast<char*>(out), 2);
}

void readU16(std::ifstream& input, uint16_t& value) {
    uint8_t in[2];
    input.read(reinterpret_cast<char*>(in), 2);
    value = in[0];
    value |= in[1] << 8;
}
uint16_t readU16(std::ifstream& input) {
    uint16_t value;
    readU16(input, value);
    return value;
}

void writeU32(std::ofstream& output, uint32_t value) {
    uint8_t out[4];
    out[0] = value;
    out[1] = value >> 8;
    out[2] = value >> 16;
    out[3] = value >> 24;
    output.write(reinterpret_cast<char*>(out), 4);
}
void readU32(std::ifstream& input, uint32_t& value) {
    uint8_t in[4];
    input.read(reinterpret_cast<char*>(in), 4);
    value = in[0] | in[1] << 8 | in[2] << 16 | in[3] << 24;
}
uint32_t readU32(std::ifstream& input) {
    uint32_t value;
    readU32(input, value);
    return value;
}

void writeU64(std::ofstream& output, uint64_t value) {
    uint8_t out[8];
    out[0] = value;
    out[1] = value >> 8;
    out[2] = value >> 16;
    out[3] = value >> 24;
    out[4] = value >> 32;
    out[5] = value >> 40;
    out[6] = value >> 48;
    out[7] = value >> 56;
    output.write(reinterpret_cast<char*>(out), 8);
}
void readU64(std::ifstream& input, uint64_t& value) {
    uint8_t in[8];
    input.read(reinterpret_cast<char*>(in), 8);
    value = 
        ((uint64_t) (in[0] | in[1] << 8 | in[2] << 16 | in[3] << 24)) |
        ((uint64_t) in[4]) << 32 | 
        ((uint64_t) in[5]) << 40 | 
        ((uint64_t) in[6]) << 48 | 
        ((uint64_t) in[7]) << 56;
}
uint64_t readU64(std::ifstream& input) {
    uint64_t value;
    readU64(input, value);
    return value;
}

void writeF32(std::ofstream& output, float value) {
    output.write((char*) &value, sizeof value);
}
void readF32(std::ifstream& input, float& value) {
    input.read((char*) &value, sizeof value);
}
float readF32(std::ifstream& input) {
    float value;
    readF32(input, value);
    return value;
}

void writeF64(std::ofstream& output, double value) {
    output.write((char*) &value, sizeof value);
}
void readF64(std::ifstream& input, double& value) {
    input.read((char*) &value, sizeof value);
}
double readF64(std::ifstream& input) {
    double value;
    readF64(input, value);
    return value;
}

void writeString(std::ofstream& output, const std::string& value) {
    uint32_t length = value.length();
    writeU32(output, length);
    output.write(value.c_str(), length);
}
void readString(std::ifstream& input, std::string& value) {
    uint32_t size = readU32(input);
    char* buff = new char[size + 1];
    input.read(buff, size);
    buff[size] = '\0';
    value = buff;
    delete[] buff;
}
std::string readString(std::ifstream& input) {
    std::string value;
    readString(input, value);
    return value;
}

void writeBool(std::ofstream& output, bool value) {
    writeU8(output, value);
}
void readBool(std::ifstream& input, bool& value) {
    value = readBool(input);
}
bool readBool(std::ifstream& input) {
    return readU8(input) != 0;
}
