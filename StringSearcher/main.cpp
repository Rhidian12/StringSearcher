#include "StringSearcher.h"

#include <filesystem>
#include <string>

/*
command line options :
	/s	search current directory and subdirectories
	/i	ignore case of characters
	/c	string to search for (required)
	/f	files to look through (required if /s is not specified)

	example:
	D:\ExampleDir\> StringSearch.exe /s /c Hello World
	D:\ExampleDir\> StringSearch.exe /s /f *.ini /c Hello World
	D:\ExampleDir\> StringSearch.exe /i /f HelloWorld.txt /c hello world
*/

namespace RDW_SS
{
	namespace
	{
		static void ParseCmdArgs(int argc, char* argv[], std::string& stringToSearch, std::string& fileToLookThrough, std::string& currentDir, bool& ignoreCase, bool& recursivelySearch)
		{
			currentDir = std::filesystem::current_path().string();

			for (int i{ 1 }; i < argc; ++i)
			{
				const std::string& currentArg{ argv[i] };

				if (currentArg == "/s")
				{
					recursivelySearch = true;
				}
				else if (currentArg == "/i")
				{
					ignoreCase = true;
				}
				else if (currentArg == "/c" && i < argc - 1)
				{
					stringToSearch = argv[++i];
				}
				else if (currentArg == "/f" && i < argc - 1)
				{
					fileToLookThrough = argv[++i];
				}
			}
		}

		static bool CheckCmdArgs(const std::string& stringToSearch, const std::string& fileToLookThrough, const bool recursivelySearch)
		{
			if (!recursivelySearch)
			{
				if (fileToLookThrough.empty() || fileToLookThrough.find("*") != std::string::npos) return false;
			}

			if (!fileToLookThrough.empty() && !std::filesystem::path{ fileToLookThrough }.has_extension())
			{
				return false;
			}

			if (stringToSearch.empty()) return false;

			return true;
		}
	}
}

int main(int argc, char* argv[])
{
	const uint8_t MIN_NR_OF_ARGS{ 2 };
	const uint8_t MAX_NR_OF_ARGS{ 6 };

	if (!(argc >= MIN_NR_OF_ARGS && argc <= MAX_NR_OF_ARGS))
	{
		std::cout << "Not enough arguments\n";
		RDW_SS::PrintHelp();
		return 1;
	}

	std::string stringToSearch{}, fileToLookThrough{}, currentDir{};
	bool ignoreCase, recursivelySearch{};

	RDW_SS::ParseCmdArgs(argc, argv, stringToSearch, fileToLookThrough, currentDir, ignoreCase, recursivelySearch);

	if (!RDW_SS::CheckCmdArgs(stringToSearch, fileToLookThrough, recursivelySearch))
	{
		std::cout << "Incorrect argument usage\n";
		RDW_SS::PrintHelp();
		return 1;
	}

	if (!recursivelySearch && std::filesystem::path{ fileToLookThrough }.is_relative())
	{
		fileToLookThrough = currentDir + "\\" + fileToLookThrough;
	}

	std::unordered_map<std::string, std::vector<uint32_t>> foundStrings{};
	if (RDW_SS::IsStringInFile(currentDir, fileToLookThrough, stringToSearch, ignoreCase, recursivelySearch, foundStrings))
	{
		size_t nrOfOccurences{};
		std::for_each(foundStrings.cbegin(), foundStrings.cend(), [&nrOfOccurences](const auto& kvPair)->void {nrOfOccurences += kvPair.second.size(); });

		std::cout << "Found " << nrOfOccurences << " of string search occurences across " << foundStrings.size() << " files\n";

		for (const auto& [file, lineNumbers] : foundStrings)
		{
			std::cout << "Found " << lineNumbers.size() << " occurences in " << file << " at lines: ";
			for (size_t i{}; i < lineNumbers.size() - 1; ++i)
			{
				std::cout << lineNumbers[i] << ", ";
			}
			std::cout << lineNumbers.back() << "\n";
		}
	}
	else
	{
		std::cout << "No occurences found!";
	}

	return 0;
}