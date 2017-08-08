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

#ifndef STREAMWRITE_HPP
#define STREAMWRITE_HPP

#include <fstream>
#include <string>
#include <stdint.h>

namespace resman {

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

// IEEE Standard for Floating-Point Arithmetic (IEEE 754)
uint64_t serializeFloat(long double fInput, uint16_t totalBits, uint16_t expBits);
long double deserializeFloat(uint64_t iInput, uint16_t totalBits, uint16_t expBits);
uint32_t serializeFloat32(float fInput);
float deserializeFloat32(uint32_t iInput);
uint64_t serializeFloat64(double fInput);
double deserializeFloat64(uint64_t iInput);

} // namespace resman

#endif // STREAMWRITE_HPP
