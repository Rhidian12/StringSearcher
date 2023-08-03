#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#ifndef RDW_STRINGSEARCHER_H
#define RDW_STRINGSEARCHER_H

namespace RDW_SS
{
	namespace Detail
	{
		inline static bool IsStringInFile_File(const std::string& fileToLookThrough, std::string stringToSearch, const bool ignoreCase, std::unordered_map<std::string, std::vector<uint32_t>>& foundStrings)
		{
			std::ifstream stream{};

			stream.open(fileToLookThrough);

			if (!stream.is_open())
			{
				std::cout << "Could not open file: " << fileToLookThrough << "\n";
				return false;
			}

			const auto stringToLowercase = [](std::string& str)->void
			{
				std::transform(str.begin(), str.end(), str.begin(), [](char c) { return std::tolower(c); });
			};

			if (ignoreCase) stringToLowercase(stringToSearch);

			bool foundMatch = false;
			uint32_t lineNumber{ 1 }; // Notepad++ starts counting at 1, so let's do the same
			std::string line{};
			while (std::getline(stream, line))
			{
				if (ignoreCase) stringToLowercase(line);

				if (line.find(stringToSearch) != std::string::npos)
				{
					foundMatch = true;
					foundStrings[fileToLookThrough].push_back(lineNumber);
				}
			}

			stream.close();
			return foundMatch;
		}

		inline static bool IsStringInFile_Dir(const std::string& dir, const std::string& fileToLookThrough, std::string stringToSearch, const bool ignoreCase, std::unordered_map<std::string, std::vector<uint32_t>>& foundStrings)
		{
			const std::filesystem::path directory{ dir };
			bool foundMatch = false;

			for (const std::filesystem::path& path : std::filesystem::directory_iterator(directory))
			{
				if (std::filesystem::is_directory(path))
				{
					if (IsStringInFile_Dir(path.string(), fileToLookThrough, stringToSearch, ignoreCase, foundStrings))
					{
						foundMatch = true;
					}
				}
				else if (std::filesystem::is_regular_file(path))
				{
					if (!fileToLookThrough.empty())
					{
						const std::string filename{ fileToLookThrough.substr(0, fileToLookThrough.find(".")) };
						const std::string extension{ fileToLookThrough.substr(fileToLookThrough.find(".")) };

						if (filename != "*" && filename != path.filename().string()) continue;
						if (extension != "*" && extension != path.extension().string()) continue;
					}

					if (IsStringInFile_File(path.string(), stringToSearch, ignoreCase, foundStrings))
					{
						foundMatch = true;
					}
				}
			}

			return foundMatch;
		}
	}

	inline static bool IsStringInFile(const std::string& currentDir, const std::string& fileToLookThrough, const std::string& stringToSearch, const bool ignoreCase, const bool recursivelySearch, std::unordered_map<std::string, std::vector<uint32_t>>& foundStrings)
	{
		if (recursivelySearch) return Detail::IsStringInFile_Dir(currentDir, fileToLookThrough, stringToSearch, ignoreCase, foundStrings);
		else return Detail::IsStringInFile_File(fileToLookThrough, stringToSearch, ignoreCase, foundStrings);
	}

	inline static void PrintHelp()
	{
		std::cout << "command line options :\n";
		std::cout << "/s	search current directory and subdirectories\n";
		std::cout << "/i	ignore case of characters\n";
		std::cout << "/c	string to search for (required)\n";
		std::cout << "/f	files to look through (required if /s is not specified)\n";
	}
}

#endif // RDW_STRINGSEARCHER_H