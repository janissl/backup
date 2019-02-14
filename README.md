# backup

A simple commandline utility to copy latest versions of files to a user-defined location.<br><br>
* If names of directories and files in the source directory tree only contain ANSI characters
it also works on Windows.<br>
* A structure of source directories is preserved at the destination.<br>
* If the destination root does not exist, its parent directory must be existing.<br>
* A log file (named _last.log_) is saved next to the executable file on each run and includes all source and destination
paths as well as error messages if any errors have occurred.<br>

### Build

Prerequisites:<br>
* cmake (v3.13+)
<br><br>

On Linux/UNIX:<br>
* _cd_ to the source root
* Execute:
```bash
mkdir build
cd build
cmake ..
make
```
__Note:__ If you have built _cmake_ from the source use `/usr/local/bin/cmake ..` instead of `cmake ..`
<br><br>

On Windows (using MinGW):<br>
* _cd_ to the source root
* Execute:
```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
```
... or use the __Build__ option in your IDE e.g. __CLion__

### Usage

On Linux/UNIX:<br>
`./backup <source_root_directory> <destination_root_directory>`

On Windows (using the PowerShell terminal):<br>
`.\backup.exe <source_root_directory> <destination_root_directory>`
