#include "StreamWrite.hpp"

void writeU32(std::ofstream& output, const uint32_t& value) {
    output.write((char*) &value, sizeof value);
}

void writeBool(std::ofstream& output, const bool& value) {
    char data;
    data = value ? 1 : 0;
    output.write(&data, sizeof data);
}

void writeU16(std::ofstream& output, const uint16_t& value) {
    output.write((char*) &value, sizeof value);
}

void writeU8(std::ofstream& output, const uint8_t& value) {
    output.write((char*) &value, sizeof value);
}

void writeF32(std::ofstream& output, const float& value) {
    output.write((char*) &value, sizeof value);
}

void writeString(std::ofstream& output, const std::string& value) {
    writeU32(output, value.length());
    output << value;
}
