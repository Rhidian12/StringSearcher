#pragma once

#include <memory> /* std::unique_ptr */

#ifdef max
#	undef max
#endif

#ifdef min
#	undef min
#endif

namespace RDW_SS::Time
{
	namespace Detail
	{
		template<typename T>
		[[nodiscard]] constexpr bool AreEqual(const T a, const T b, const T epsilon = std::numeric_limits<T>::epsilon())
		{
			static_assert(std::is_fundamental_v<T>, "AreEqual<T>() > T must be a fundamental type");

			return static_cast<T>(abs(a - b)) <= epsilon;
		}
	}

	enum class TimeLength
	{
		NanoSeconds,
		MicroSeconds,
		MilliSeconds,
		Seconds,
		Minutes,
		Hours,
	};

	inline static constexpr double SecToNano{ 1'000'000'000.0 };
	inline static constexpr double SecToMicro{ 1'000'000.0 };
	inline static constexpr double SecToMilli{ 1'000.0 };
	inline static constexpr double SecToMin{ 1.0 / 60.0 };
	inline static constexpr double SecToHours{ 1.0 / 3600.0 };

	inline static constexpr double NanoToSec{ 1.0 / 1'000'000'000.0 };
	inline static constexpr double MicroToSec{ 1.0 / 1'000'000.0 };
	inline static constexpr double MilliToSec{ 1.0 / 1'000.0 };
	inline static constexpr double MinToSec{ 60.0 };
	inline static constexpr double HoursToSec{ 3600.0 };

	/// <summary>
	/// A Timepoint is similar to std::chrono::time_point and should be viewed as it
	/// Timepoints can be initialised with a time value, received from Integrian3D::Time::Timer
	/// They can later then be used to do arithmetics to calculate time
	/// </summary>
	class Timepoint final
	{
	public:
		constexpr Timepoint()
			: m_Time{}
		{}
		constexpr explicit Timepoint(const double time)
			: m_Time{ time }
		{}
		constexpr ~Timepoint() = default;

		constexpr Timepoint(const Timepoint&) noexcept = default;
		constexpr Timepoint(Timepoint&&) noexcept = default;
		constexpr Timepoint& operator=(const Timepoint&) noexcept = default;
		constexpr Timepoint& operator=(Timepoint&&) noexcept = default;

		template<TimeLength T = TimeLength::Seconds>
		[[nodiscard]] constexpr double Count() const
		{
			if constexpr (T == TimeLength::NanoSeconds)
				return m_Time * SecToNano;
			else if constexpr (T == TimeLength::MicroSeconds)
				return m_Time * SecToMicro;
			else if constexpr (T == TimeLength::MilliSeconds)
				return m_Time * SecToMilli;
			else if constexpr (T == TimeLength::Seconds)
				return m_Time;
			else if constexpr (T == TimeLength::Minutes)
				return m_Time * SecToMin;
			else /* Hours */
				return m_Time * SecToHours;
		}

		template<TimeLength T = TimeLength::Seconds, typename Ret>
		[[nodiscard]] constexpr Ret Count() const
		{
			if constexpr (T == TimeLength::NanoSeconds)
				return static_cast<Ret>(m_Time * SecToNano);
			else if constexpr (T == TimeLength::MicroSeconds)
				return static_cast<Ret>(m_Time * SecToMicro);
			else if constexpr (T == TimeLength::MilliSeconds)
				return static_cast<Ret>(m_Time * SecToMilli);
			else if constexpr (T == TimeLength::Seconds)
				return static_cast<Ret>(m_Time);
			else if constexpr (T == TimeLength::Minutes)
				return static_cast<Ret>(m_Time * SecToMin);
			else /* Hours */
				return static_cast<Ret>(m_Time * SecToHours);
		}

	#pragma region Operators
		friend constexpr Timepoint operator-(const Timepoint& a, const Timepoint& b);
		friend constexpr Timepoint operator+(const Timepoint& a, const Timepoint& b);

		friend constexpr Timepoint& operator+=(Timepoint& a, const Timepoint& b);
		friend constexpr Timepoint& operator-=(Timepoint& a, const Timepoint& b);

		friend constexpr bool operator==(const Timepoint& a, const Timepoint& b);
		friend constexpr bool operator!=(const Timepoint& a, const Timepoint& b);

		friend constexpr bool operator<(const Timepoint& a, const Timepoint& b);
		friend constexpr bool operator<=(const Timepoint& a, const Timepoint& b);
		friend constexpr bool operator>(const Timepoint& a, const Timepoint& b);
		friend constexpr bool operator>=(const Timepoint& a, const Timepoint& b);
	#pragma endregion

	private:
		double m_Time; /* Stored in seconds */
	};

#pragma region Timepoint Operators
	constexpr Timepoint operator-(const Timepoint& a, const Timepoint& b)
	{
		return Timepoint{ a.m_Time - b.m_Time };
	}
	constexpr Timepoint operator+(const Timepoint& a, const Timepoint& b)
	{
		return Timepoint{ a.m_Time + b.m_Time };
	}
	constexpr Timepoint& operator+=(Timepoint& a, const Timepoint& b)
	{
		a.m_Time += b.m_Time;

		return a;
	}
	constexpr Timepoint& operator-=(Timepoint& a, const Timepoint& b)
	{
		a.m_Time -= b.m_Time;

		return a;
	}
	constexpr bool operator==(const Timepoint& a, const Timepoint& b)
	{
		return Detail::AreEqual(a.m_Time, b.m_Time);
	}
	constexpr bool operator!=(const Timepoint& a, const Timepoint& b)
	{
		return !(a == b);
	}
	constexpr bool operator<(const Timepoint& a, const Timepoint& b)
	{
		return a.m_Time < b.m_Time;
	}
	constexpr bool operator<=(const Timepoint& a, const Timepoint& b)
	{
		return a.m_Time <= b.m_Time;
	}
	constexpr bool operator>(const Timepoint& a, const Timepoint& b)
	{
		return a.m_Time > b.m_Time;
	}
	constexpr bool operator>=(const Timepoint& a, const Timepoint& b)
	{
		return a.m_Time >= b.m_Time;
	}

#pragma endregion

	class Timer final
	{
	public:
		~Timer() = default;

		static Timer& GetInstance()
		{
			if (!m_pInstance)
				m_pInstance.reset(new Timer{});

			return *m_pInstance.get();
		}

		Timer(const Timer&) noexcept = delete;
		Timer(Timer&&) noexcept = delete;
		Timer& operator=(const Timer&) noexcept = delete;
		Timer& operator=(Timer&&) noexcept = delete;

		void Start()
		{
			m_PreviousTimepoint = Now();
		}

		void Update()
		{
			m_StartTimepoint = Now();

			m_ElapsedSeconds = (m_StartTimepoint - m_PreviousTimepoint).Count();
			m_ElapsedSeconds = std::min(m_ElapsedSeconds, m_MaxElapsedSeconds);

			m_TotalElapsedSeconds += m_ElapsedSeconds;

			m_PreviousTimepoint = m_StartTimepoint;

			m_FPS = static_cast<int>(1.0 / m_ElapsedSeconds);
		}

		[[nodiscard]] static Timepoint Now()
		{
			const int64_t frequency{ _Query_perf_frequency() };
			const int64_t counter{ _Query_perf_counter() };

			// 10 MHz is a very common QPC frequency on modern PCs. Optimizing for
			// this specific frequency can double the performance of this function by
			// avoiding the expensive frequency conversion path.
			constexpr int64_t tenMHz = 10'000'000;

			if (frequency == tenMHz)
			{
				constexpr int64_t multiplier{ static_cast<int64_t>(SecToNano) / tenMHz };
				return Timepoint{ (counter * multiplier) * NanoToSec };
			}
			else
			{
				// Instead of just having "(counter * static_cast<int64_t>(SecToNano)) / frequency",
				// the algorithm below prevents overflow when counter is sufficiently large.
				// It assumes that frequency * static_cast<int64_t>(SecToNano) does not overflow, which is currently true for nano period.
				// It is not realistic for counter to accumulate to large values from zero with this assumption,
				// but the initial value of counter could be large.

				const int64_t whole = (counter / frequency) * static_cast<int64_t>(SecToNano);
				const int64_t part = (counter % frequency) * static_cast<int64_t>(SecToNano) / frequency;
				return Timepoint{ (whole + part) * NanoToSec };
			}
		}

		[[nodiscard]] double GetElapsedSeconds() const { return m_ElapsedSeconds; }
		[[nodiscard]] double GetFixedElapsedSeconds() const { return m_TimePerFrame; }
		[[nodiscard]] double GetTotalElapsedSeconds() const { return m_TotalElapsedSeconds; }
		[[nodiscard]] int GetFPS() const { return m_FPS; }
		[[nodiscard]] double GetTimePerFrame() const { return m_TimePerFrame; }

	#pragma region GetElapsedTime
		template<TimeLength T>
		[[nodiscard]] double GetElapsedTime() const
		{
			if constexpr (T == TimeLength::NanoSeconds)
				return m_ElapsedSeconds * SecToNano;
			else if constexpr (T == TimeLength::MicroSeconds)
				return m_ElapsedSeconds * SecToMicro;
			else if constexpr (T == TimeLength::MilliSeconds)
				return m_ElapsedSeconds * SecToMilli;
			else if constexpr (T == TimeLength::Seconds)
				return m_ElapsedSeconds;
			else if constexpr (T == TimeLength::Minutes)
				return m_ElapsedSeconds * SecToMin;
			else /* Hours */
				return m_ElapsedSeconds * SecToHours;
		}
		template<TimeLength T, typename Ret>
		[[nodiscard]] Ret GetElapsedTime() const
		{
			if constexpr (T == TimeLength::NanoSeconds)
				return static_cast<Ret>(m_ElapsedSeconds * SecToNano);
			else if constexpr (T == TimeLength::MicroSeconds)
				return static_cast<Ret>(m_ElapsedSeconds * SecToMicro);
			else if constexpr (T == TimeLength::MilliSeconds)
				return static_cast<Ret>(m_ElapsedSeconds * SecToMilli);
			else if constexpr (T == TimeLength::Seconds)
				return static_cast<Ret>(m_ElapsedSeconds);
			else if constexpr (T == TimeLength::Minutes)
				return static_cast<Ret>(m_ElapsedSeconds * SecToMin);
			else /* Hours */
				return static_cast<Ret>(m_ElapsedSeconds * SecToHours);
		}
	#pragma endregion

	#pragma region GetFixedElapsedTime
		template<TimeLength T>
		[[nodiscard]] double GetFixedElapsedTime() const
		{
			if constexpr (T == TimeLength::NanoSeconds)
				return m_TimePerFrame * SecToNano;
			else if constexpr (T == TimeLength::MicroSeconds)
				return m_TimePerFrame * SecToMicro;
			else if constexpr (T == TimeLength::MilliSeconds)
				return m_TimePerFrame * SecToMilli;
			else if constexpr (T == TimeLength::Seconds)
				return m_TimePerFrame;
			else if constexpr (T == TimeLength::Minutes)
				return m_TimePerFrame * SecToMin;
			else /* Hours */
				return m_TimePerFrame * SecToHours;
		}
		template<TimeLength T, typename Ret>
		[[nodiscard]] Ret GetFixedElapsedTime() const
		{
			if constexpr (T == TimeLength::NanoSeconds)
				return static_cast<Ret>(m_TimePerFrame * SecToNano);
			else if constexpr (T == TimeLength::MicroSeconds)
				return static_cast<Ret>(m_TimePerFrame * SecToMicro);
			else if constexpr (T == TimeLength::MilliSeconds)
				return static_cast<Ret>(m_TimePerFrame * SecToMilli);
			else if constexpr (T == TimeLength::Seconds)
				return static_cast<Ret>(m_TimePerFrame);
			else if constexpr (T == TimeLength::Minutes)
				return static_cast<Ret>(m_TimePerFrame * SecToMin);
			else /* Hours */
				return static_cast<Ret>(m_TimePerFrame * SecToHours);
		}
	#pragma endregion

	#pragma region GetTotalElapsedTime
		template<TimeLength T>
		[[nodiscard]] double GetTotalElapsedTime() const
		{
			if constexpr (T == TimeLength::NanoSeconds)
				return m_TotalElapsedSeconds * SecToNano;
			else if constexpr (T == TimeLength::MicroSeconds)
				return m_TotalElapsedSeconds * SecToMicro;
			else if constexpr (T == TimeLength::MilliSeconds)
				return m_TotalElapsedSeconds * SecToMilli;
			else if constexpr (T == TimeLength::Seconds)
				return m_TotalElapsedSeconds;
			else if constexpr (T == TimeLength::Minutes)
				return m_TotalElapsedSeconds * SecToMin;
			else /* Hours */
				return m_TotalElapsedSeconds * SecToHours;
		}
		template<TimeLength T, typename Ret>
		[[nodiscard]] Ret GetTotalElapsedTime() const
		{
			if constexpr (T == TimeLength::NanoSeconds)
				return static_cast<Ret>(m_TotalElapsedSeconds * SecToNano);
			else if constexpr (T == TimeLength::MicroSeconds)
				return static_cast<Ret>(m_TotalElapsedSeconds * SecToMicro);
			else if constexpr (T == TimeLength::MilliSeconds)
				return static_cast<Ret>(m_TotalElapsedSeconds * SecToMilli);
			else if constexpr (T == TimeLength::Seconds)
				return static_cast<Ret>(m_TotalElapsedSeconds);
			else if constexpr (T == TimeLength::Minutes)
				return static_cast<Ret>(m_TotalElapsedSeconds * SecToMin);
			else /* Hours */
				return static_cast<Ret>(m_TotalElapsedSeconds * SecToHours);
		}
	#pragma endregion

	private:
		Timer()
			: m_MaxElapsedSeconds{ 0.1 }
			, m_ElapsedSeconds{}
			, m_TotalElapsedSeconds{}
			, m_FPS{}
			, m_FPSCounter{}
			, m_FPSTimer{}
			, m_TimePerFrame{ 1.0 / 144.0 }
			, m_StartTimepoint{}
			, m_PreviousTimepoint{}
		{
			Start();
		}

		inline static std::unique_ptr<Timer> m_pInstance{};

		const double m_MaxElapsedSeconds;
		double m_ElapsedSeconds;
		double m_TotalElapsedSeconds;
		int m_FPS;
		int m_FPSCounter;
		double m_FPSTimer;
		double m_TimePerFrame;

		Timepoint m_StartTimepoint;
		Timepoint m_PreviousTimepoint;
	};
}