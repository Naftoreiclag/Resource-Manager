#include "StreamWrite.hpp"

void writeU32(std::ofstream& output, uint32_t value) {
    output.write((char*) &value, sizeof value);
}

void writeU16(std::ofstream& output, uint16_t value) {
    output.write((char*) &value, sizeof value);
}

void writeU8(std::ofstream& output, uint8_t value) {
    output.write((char*) &value, sizeof value);
}

void writeBool(std::ofstream& output, bool value) {
    writeU8(output, value);
}

void writeF32(std::ofstream& output, float value) {
    output.write((char*) &value, sizeof value);
}

void writeString(std::ofstream& output, const std::string& value) {
    uint32_t length = value.length();
    writeU32(output, length);
    output.write(value.c_str(), length);
}
