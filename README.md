# backup

A simple commandline utility to copy latest versions of files to a user-defined location.<br><br>
* If names of directories and files in the source directory tree only contain ANSI characters
it should also work on Windows.<br>
* A structure of source directories is preserved at the destination.<br>
* If the destination root does not exist, its parent directory must be existing.<br>
* A log file (named _last.log_) is saved next to the executable file on each run and contains all source and destination
paths as well as error messages if some errors have occurred.<br>
#### Usage:
`./backup <source_root_directory> <destination_root_directory>`
