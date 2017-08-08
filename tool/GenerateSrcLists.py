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

sourceListVector = '### SOURCE LIST ###'
boilerplateFilename = 'SrcListBoilerplate.cmake'

proj_name = get_project_name()

def addQuotes(list):
    for i in range(len(list)):
        list[i] = '"' + str(list[i]) + '"'

def generate(destination, sourceList):
    addQuotes(sourceList)
    replacements = {}
    replacements[sourceListVector] = sourceList
    writeWithReplacements(boilerplateFilename, destination, replacements)

def add_thirdparty(sourceList):
    thirdparty_sl, thirdparty_dl, _ = indexFiles( \
        '../src/thirdparty/', ['.cpp', '.c', '.cc'], [], False)
    for source in thirdparty_sl:
        sourceList.append('../thirdparty/' + source)
    
# Generate for main sources
sourceList, dirList, _ = indexFiles( \
        '../src/' + proj_name + '/', ['.cpp'], ['deprecated/', 'test/'], False)
sourceList.append('Main.cpp')
add_thirdparty(sourceList)
sourceList.sort()
print('Main Sources: ' + str(len(sourceList)))
print('Main Directories: ' + str(len(dirList)))
generate('../cmake/MainSrcList.cmake', sourceList)

# Generate for test sources
sourceList, dirList, _ = indexFiles( \
        '../src/' + proj_name + '/', ['.cpp'], ['deprecated/'], False)
sourceList.append('Test.cpp')
add_thirdparty(sourceList)
sourceList.sort()
print('Test Sources: ' + str(len(sourceList)))
print('Test Directories: ' + str(len(dirList)))
generate('../cmake/TestSrcList.cmake', sourceList)
