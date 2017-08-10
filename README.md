# Naftoreiclag's Resource Manager

*Currently thinking of a better name...*

## Description

This yet-unnamed project simplifies packaging of files meant to be shipped with a piece of software to be utilized at run-time by said software (as "resources" or "assets"). I created this software to solve various issues I kept running into while developing a video game:


- I wanted to organize my resources into folders while I worked on them... but I wanted all resources to be in one directory while reading them through code.
- I wanted to use a consistent file format for all files of a common medium (e.g. 3D models, images, sound) to cut down on the number of file-reading libraries I would need to ship with my game... but the format that was easiest to work with often was not the format that was easiest to read from.
- I needed my own file format for 3D models.
- I wanted to easily manage the conversion of high-quality source files into lower-quality RAM-friendly files. (e.g. automatic downsampling of 4k "source, tga" textures into 1k "GPU-friendly" textures)
- I did not want to constantly keep resource filenames and paths in sync with the string literals in my code.


This project processes a resource project directory into a single "packaged" directory through the following steps:


1. Search the project tree for JSON files describing how to process neighboring resource files into the "run-time ready" format and specifying an absolute global name for each resource ("King.image" instead of "King.png").
2. Process all files as specified and output into a single directory.
3. Produce a single JSON dictionary translating those absolute global names into files which are read from ("the contents of 'King.image' are found in '60_King.png'")


Output file formats:


|Medium|Format|
|---|---|
|Image|.png|
|3D Model|custom|
## Building & Running

Uses [CMake](https://cmake.org/) for build configurations.

### Dependencies

myproject requires the following libraries to be built correctly.

- [Assimp](http://assimp.sourceforge.net/)
  for 3D data loading
- [Boost](http://www.boost.org/)
  for all-around usefulness
- [Easylogging++](https://github.com/muflihun/easyloggingpp/)
  for logging
- [FLAC](https://xiph.org/flac/)
- [JsonCpp](https://github.com/open-source-parsers/jsoncpp/)
  for serialization, config files
- [Ogg](https://www.xiph.org/ogg/)
- [stb](https://github.com/nothings/stb)
  for image loading
- [Vorbis](http://www.vorbis.com/)

*Each library listed above is the copyright of its respective author(s). Please
see individual library homepages for more accurate licensing information.*

### Utilities

In the `tool/` directory are Python scripts that you may find useful:
- `GenerateSrcLists.py` populates the cmake files 
  `cmake/MainSrcList.cmake` and `cmake/TestSrcList.cmake` from the contents 
  of the `src/` directory.
- `SyncEngineCodelite.py` populates a [Codelite](https://codelite.org/)
  project file (by default, `ide/Codelite/Codelite.project`).
- `RefactorHeaderIncludes.py` was used to refactor older source and header
  files to use the new source folder format.

### License

This project's source code files (located in this repository under the
`src/resman/` directory) are distributed under the
[Apache License, Version 2.0](http://www.apache.org/licenses/LICENSE-2.0).
A copy of this license is available in the `LICENSE` file located in the
root of this repository.

Included in this repository may be source code for other open-source projects
that are embedded in this project. This source code for such third-party
projects is available in the `src/thirdparty` subdirectory. 
Such projects may be subject to different licensing terms. Please see 
individual files for details.

Files found outside the `src/` repository, including but not limited to those
located in the `cmake/` directory, may also be subject to different licensing
terms. See individual files for details.
