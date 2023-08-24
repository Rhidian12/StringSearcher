# StringSearcher

StringSearcher is a (Windows-only) command-line tool loosely based on findstr (Windows).

### Usage
It is meant as a Command line tool, but all functionality is contained in the StringSearcher header, which can be included in any C++ project.
This tool is basically as fast as findstr on Windows

Command line format:
		StringSearcher.exe [--recursive N | -r  N] [--ignorecase | -i] [--file <file> | -f <file>] <strings> <mask>

		command line options :
			--recursive [N]		search current directory and subdirectories, limited to N levels deep of sub directories. If N is not given or 0, this is unlimited
			--ignorecase		ignore case of characters
			--file				file to look through (required when --recursive or -r are not specified, but not available if -r or --recursive is specified)
			-r					search current directory and subdirectories. Same as --recursive 0
			-i					ignore case of characters
			-f					file to look through (required when --recursive or -r are not specified, but not available if -r or --recursive is specified)

		example:
			D:\ExampleDir\> StringSearcher.exe -i --file hello_world.txt "Hello World"
			D:\ExampleDir\> StringSearcher.exe --recursive 3 "Hello World"
			D:\ExampleDir\> StringSearcher.exe --recursive -i Hello!

### Future Work
- Add Regex support
- Make it faster
- Add timing information
