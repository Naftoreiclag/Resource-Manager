#   Copyright 2017 James Fong
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

# Populates:
# - JSONCPP_FOUND
# - JSONCPP_INCLUDE_DIRS
# - JSONCPP_SOURCE_FILES

set(JSONCPP_FOUND FALSE)
find_path(JSONCPP_INCLUDE_DIRS NAMES "json/json.h")
set(JSONCPP_SOURCE_FILES "JSONCPP_SOURCE_FILES-NOTFOUND"
    CACHE FILEPATH "Typically Jsoncpp.cpp")

if(JSONCPP_INCLUDE_DIRS)
    set(JSONCPP_FOUND TRUE)
    set(JSONCPP_SOURCE_FILES "${JSONCPP_INCLUDE_DIRS}/jsoncpp.cpp")
endif()

mark_as_advanced(
    JSONCPP_INCLUDE_DIRS
    JSONCPP_SOURCE_FILES
)
