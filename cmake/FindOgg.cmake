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
# - OGG_FOUND
# - OGG_INCLUDE_DIRS
# - OGG_LIBRARIES

set(OGG_FOUND FALSE)
find_path(OGG_INCLUDE_DIRS NAMES "ogg/ogg.h")
find_library(OGG_LIBRARIES NAMES "ogg")

if(OGG_INCLUDE_DIRS AND OGG_LIBRARIES)
    set(OGG_FOUND TRUE)
endif()

mark_as_advanced(
    OGG_INCLUDE_DIRS
    OGG_LIBRARIES
)
