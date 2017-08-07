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
# - ASSIMP_FOUND
# - ASSIMP_INCLUDE_DIRS
# - ASSIMP_LIBRARIES

set(ASSIMP_FOUND FALSE)
find_path(ASSIMP_INCLUDE_DIRS NAMES "assimp/Importer.hpp")
find_library(ASSIMP_LIBRARIES NAMES "assimp")

if(ASSIMP_INCLUDE_DIRS AND ASSIMP_LIBRARIES)
    set(ASSIMP_FOUND TRUE)
endif()

mark_as_advanced(
    ASSIMP_INCLUDE_DIRS
    ASSIMP_LIBRARIES
)
