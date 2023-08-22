#include <assert.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stack>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <Windows.h>

#ifndef RDW_STRINGSEARCHER_H // not every compiler supports #pragma once
#define RDW_STRINGSEARCHER_H

namespace RDW_SS
{
	namespace Detail
	{
		inline static void IsStringInFile(const std::string& fileToLookThrough, std::string stringToSearch, const bool ignoreCase,
			std::unordered_map<std::string, std::vector<uint32_t>>& foundStrings)
		{
			std::ifstream stream{};

			stream.open(fileToLookThrough);

			if (!stream.is_open())
			{
				std::cout << "Could not open file: " << fileToLookThrough << "\n";
				return;
			}

			const auto stringToLowercase = [](std::string& str)->void
			{
				std::transform(str.begin(), str.end(), str.begin(), [](char c) { return std::tolower(c); });
			};

			if (ignoreCase) stringToLowercase(stringToSearch);

			uint32_t lineNumber{ 1 }; // Notepad++ starts counting at 1, so let's do the same
			std::string line{};
			while (std::getline(stream, line))
			{
				if (ignoreCase) stringToLowercase(line);

				if (line.find(stringToSearch) != std::string::npos)
				{
					foundStrings[fileToLookThrough].push_back(lineNumber);
				}

				++lineNumber;
			}

			stream.close();
		}

		inline static void IsStringInFile(const std::vector<std::string>& filesToLookThrough, std::string stringToSearch, const bool ignoreCase,
			std::unordered_map<std::string, std::vector<uint32_t>>& foundStrings, std::mutex& mutex)
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

				const auto stringToLowercase = [](std::string& str)->void
				{
					std::transform(str.begin(), str.end(), str.begin(), [](char c) { return std::tolower(c); });
				};

				if (ignoreCase) stringToLowercase(stringToSearch);

				uint32_t lineNumber{ 1 }; // Notepad++ starts counting at 1, so let's do the same
				std::string line{};
				while (std::getline(stream, line))
				{
					if (ignoreCase) stringToLowercase(line);

					if (line.find(stringToSearch) != std::string::npos)
					{
						const std::lock_guard<std::mutex> lock{ mutex };
						foundStrings[filename].push_back(lineNumber);
					}

					++lineNumber;
				}

				stream.close();
			}
		}

		inline static bool IsFilenameValid(char* pFilename, const std::string& filenameFilter)
		{
			if (filenameFilter == "*") return true;

			bool isFilenameValid{ true };
			for (size_t i{}; i < MAX_PATH; ++i)
			{
				if (pFilename[i] == '\0' || pFilename[i] == '.') break;

				if (i >= filenameFilter.size())
				{
					isFilenameValid = false;
					break;
				}

				if (pFilename[i] != filenameFilter[i])
				{
					isFilenameValid = false;
					break;
				}
			}

			return isFilenameValid;
		}

		inline static bool IsExtensionValid(char* pFilename, const std::string& extensionFilter)
		{
			if (extensionFilter == "*") return true;

			{
				bool hasExtension{};
				for (size_t i{}; i < MAX_PATH; ++i)
				{
					if (*pFilename != '.') ++pFilename;
					else
					{
						hasExtension = true;
						break;
					}
				}

				if (!hasExtension) return false;
			}

			bool isExtensionValid{ true };
			for (size_t i{}; i < MAX_PATH; ++i)
			{
				if (pFilename[i] == '\0') break;

				if (i >= extensionFilter.size())
				{
					isExtensionValid = false;
					break;
				}

				if (pFilename[i] != extensionFilter[i])
				{
					isExtensionValid = false;
					break;
				}
			}

			return isExtensionValid;
		}

		inline static std::vector<std::string> GetAllFilesInDirectory(const std::string& rootDir, const std::string& filterFile)
		{
			std::vector<std::string> files{};
			files.reserve(80);

			WIN32_FIND_DATAA findFileData;
			HANDLE fileHandle = INVALID_HANDLE_VALUE;

			std::stack<std::string> fileStack{};
			fileStack.push(rootDir);

			const std::string filterFilename{ filterFile.substr(0, filterFile.find(".")) };
			const std::string filterExtension{ filterFile.substr(filterFile.find(".")) };
			const bool isWildcardName{ filterFilename == "*" };
			const bool isWildcardExtension{ filterExtension == "*" };

			while (!fileStack.empty())
			{
				const std::string child{ fileStack.top() };
				fileStack.pop();

				fileHandle = FindFirstFileA((child + "\\*").c_str(), &findFileData);
				if (fileHandle == INVALID_HANDLE_VALUE)
				{
					std::cout << "FindFirstFile failed on file" << child << "\n";
					return std::vector<std::string>{};
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
						fileStack.push(child + "\\" + findFileData.cFileName);
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

	inline static void IsStringInFile(const std::string& currentDir, const std::string& fileToLookThrough, const std::string& stringToSearch,
		const bool ignoreCase, const bool recursivelySearch, std::unordered_map<std::string, std::vector<uint32_t>>& foundStrings,
		StringSearchStatistics* pStatistics)
	{
		if (recursivelySearch)
		{
			const size_t nrOfThreads{ std::thread::hardware_concurrency() };
			const std::vector<std::string> allFiles{ Detail::GetAllFilesInDirectory(currentDir, fileToLookThrough) };
			const size_t nrOfFilesPerThread{ allFiles.size() / nrOfThreads };

			if (nrOfFilesPerThread == 0) return;

			const bool evenNumberOfFiles{ allFiles.size() % nrOfThreads == 0 };
			std::vector<std::thread> threads{};
			threads.reserve(nrOfThreads);
			std::mutex mutex{};

			using FuncType = void(const std::vector<std::string>&, std::string, const bool, std::unordered_map<std::string, std::vector<uint32_t>>&, std::mutex&);

			size_t nrOfFilesAdded{};
			for (size_t i{}; i < nrOfThreads - 1; ++i)
			{
				const size_t startOffset{ nrOfFilesPerThread * i };
				const size_t endOffset{ nrOfFilesPerThread * (i + 1) };

				nrOfFilesAdded += endOffset - startOffset;
#ifdef _DEBUG
				{ // destroys debug performance but we dont care lol
					auto testFileSizes = std::vector<std::string>{ allFiles.begin() + startOffset, allFiles.begin() + endOffset };
					assert(testFileSizes.size() == nrOfFilesPerThread);
				}
#endif

				threads.emplace_back(static_cast<FuncType*>(&Detail::IsStringInFile), std::vector<std::string>{ allFiles.begin() + startOffset, allFiles.begin() + endOffset }, stringToSearch, ignoreCase, std::ref(foundStrings), std::ref(mutex));
			}

			threads.emplace_back(static_cast<FuncType*>(&Detail::IsStringInFile), std::vector<std::string>{ allFiles.begin() + nrOfFilesAdded, allFiles.end() }, stringToSearch, ignoreCase, std::ref(foundStrings), std::ref(mutex));
			nrOfFilesAdded += std::distance(allFiles.begin() + nrOfFilesAdded, allFiles.end());

#ifdef _DEBUG
			assert(nrOfFilesAdded == allFiles.size());
#endif

			for (size_t i{}; i < nrOfThreads; ++i) threads[i].join();

			if (pStatistics)
			{
				pStatistics->NumberOfFilesSearched = static_cast<int32_t>(nrOfFilesAdded);
			}
		}
		else
		{
			Detail::IsStringInFile(fileToLookThrough, stringToSearch, ignoreCase, foundStrings);
		}
	}

	inline static void PrintHelp()
	{
		std::cout << "command line options :\n";
		std::cout << "/s	search current directory and subdirectories\n";
		std::cout << "/i	ignore case of characters\n";
		std::cout << "/c	string to search for (required)\n";
		std::cout << "/f	files to look through (required if /s is not specified)\n";
		std::cout << "Example: StringSearcher.exe /s /c Hello World! /f *.txt\n";
	}
}

#endif // RDW_STRINGSEARCHER_H