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
# - FLAC_FOUND
# - FLAC_INCLUDE_DIRS
# - FLAC_LIBRARIES

set(FLAC_FOUND FALSE)
find_path(FLAC_INCLUDE_DIRS NAMES "FLAC/all.h")
find_library(FLAC_LIBRARIES NAMES "FLAC")

if(FLAC_INCLUDE_DIRS AND FLAC_LIBRARIES)
    set(FLAC_FOUND TRUE)
endif()

mark_as_advanced(
    FLAC_INCLUDE_DIRS
    FLAC_LIBRARIES
)
