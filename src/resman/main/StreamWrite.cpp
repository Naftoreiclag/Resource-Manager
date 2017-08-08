/*
 *  Copyright 2016-2017 James Fong
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "StreamWrite.hpp"

namespace resman {

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
    writeU32(output, serializeFloat32(value));
}
void readF32(std::ifstream& input, float& value) {
    value = readF32(input);
}
float readF32(std::ifstream& input) {
    return deserializeFloat32(readU32(input));
}

void writeF64(std::ofstream& output, double value) {
    writeU64(output, serializeFloat64(value));
}
void readF64(std::ifstream& input, double& value) {
    value = readF64(input);
}
double readF64(std::ifstream& input) {
    return deserializeFloat64(readU64(input));
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

uint64_t serializeFloat(long double fInput, uint16_t totalBits, uint16_t expBits) {
    if(fInput == 0.0) return 0;
    uint16_t sigBits = totalBits - expBits - 1;
    int64_t sign;
    long double fNormalized;
    if(fInput < 0) {
        sign = 1;
        fNormalized = -fInput;
    } else {
        sign = 0;
        fNormalized = fInput;
    }
    // Arguably better than using log2l(...)
    uint32_t exponent = 0;
    while(fNormalized >= 2.0) {
        fNormalized /= 2.0;
        ++ exponent;
    }
    while(fNormalized < 1.0) {
        fNormalized *= 2.0;
        -- exponent;
    }
    fNormalized = fNormalized - 1.0;
    int64_t significand = fNormalized * ((1LL << sigBits) + 0.5f);
    int64_t exp = exponent + (1 << (expBits - 1)) - 1;
    return
        sign << (totalBits - 1) |
        exp << sigBits |
        significand;
}

long double deserializeFloat(uint64_t iInput, uint16_t totalBits, uint16_t expBits) {
    if(iInput == 0) return 0.0;
    uint16_t sigBits = totalBits - expBits - 1;
    long double fOutput = iInput & ((1LL << sigBits) - 1);
    fOutput = (fOutput / (1LL << sigBits)) + 1;
    int64_t exponent = 
        ((iInput >> sigBits) & ((1LL << expBits) - 1)) - 
        ((1 << (expBits - 1)) - 1);
    if(exponent > 0) {
        fOutput *= 1 << exponent;
    } else if(exponent < 0) {
        fOutput /= 1 << -exponent;
    }
    return fOutput * ((iInput >> (totalBits - 1)) & 1 ? -1 : 1);
}
uint32_t serializeFloat32(float fInput) { return serializeFloat(fInput, 32, 8); }
float deserializeFloat32(uint32_t iInput) { return deserializeFloat(iInput, 32, 8); }
uint64_t serializeFloat64(double fInput) { return serializeFloat(fInput, 64, 11); }
double deserializeFloat64(uint64_t iInput) { return deserializeFloat(iInput, 64, 11); }

/*
    {
        std::ofstream outputData("test.bin", std::ios::out | std::ios::binary);
        writeU8(outputData, '$');
        writeU16(outputData, 4321);
        writeU32(outputData, 9999999);
        writeU64(outputData, 9876543210L);
        writeString(outputData, "Hello World");
        writeBool(outputData, false);
        writeF32(outputData, 3.1415926f);
        writeF64(outputData, 3.1415926535897932384626433832795028841971694);
    }
    {
        std::ifstream inputData("test.bin", std::ios::in | std::ios::binary);
        std::cout << readU8(inputData) << std::endl;
        std::cout << readU16(inputData) << std::endl;
        std::cout << readU32(inputData) << std::endl;
        std::cout << readU64(inputData) << std::endl;
        std::cout << readString(inputData) << std::endl;
        std::cout << readBool(inputData) << std::endl;
        std::cout << readF32(inputData) << std::endl;
        std::cout << readF64(inputData) << std::endl;
    }
*/

} // namespace resman
