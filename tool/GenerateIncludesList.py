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

import os
from Common import indexFiles
from Common import writeWithReplacements
from Common import get_project_name

dirListVector = '### DIRECTORY LIST ###'
boilerplateFilename = 'IncludeListBoilerplate.cmake'

def addQuotesAndPrefix(list):
    for i in range(len(list)):
        list[i] = '"thirdparty/' + str(list[i]) + '"'

def generate(destination, dirList):
    addQuotesAndPrefix(dirList)
    replacements = {}
    replacements[dirListVector] = dirList
    writeWithReplacements(boilerplateFilename, destination, replacements)
    
# Generate for main sources
_, dirList, __ = indexFiles('../src/thirdparty/', \
        ['.h', '.hpp', '.c', '.cpp', '.cc'], [], False, 1)
dirList.sort()
print('Third-party Projects: ' + str(len(dirList)))
generate('../cmake/IncludesList.cmake', dirList)
