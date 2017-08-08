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

def writeWithReplacements(boilerplateFilename, outputFilename, replacements):
    ''' Outputs a copy of the file at "boilerplateFilename" to "outputFilename",
    except with any line starting with a key in the dictionary "replacements"
    is replaced with the corresponding value.
    
    If that corresponding value is a string, the entire line is replaced with
    that string. If the value is a list of strings, then the entire line is
    replaced with that list's elements joined together as one string with
    newline characters between each element.
    '''
    with open(outputFilename, 'w') as outputFile:
        with open(boilerplateFilename, 'r') as boilerplateFile:
            for line in boilerplateFile:
                replaced = False
                for key in replacements:
                    if line.startswith(key):
                        value = replacements[key]
                        if type(value) == type([]):
                            for entry in value:
                                outputFile.write(str(entry) + '\n')
                        else:
                            outputFile.write(str(value) + '\n')
                        replaced = True
                        break
                if not replaced:
                    outputFile.write(line)

def indexFiles(searchPath, allowedFileExts, blacklistedDirs, includeRoot=True):
    ''' Indexes all of the files located in searchPath.
    
    Only files with the extensions listed in allowedFileExts are included.
    Any files located in the directories (or subdirectories thereof) listed in 
    blacklistedDirs are excluded.
    
    Files at the root of the search path are included iff includeRoot is True.
    
    First return is a list of all of the files, relative to searchPath.
    
    Second return is all of the directories relative to searchPath containing
    at least one file listed in the first return.
    
    Third return is a dictionary mapping filenames to full paths relative to
    searchPath.
    '''

    sourceList = []
    includeDirList = []
    fnameToPath = {}

    def addSource(filepath):
        if not any(filepath.startswith(dir) for dir in blacklistedDirs) \
                and any(filepath.endswith(ext) for ext in allowedFileExts):
            sourceList.append(filepath)
            fileParentPath, filename = os.path.split(filepath)
            if fileParentPath and fileParentPath not in includeDirList:
                includeDirList.append(fileParentPath)
            if filename in fnameToPath:
                print('WARNING! File: ' + filename + ' occurs twice!')
                print('\t' + fnameToPath[filename])
                print('\t' + fileParentPath)
            else:
                fnameToPath[filename] = filepath

    def recursiveSearch(prefixStr, location):
        for node in os.listdir(location):
            nodeLocation = os.path.join(location, node)
        
            if os.path.isfile(nodeLocation):
                if not (prefixStr == '' and not includeRoot):
                    addSource(prefixStr + str(node))
            else:
                recursiveSearch(prefixStr + str(node) + '/', nodeLocation)

    recursiveSearch('', searchPath)
    sourceList = sorted(sourceList)
    includeDirList = sorted(includeDirList)

    return sourceList, includeDirList, fnameToPath

class CannotDeduceProjectName(Exception):
    pass
    
def get_project_name():
    subdirs = [dir for dir in os.listdir('../src/')]
    if 'thirdparty' in subdirs:
        subdirs.remove('thirdparty')
    if len(subdirs) != 1:
        raise CannotDeduceProjectName()
    return subdirs[0]
    