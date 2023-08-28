#include "StringSearcher.h"
#include "Timer.h"

#include <iostream>

namespace
{
	static int GetAverage(const std::vector<int>& v)
	{
		int average{};

		for (const int value : v)
		{
			average += value;
		}

		return average / static_cast<int>(v.size());
	}

	static int GetMedian(const std::vector<int>& v)
	{
		const int size{ static_cast<int>(v.size()) };
		const int medianIndex{ (size + 1) / 2 };

		if (size % 2 == 0)
		{
			return (v[medianIndex] + v[medianIndex + 1]) / 2;
		}
		else
		{
			return v[medianIndex];
		}
	}
}

int RunPerformanceTests(const int nrOfIterations, const std::string& directoryToSearch, const std::string& stringToSearch, const std::string& mask)
{
	using namespace RDW_SS;
	using namespace Time;

	std::vector<int> findStrTimes{}, stringSearcherTimes{};

	for (int i{}; i < nrOfIterations; ++i)
	{
		{
			const std::string findstrCommand{ "cd " + directoryToSearch + " && findstr /s " + stringToSearch + " " + mask + " > nul" };

			const Timepoint t1{ Timer::GetInstance().Now() };
			system(findstrCommand.c_str());
			const Timepoint t2{ Timer::GetInstance().Now() };

			findStrTimes.push_back((t2 - t1).Count<TimeLength::MilliSeconds, int>());
		}

		{
			const std::string stringSearcherCommand{ "cd " + directoryToSearch + " && StringSearcher.exe -r " + stringToSearch + " " + mask + " > nul" };

			const Timepoint t1{ Timer::GetInstance().Now() };
			std::unordered_map<std::string, std::vector<uint32_t>> foundStrings{};
			system(stringSearcherCommand.c_str());
			const Timepoint t2{ Timer::GetInstance().Now() };

			stringSearcherTimes.push_back((t2 - t1).Count<TimeLength::MilliSeconds, int>());
		}
	}

	std::sort(findStrTimes.begin(), findStrTimes.end());
	std::sort(stringSearcherTimes.begin(), stringSearcherTimes.end());

	int nrOfElementsToRemove{ nrOfIterations / 10 / 2 };
	if (nrOfElementsToRemove == 0)
	{
		nrOfElementsToRemove = 1;
	}

	findStrTimes.erase(findStrTimes.begin(), findStrTimes.begin() + nrOfElementsToRemove);
	findStrTimes.erase(findStrTimes.begin() + findStrTimes.size() - nrOfElementsToRemove, findStrTimes.end());
	stringSearcherTimes.erase(stringSearcherTimes.begin(), stringSearcherTimes.begin() + nrOfElementsToRemove);
	stringSearcherTimes.erase(stringSearcherTimes.begin() + stringSearcherTimes.size() - nrOfElementsToRemove, stringSearcherTimes.end());

	std::cout << "\n\nNr Of Iterations: " << nrOfIterations << "\n";

	std::cout << "FindStr Times:\n\n";

	std::cout << "Average (ms): " << GetAverage(findStrTimes) << "\n";
	std::cout << "Median (ms): " << GetMedian(findStrTimes) << "\n";

	std::cout << "\n========================\n";

	std::cout << "StringSearcher Times:\n\n";

	std::cout << "Average (ms): " << GetAverage(stringSearcherTimes) << "\n";
	std::cout << "Median (ms): " << GetMedian(stringSearcherTimes) << "\n";

	return 0;
}