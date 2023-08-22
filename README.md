# StringSearcher

StringSearcher is a (Windows-only) command-line tool loosely based on findstr (Windows).

### Usage
It is meant as a Command line tool, but all functionality is contained in the StringSearcher header, which can be included in any C++ project.
This tool is basically as fast as findstr on Windows

Command line options:
/s    ->    Searches current directory and sub-directories
/i    ->    Ignore all case in strings
/c    ->    String to search for (required argument)
/f    ->    If /s is specified, /f denotes a filetype and/or filename to search through in all (sub-)directories. If /s is **not** specified, this denotes the file to search through. (required argument if /s is **not** specified)

### Future Work
- Add Regex support
- Make it faster
- Add timing information
