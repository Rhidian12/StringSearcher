#include "StringSearcher.h"

#include <chrono>
#include <filesystem>
#include <string>

/*
command line format:
	StringSearcher.exe [--recursive N | -r  N] [--ignorecase | -i] [--file <file> | -f <file>] <strings> <mask>

	command line options :
		--recursive [N]		search current directory and subdirectories, limited to N levels deep of sub directories. If N is not given or 0, this is unlimited
		--ignorecase		ignore case of characters
		--file				file to look through (required when --recursive or -r are not specified)
		-r					search current directory and subdirectories. Same as --recursive 0
		-i					ignore case of characters
		-f					file to look through (required when --recursive or -r are not specified)

	example:
		D:\ExampleDir\> StringSearch.exe -i --file hello_world.txt "Hello World"
		D:\ExampleDir\> StringSearch.exe --recursive 3 "Hello World"
		D:\ExampleDir\> StringSearch.exe --recursive -i Hello!
*/

namespace RDW_SS
{
	namespace
	{
		static bool IsArgDigit(char* pArg)
		{
			while (*pArg != '\0')
			{
				if (!std::isdigit(*pArg++)) return false;
			}

			return true;
		}

		static void ParseCmdArgs(int argc, char* argv[], std::string& stringToSearch, std::string& mask, bool& ignoreCase, bool& recursivelySearch, int32_t& recursiveDepth, std::string& fileToSearch)
		{
			for (int i{ 1 }; i < argc; ++i)
			{
				const std::string& currentArg{ argv[i] };

				if (currentArg == "--recursive")
				{
					recursivelySearch = true;

					// Check if next arg is a number
					if (i < argc - 1 && IsArgDigit(argv[i + 1]))
					{
						recursiveDepth = std::stoi(argv[++i]);
					}
				}
				else if (currentArg == "-r")
				{
					recursivelySearch = true;
				}
				else if (currentArg == "--ignorecase" || currentArg == "-i")
				{
					ignoreCase = true;
				}
				else if (currentArg == "--file" || currentArg == "-f")
				{
					if (i < argc - 1)
					{
						fileToSearch = argv[++i];
					}
					else
					{
						std::cout << "Warning: Missing argument for --file (-f)\n";
					}
				}
				else if (currentArg[0] == '"')
				{
					stringToSearch = currentArg;
					stringToSearch.pop_back();
					stringToSearch.erase(stringToSearch.begin());
				}
				else
				{
					// no cmd, so fill in stringToSearch and mask, in that order
					if (stringToSearch.empty())
					{
						stringToSearch = currentArg;
					}
					else if (mask.empty())
					{
						mask = currentArg;
					}
					else
					{
						std::cout << "Warning, argument " << currentArg << " is unknown and being discarded\n";
					}
				}
			}
		}

		static bool CheckCmdArgs(const std::string& stringToSearch, const std::string& fileToSearch, const bool recursivelySearch)
		{
			if (!recursivelySearch)
			{
				if (fileToSearch.empty() || fileToSearch.find("*") != std::string::npos) return false;
			}

			if (!fileToSearch.empty() && !std::filesystem::path{ fileToSearch }.has_extension())
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
	using clock = std::chrono::high_resolution_clock;
	const clock::time_point start{ clock::now() };

	constexpr uint8_t MIN_NR_OF_ARGS{ 2 };
	constexpr uint8_t MAX_NR_OF_ARGS{ 7 };

	const uint8_t actualNrOfArgs{ static_cast<uint8_t>(argc - 1) };
	if (!(actualNrOfArgs >= MIN_NR_OF_ARGS && actualNrOfArgs <= MAX_NR_OF_ARGS))
	{
		std::cout << "Not enough arguments\n";
		RDW_SS::PrintHelp();
		return 1;
	}

	const std::string currentDir{ std::filesystem::current_path().string() };
	
	std::string stringToSearch{}, mask{}, fileToSearch{};
	bool ignoreCase{}, recursivelySearch{};
	int32_t recursiveDepth{};

	RDW_SS::ParseCmdArgs(argc, argv, stringToSearch, mask, ignoreCase, recursivelySearch, recursiveDepth, fileToSearch);

	if (!RDW_SS::CheckCmdArgs(stringToSearch, fileToSearch, recursivelySearch))
	{
		std::cout << "Incorrect argument usage\n";
		RDW_SS::PrintHelp();
		return 1;
	}

	if (!recursivelySearch && std::filesystem::path{ fileToSearch }.is_relative())
	{
		fileToSearch = currentDir + "\\" + fileToSearch;
	}

	RDW_SS::StringSearchStatistics statistics{};
	std::unordered_map<std::string, std::vector<uint32_t>> foundStrings{};
	RDW_SS::IsStringInFile(currentDir, fileToSearch, mask, stringToSearch, ignoreCase, recursivelySearch, recursiveDepth, foundStrings, &statistics);

	std::cout << "Searched through " << statistics.NumberOfFilesSearched << " files\n";

	if (!foundStrings.empty())
	{
		size_t nrOfOccurences{};
		std::for_each(foundStrings.cbegin(), foundStrings.cend(), [&nrOfOccurences](const auto& kvPair)->void { nrOfOccurences += kvPair.second.size(); });

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

	std::cout << "Finished in " << std::chrono::duration_cast<std::chrono::milliseconds>(clock::now() - start).count() << " milliseconds\n";

	return 0;
}