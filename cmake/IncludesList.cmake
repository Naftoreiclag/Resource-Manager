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

# This file contains a listing of all of the additional include dirs used in the
# build targets. Populates a list called PGLOCAL_INCLUDES_LIST

# Preferred method of adding source items is through the Python script in:
# `util/GenerateIncludesList.py`

# This function appends the provided string list to PGLOCAL_INCLUDES_LIST
set(PGLOCAL_INCLUDES_LIST "")
foreach(dir 

"thirdparty/easyloggingpp"
"thirdparty/jsoncpp"
"thirdparty/murmurhash3"
"thirdparty/stb"

)
list(APPEND PGLOCAL_INCLUDES_LIST 
        "${PGLOCAL_SOURCE_DIR}/${dir}")
endforeach()
