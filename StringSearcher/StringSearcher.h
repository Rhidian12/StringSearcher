#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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

		inline static void IsStringInFile() {}

		inline static std::vector<std::string> GetAllFilesInDirectory(const std::string& rootDir, const std::string& fileToLookThrough)
		{
			const std::filesystem::path directory{ rootDir };
			std::vector<std::string> fileNames{};

			// reserve some memory for all of the files/directories in the directory
			fileNames.reserve(std::distance(std::filesystem::directory_iterator(directory), std::filesystem::directory_iterator()));

			for (const std::filesystem::path& path : std::filesystem::directory_iterator(directory))
			{
				if (std::filesystem::is_directory(path))
				{
					const std::vector namesInDir{ GetAllFilesInDirectory(path.string(), fileToLookThrough) };
					fileNames.insert(fileNames.end(), namesInDir.begin(), namesInDir.end());
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

					fileNames.emplace_back(path.string());
				}
			}

			return fileNames;
		}
	}

	inline static void IsStringInFile(const std::string& currentDir, const std::string& fileToLookThrough, const std::string& stringToSearch,
		const bool ignoreCase, const bool recursivelySearch, std::unordered_map<std::string, std::vector<uint32_t>>& foundStrings)
	{
		if (recursivelySearch)
		{
			// use half of the threads
			const size_t nrOfThreads{ std::thread::hardware_concurrency() };
			const std::vector<std::string> allFiles{ Detail::GetAllFilesInDirectory(currentDir, fileToLookThrough) };
			const size_t nrOfFilesPerThread{ allFiles.size() / nrOfThreads };
			std::vector<std::thread> threads{};
			threads.reserve(nrOfThreads);
			std::mutex mutex{};

			if (nrOfFilesPerThread == 0) return;

			using FuncType = void(const std::vector<std::string>&, std::string, const bool, std::unordered_map<std::string, std::vector<uint32_t>>&, std::mutex&);

			for (size_t i{}; i < nrOfThreads; ++i)
			{
				threads.emplace_back(static_cast<FuncType*>(&Detail::IsStringInFile), std::vector<std::string>{"a"}, stringToSearch, ignoreCase, std::ref(foundStrings), std::ref(mutex));
			}

			for (size_t i{}; i < nrOfThreads; ++i) threads[i].join();
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
	}
}

#endif // RDW_STRINGSEARCHER_H