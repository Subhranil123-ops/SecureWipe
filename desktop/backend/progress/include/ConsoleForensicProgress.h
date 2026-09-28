#pragma once

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>


class ConsoleForensicProgress
{
public:

    ConsoleForensicProgress() = default;


    void start(
        const std::string &devicePath,
        std::uint64_t totalBytes)
    {
        devicePath_ =
            devicePath;

        totalBytes_ =
            totalBytes;

        startTime_ =
            std::chrono::steady_clock::now();

        lastRenderTime_ =
            startTime_;

        started_ =
            true;

        std::cout
            << "\n"
            << "+==================================================================+\n"
            << "|  FORENSIC ACQUISITION ACTIVE                                    |\n"
            << "+==================================================================+\n"
            << "\n"
            << "  Status       : SCANNING\n"
            << "  Source       : "
            << devicePath_
            << "\n"
            << "  Scanned      : 0 B / "
            << formatBytes(
                   totalBytes_)
            << "\n"
            << "  Progress     : 0%\n"
            << "  Elapsed      : 00:00:00\n"
            << "\n"
            << "  [--------------------------------------------------] 0%\n"
            << "\n"
            << "  The native collector is actively reading the physical source.\n"
            << "  DO NOT disconnect the device during acquisition.\n"
            << std::flush;
    }


    void update(
        std::uint64_t bytesScanned,
        std::uint64_t totalBytes)
    {
        if (!started_)
        {
            start(
                "",
                totalBytes);
        }


        totalBytes_ =
            totalBytes;


        const auto now =
            std::chrono::steady_clock::now();


        const bool completed =
            totalBytes > 0 &&
            bytesScanned >= totalBytes;


        const auto elapsedMilliseconds =
            std::chrono::duration_cast<
                std::chrono::milliseconds>(
                now -
                lastRenderTime_)
                .count();


        /*
         * EvidenceCollector can generate progress
         * callbacks much faster than a human can read.
         *
         * Therefore the terminal is updated at most
         * once every 500 ms, while the final 100%
         * state is always rendered.
         */
        if (
            !completed &&
            elapsedMilliseconds < 500)
        {
            return;
        }


        lastRenderTime_ =
            now;


        const int percentage =
            calculatePercentage(
                bytesScanned,
                totalBytes);


        const auto elapsed =
            std::chrono::duration_cast<
                std::chrono::seconds>(
                now -
                startTime_)
                .count();


        /*
         * Clear the previous dynamic lines using
         * ANSI escape sequences.
         *
         * This keeps the CLI readable instead of
         * printing thousands of progress lines.
         */
        std::cout
            << "\r"
            << "\033[2K"
            << "  Status       : SCANNING\n";


        std::cout
            << "\033[2K"
            << "  Source       : "
            << devicePath_
            << "\n";


        std::cout
            << "\033[2K"
            << "  Scanned      : "
            << formatBytes(
                   bytesScanned)
            << " / "
            << formatBytes(
                   totalBytes)
            << "\n";


        std::cout
            << "\033[2K"
            << "  Progress     : ";


        if (percentage >= 0)
        {
            std::cout
                << std::setw(3)
                << percentage
                << "%";
        }
        else
        {
            std::cout
                << "  ?";
        }


        std::cout
            << "\n";


        std::cout
            << "\033[2K"
            << "  Elapsed      : "
            << formatElapsed(
                   elapsed)
            << "\n";


        std::cout
            << "\033[2K"
            << "\n";


        std::cout
            << "\033[2K"
            << "  ["
            << makeBar(
                   percentage,
                   50)
            << "] ";


        if (percentage >= 0)
        {
            std::cout
                << percentage
                << "%";
        }
        else
        {
            std::cout
                << "?";
        }


        std::cout
            << "\n"
            << std::flush;
    }


    void complete(
        std::uint64_t bytesScanned,
        std::uint64_t totalBytes)
    {
        if (!started_)
        {
            return;
        }


        const auto now =
            std::chrono::steady_clock::now();


        const auto elapsed =
            std::chrono::duration_cast<
                std::chrono::seconds>(
                now -
                startTime_)
                .count();


        /*
         * Clear the currently displayed progress
         * block before printing the permanent
         * completion summary.
         */
        std::cout
            << "\r"
            << "\033[2K";


        std::cout
            << "\n"
            << "+==================================================================+\n"
            << "|  FORENSIC ACQUISITION COMPLETED                                |\n"
            << "+==================================================================+\n"
            << "\n"
            << "  Status       : COMPLETED\n"
            << "  Source       : "
            << devicePath_
            << "\n"
            << "  Scanned      : "
            << formatBytes(
                   bytesScanned)
            << " / "
            << formatBytes(
                   totalBytes)
            << "\n"
            << "  Progress     : 100%\n"
            << "  Elapsed      : "
            << formatElapsed(
                   elapsed)
            << "\n"
            << "\n"
            << "  [==================================================] 100%\n"
            << "\n"
            << "  [OK] Native forensic acquisition completed.\n"
            << std::flush;


        started_ =
            false;
    }


    void fail(
        const std::string &message)
    {
        if (!started_)
        {
            return;
        }


        std::cout
            << "\n"
            << "\n"
            << "+==================================================================+\n"
            << "|  FORENSIC ACQUISITION ERROR                                    |\n"
            << "+==================================================================+\n"
            << "\n"
            << "  [FAIL] "
            << message
            << "\n"
            << std::flush;


        started_ =
            false;
    }


private:

    static int calculatePercentage(
        std::uint64_t bytesScanned,
        std::uint64_t totalBytes)
    {
        if (totalBytes == 0)
        {
            return -1;
        }


        if (bytesScanned >= totalBytes)
        {
            return 100;
        }


        const std::uint64_t percentage =
            (
                bytesScanned *
                100ULL
            ) /
            totalBytes;


        return static_cast<int>(
            std::min<std::uint64_t>(
                percentage,
                100ULL));
    }


    static std::string formatBytes(
        std::uint64_t bytes)
    {
        constexpr double KB =
            1024.0;

        constexpr double MB =
            KB * 1024.0;

        constexpr double GB =
            MB * 1024.0;

        constexpr double TB =
            GB * 1024.0;


        std::ostringstream output;


        output
            << std::fixed
            << std::setprecision(2);


        if (
            bytes >=
            static_cast<std::uint64_t>(
                TB))
        {
            output
                << (
                    static_cast<double>(
                        bytes) /
                    TB)
                << " TB";


            return output.str();
        }


        if (
            bytes >=
            static_cast<std::uint64_t>(
                GB))
        {
            output
                << (
                    static_cast<double>(
                        bytes) /
                    GB)
                << " GB";


            return output.str();
        }


        if (
            bytes >=
            static_cast<std::uint64_t>(
                MB))
        {
            output
                << (
                    static_cast<double>(
                        bytes) /
                    MB)
                << " MB";


            return output.str();
        }


        if (
            bytes >=
            static_cast<std::uint64_t>(
                KB))
        {
            output
                << (
                    static_cast<double>(
                        bytes) /
                    KB)
                << " KB";


            return output.str();
        }


        output
            << bytes
            << " B";


        return output.str();
    }


    static std::string formatElapsed(
        std::int64_t seconds)
    {
        if (seconds < 0)
        {
            seconds =
                0;
        }


        const std::int64_t hours =
            seconds /
            3600;


        const std::int64_t minutes =
            (
                seconds %
                3600
            ) /
            60;


        const std::int64_t remainingSeconds =
            seconds %
            60;


        std::ostringstream output;


        output
            << std::setfill('0')
            << std::setw(2)
            << hours
            << ":"
            << std::setw(2)
            << minutes
            << ":"
            << std::setw(2)
            << remainingSeconds;


        return output.str();
    }


    static std::string makeBar(
        int percentage,
        int width)
    {
        if (percentage < 0)
        {
            return std::string(
                width,
                '-');
        }


        percentage =
            std::clamp(
                percentage,
                0,
                100);


        const int filled =
            (
                percentage *
                width
            ) /
            100;


        return
            std::string(
                filled,
                '=') +
            std::string(
                width -
                filled,
                '-');
    }


private:

    std::string devicePath_;

    std::uint64_t totalBytes_ =
        0;

    std::chrono::steady_clock::time_point
        startTime_{};

    std::chrono::steady_clock::time_point
        lastRenderTime_{};

    bool started_ =
        false;
};