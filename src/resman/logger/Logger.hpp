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

#ifndef PEGR_LOGGER_LOGGER_HPP
#define PEGR_LOGGER_LOGGER_HPP

#include <easylogging++.h>

namespace pegr {
namespace Logger {

void initialize();
void cleanup();

el::Logger* log();
el::Logger* alog(const char* addon_name);

} // namespace Logger
} // namespace pegr

#endif // PEGR_LOGGER_LOGGER_HPP
