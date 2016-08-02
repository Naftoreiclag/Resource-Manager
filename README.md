Naftoreiclag's Resource Manager
===============================
*Currently thinking of a better name...*

Description
-----------
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

Notes
-----
*(This README is a work in progress.)*

