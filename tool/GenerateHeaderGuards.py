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
from Common import get_project_name

proj_name = get_project_name()

searchPath = '../src/' + proj_name + '/'
includePrefix = proj_name + '/'

from Common import indexFiles

headerFnames, _, _ = indexFiles(searchPath, ['.hpp'], [])

import re

ifndefPattern = re.compile('\s*#ifndef\s+(\S*)\s*')
definePattern = re.compile('\s*#define\s+(\S*)\s*')
endifPattern = re.compile('\s*#endif\s+//\s*(\S*)\s*')

def makeGuardFromFname(fname):
    fname = fname.upper().replace('_', '').replace('.', '_').replace('/', '_')
    fname = ''.join(c for c in fname if c in \
            'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_')
    return fname

duplicates = {}
for headerFname in headerFnames:
    guard = None
    with open(searchPath + headerFname, 'r') as headerFile:
        # Dictionary of (has_define, has_endif) tuples
        potentialGuards = {}
        for line in headerFile:
            match = ifndefPattern.match(line)
            if match:
                potentialGuards[match.group(1)] = [False, False]
                continue
            match = definePattern.match(line)
            if match and match.group(1) in potentialGuards:
                potentialGuards[match.group(1)][0] = True
                continue
            match = endifPattern.match(line)
            if match and match.group(1) in potentialGuards:
                potentialGuards[match.group(1)][1] = True
                continue
        
        validGuards = []
        for guard, comps in potentialGuards.items():
            if comps[0] and comps[1]:
                validGuards.append(guard)
        
        if len(validGuards) > 1:
            print('Error! ' + headerFname + ' has multiple possible guards: ' \
                    + str(validGuards))
            continue
        
        if len(validGuards) == 1:
            guard = validGuards[0]
        
    if guard:
        newGuard = makeGuardFromFname(includePrefix + headerFname)
        
        if newGuard in duplicates:
            duplicates[newGuard].append(headerFname)
        else:
            duplicates[newGuard] = [headerFname]
        
        if newGuard == guard:
            continue
        print ('Replacing ' + guard + ' with ' + newGuard)
        headerLines = []
        with open(searchPath + headerFname, 'r') as headerFile:
            headerLines = headerFile.readlines()
    
        for lineIdx in range(len(headerLines)):
            headerLine = headerLines[lineIdx]
            match = ifndefPattern.match(headerLine)
            if match and match.group(1) == guard:
                headerLines[lineIdx] = '#ifndef ' + newGuard + '\n'
                continue
            match = definePattern.match(headerLine)
            if match and match.group(1) == guard:
                headerLines[lineIdx] = '#define ' + newGuard + '\n'
                continue
            match = endifPattern.match(headerLine)
            if match and match.group(1) == guard:
                headerLines[lineIdx] = '#endif // ' + newGuard + '\n'
                continue
        with open(searchPath + headerFname, 'w') as headerFile:
            headerFile.writelines(headerLines)
    else:
        print('Warning! ' + headerFname + ' has no guard!')
        
for guard, count in duplicates.items():
    if len(count) > 1:
        print('ERROR! ' + guard + ' occurs ' + len(count) + ' times! ' \
            + str(count))