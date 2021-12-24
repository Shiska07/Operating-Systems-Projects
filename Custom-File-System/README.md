# Custom-File-System

This program implements a user space protable index allocated file system in C. The size of the file system is ~33MB which can be easily modified by changing BLOCK_SIZE and NUM_BLOCKS
in the code. The program allows the user to create a new file system or open an existing file system. 

The following commands are supported:

1. createfs name    (creates a new file system image)
2. put filename     (copies local files to the file system image)
3. get filename     (copies file from the file system to the cwd)
4. list             (list all files in the image. Note: list -h will als list hidden files)
5. df       (displays free space avaialble)
6. open     (opens a file system image)
7. close    (closes current a file system image)
8. savefs   (saves the current file system image to the drive)
9. attrib   (adds(+) or deletes(-) file attributes eg:attrib +h(hidden) attrib +r(read only))
10. del filename

The program currently only supports single level directories.
