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

# This file contains a listing of all of the source files used in the engine
# build target. Populates a list called PGLOCAL_SOURCES_LIST

# Preferred method of adding source items is through the Python script in:
# `util/Generate*SrcList.py`

# This function appends the provided string list to PGLOCAL_SOURCES_LIST
set(PGLOCAL_SOURCES_LIST "")
foreach(fname 

"../thirdparty/easyloggingpp/easylogging++.cc"
"../thirdparty/jsoncpp/jsoncpp.cpp"
"../thirdparty/murmurhash3/MurmurHash3.cpp"
"Main.cpp"
"logger/Logger.cpp"
"main/ConvertFont.cpp"
"main/ConvertGenericJson.cpp"
"main/ConvertGeometry.cpp"
"main/ConvertGlsl.cpp"
"main/ConvertImage.cpp"
"main/ConvertMiscellaneous.cpp"
"main/ConvertWaveform.cpp"
"main/Convert_bgfx_Shader.cpp"
"main/Expand_bgfx_Shader.cpp"
"main/JsonUtil.cpp"
"main/StreamWrite.cpp"

)
list(APPEND PGLOCAL_SOURCES_LIST 
        "${PGLOCAL_SOURCE_DIR}/${PGLOCAL_PROJ_NAME}/${fname}")
endforeach()
