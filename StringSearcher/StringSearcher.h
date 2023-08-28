#pragma once

#include <algorithm>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stack>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <vector>
#include <Windows.h>

#ifdef _DEBUG
#define RDW_SS_ASSERT(expr) if (!(expr))	\
							{	\
								std::cout << "Assertion triggered at line: " << __LINE__ << " in file: " << __FILE__ << "\n";	\
								__debugbreak();	\
							}
#else
#define RDW_SS_ASSERT(expr)
#endif

namespace RDW_SS
{
	namespace Detail
	{
		inline static void TransformStringToLowercase(std::string& str)
		{
			std::transform(str.begin(), str.end(), str.begin(), [](const char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
		}

		inline void SearchFilesForString(
			const std::vector<std::string>& filesToLookThrough,
			std::string stringToSearch,
			const bool ignoreCase,
			std::unordered_map<std::string, std::vector<uint32_t>>& foundStrings,
			std::mutex& mutex)
		{
			std::ifstream stream{};

			for (const std::string& filename : filesToLookThrough)
			{
				stream.open(filename);

				if (!stream.is_open())
				{
					std::cout << "Could not open file: " << filename << "\n";
					return;
				}

				if (ignoreCase)
				{
					TransformStringToLowercase(stringToSearch);
				}

				// Notepad++ starts counting at 1, so let's do the same
				std::string line{};
				for (uint32_t lineNumber{ 1 }; std::getline(stream, line); ++lineNumber)
				{
					if (ignoreCase)
					{
						TransformStringToLowercase(line);
					}

					if (line.find(stringToSearch) != std::string::npos)
					{
						const std::scoped_lock<std::mutex> lock{ mutex };
						foundStrings[filename].push_back(lineNumber);
					}
				}

				stream.close();
			}
		}

		inline bool IsFilenameValid(const std::string_view filename, const std::string_view filenameFilter)
		{
			if (filenameFilter == "*") return true;

			return filename.find(filenameFilter) != std::string_view::npos;
		}

		inline bool IsExtensionValid(const std::string_view filename, const std::string_view extensionFilter)
		{
			if (extensionFilter == "*") return true;

			return filename.find(extensionFilter) != std::string_view::npos;
		}

		inline bool ShouldDirectoryBeConsidered(const std::string& rootDir, const std::string& currentDir, const uint32_t recursiveDepth)
		{
			if (recursiveDepth == 0) return true;

			const int64_t depth{ std::count(currentDir.cbegin(), currentDir.cend(), '\\') - std::count(rootDir.cbegin(), rootDir.cend(), '\\') };

			return depth <= recursiveDepth;
		}

		inline std::vector<std::string> GetAllFilesInDirectory(const std::string& rootDir, const std::string& mask, const uint32_t recursiveDepth)
		{
			std::vector<std::string> files{};
			files.reserve(80);

			WIN32_FIND_DATAA findFileData;
			HANDLE fileHandle = INVALID_HANDLE_VALUE;

			std::stack<std::string> fileStack{};
			fileStack.push(rootDir);

			const std::string filterFilename{ mask.empty() ? "*" : mask.substr(0, mask.find(".")) };
			const std::string filterExtension{ mask.empty() ? "*" : mask.substr(mask.find(".")) };

			while (!fileStack.empty())
			{
				const std::string child{ fileStack.top() };
				fileStack.pop();

				const std::string rootWildcard{ child + "\\*" };
				fileHandle = FindFirstFileA(rootWildcard.c_str(), &findFileData);
				if (fileHandle == INVALID_HANDLE_VALUE)
				{
					std::cout << "FindFirstFile failed on file" << child << "\n";
					if (!fileStack.empty())
					{
						continue;
					}
					else
					{
						return files;
					}
				}

				do
				{
					if (strcmp(findFileData.cFileName, ".") == 0 ||
						strcmp(findFileData.cFileName, "..") == 0)
					{
						continue;
					}

					if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						const std::string directoryName{ child + "\\" + findFileData.cFileName };

						if (ShouldDirectoryBeConsidered(rootDir, directoryName, recursiveDepth)) fileStack.push(directoryName);
					}
					else
					{
						if (IsFilenameValid(findFileData.cFileName, filterFilename) && IsExtensionValid(findFileData.cFileName, filterExtension))
						{
							files.push_back(child + "\\" + findFileData.cFileName);
						}
					}
				} while (FindNextFileA(fileHandle, &findFileData) != 0);
			}

			FindClose(fileHandle);

			return files;
		}
	}

	struct StringSearchStatistics final
	{
		int32_t NumberOfFilesSearched;
	};

	inline static void IsStringInFile(
		const std::string& currentDir,
		const std::string& fileToSearch,
		const std::string& mask,
		const std::string& stringToSearch,
		const bool ignoreCase,
		const bool recursivelySearch,
		const uint32_t recursiveDepth,
		std::unordered_map<std::string, std::vector<uint32_t>>& foundStrings,
		StringSearchStatistics* pStatistics)
	{
		if (recursivelySearch)
		{
			size_t nrOfThreads{ std::thread::hardware_concurrency() };
			const std::vector<std::string> allFiles{ Detail::GetAllFilesInDirectory(currentDir, mask, recursiveDepth) };

			if (nrOfThreads > allFiles.size()) nrOfThreads = allFiles.size();

			const size_t nrOfFilesPerThread{ allFiles.size() / nrOfThreads };

			if (nrOfFilesPerThread == 0) return;

			std::vector<std::jthread> threads{};
			threads.reserve(nrOfThreads);
			std::mutex mutex{};

			size_t nrOfFilesAdded{};
			for (size_t i{}; i < nrOfThreads - 1; ++i)
			{
				const size_t startOffset{ nrOfFilesPerThread * i };
				const size_t endOffset{ nrOfFilesPerThread * (i + 1) };

				nrOfFilesAdded += endOffset - startOffset;

				RDW_SS_ASSERT((endOffset - startOffset) == nrOfFilesPerThread);

				threads.emplace_back(&Detail::SearchFilesForString, std::vector<std::string>{ allFiles.begin() + startOffset, allFiles.begin() + endOffset },
					stringToSearch, ignoreCase, std::ref(foundStrings), std::ref(mutex));
			}

			threads.emplace_back(&Detail::SearchFilesForString, std::vector<std::string>{ allFiles.begin() + nrOfFilesAdded, allFiles.end() },
				stringToSearch, ignoreCase, std::ref(foundStrings), std::ref(mutex));

			nrOfFilesAdded += allFiles.size() - nrOfFilesAdded;

			RDW_SS_ASSERT(nrOfFilesAdded == allFiles.size());

			if (pStatistics)
			{
				pStatistics->NumberOfFilesSearched = static_cast<int32_t>(nrOfFilesAdded);
			}
		}
		else
		{
			std::mutex mutex{};
			Detail::SearchFilesForString(std::vector<std::string>{ fileToSearch }, stringToSearch, ignoreCase, foundStrings, mutex);
		}
	}

	/*
	command line format:
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
	*/

	inline static void PrintHelp()
	{
		std::cout << "Command Line Format:\n";
		std::cout << "StringSearcher.exe [--recursive N | -r  N] [--ignorecase | -i] [--file <file> | -f <file>] <strings> <mask>\n\n";

		std::cout << "command line options :\n";
		std::cout << "--recursive [N]		search current directory and subdirectories, limited to N levels deep of sub directories. If N is not given or 0, this is unlimited\n";
		std::cout << "--ignorecase			ignore case of characters\n";
		std::cout << "--file				file to look through (required when --recursive or -r are not specified)\n";
		std::cout << "-r					search current directory and subdirectories. Same as --recursive 0\n";
		std::cout << "-i					ignore case of characters\n";
		std::cout << "-f					file to look through (required when --recursive or -r are not specified)\n\n";

		std::cout << "Notes:\n";
		std::cout << "The string to be searched must be between quotation marks if separated by spaces\n";
		std::cout << "The mask is only applied when searching recursively, and accepts a wildcard token: *\n\n";

		std::cout << "Example: StringSearcher.exe -i --file hello_world.txt \"Hello World\"\n";
		std::cout << "Example: StringSearcher.exe --recursive 2 Hello! *.txt\n";
	}
}