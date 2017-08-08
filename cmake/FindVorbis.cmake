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
# - VORBIS_FOUND
# - VORBIS_INCLUDE_DIRS
# - VORBIS_LIBRARIES
set(VORBIS_FOUND FALSE)
find_path(VORBIS_INCLUDE_DIRS NAMES "vorbis/codec.h")
find_library(VORBIS_LIBRARY_VORBIS NAMES "vorbis")
find_library(VORBIS_LIBRARY_VORBISFILE NAMES "vorbisfile")
find_library(VORBIS_LIBRARY_VORBISENC NAMES "vorbisenc")

if(VORBIS_INCLUDE_DIRS
        AND VORBIS_LIBRARY_VORBIS
        AND VORBIS_LIBRARY_VORBISFILE
        AND VORBIS_LIBRARY_VORBISENC)
    set(VORBIS_FOUND TRUE)
    list(APPEND VORBIS_LIBRARIES ${VORBIS_LIBRARY_VORBIS})
    list(APPEND VORBIS_LIBRARIES ${VORBIS_LIBRARY_VORBISFILE})
    list(APPEND VORBIS_LIBRARIES ${VORBIS_LIBRARY_VORBISENC})
endif()

mark_as_advanced(
    VORBIS_INCLUDE_DIRS
    VORBIS_LIBRARY_VORBIS
    VORBIS_LIBRARY_VORBISFILE
    VORBIS_LIBRARY_VORBISENC
)
