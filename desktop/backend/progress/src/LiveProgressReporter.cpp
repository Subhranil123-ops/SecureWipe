#include "LiveProgressReporter.h"

#include <Windows.h>
#include <WinHttp.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

namespace
{

struct HttpResponse
{
    bool transportOk = false;
    DWORD statusCode = 0;

    std::string body;
    std::string error;
};

std::wstring widenAscii(
    const std::string& value)
{
    std::wstring wide;

    wide.reserve(
        value.size());

    for (const unsigned char character : value)
    {
        wide.push_back(
            static_cast<wchar_t>(
                character));
    }

    return wide;
}

bool crackUrl(
    const std::string& url,
    std::wstring& host,
    std::wstring& path,
    INTERNET_PORT& port,
    bool& secure,
    std::string& error)
{
    const std::wstring wideUrl =
        widenAscii(url);

    URL_COMPONENTSW components{};

    components.dwStructSize =
        sizeof(components);

    wchar_t hostBuffer[512]{};
    wchar_t pathBuffer[32768]{};
    wchar_t extraBuffer[4096]{};

    components.lpszHostName =
        hostBuffer;

    components.dwHostNameLength =
        static_cast<DWORD>(
            sizeof(hostBuffer) /
            sizeof(hostBuffer[0]));

    components.lpszUrlPath =
        pathBuffer;

    components.dwUrlPathLength =
        static_cast<DWORD>(
            sizeof(pathBuffer) /
            sizeof(pathBuffer[0]));

    components.lpszExtraInfo =
        extraBuffer;

    components.dwExtraInfoLength =
        static_cast<DWORD>(
            sizeof(extraBuffer) /
            sizeof(extraBuffer[0]));

    if (!WinHttpCrackUrl(
            wideUrl.c_str(),
            static_cast<DWORD>(
                wideUrl.size()),
            0,
            &components))
    {
        error =
            "WinHttpCrackUrl failed with error " +
            std::to_string(
                GetLastError()) +
            ".";

        return false;
    }

    host.assign(
        hostBuffer,
        components.dwHostNameLength);

    path.assign(
        pathBuffer,
        components.dwUrlPathLength);

    if (path.empty())
    {
        path = L"/";
    }

    if (components.dwExtraInfoLength > 0)
    {
        path.append(
            extraBuffer,
            components.dwExtraInfoLength);
    }

    port =
        components.nPort;

    secure =
        components.nScheme ==
        INTERNET_SCHEME_HTTPS;

    return true;
}

HttpResponse sendHttpRequest(
    const std::string& method,
    const std::string& url,
    const std::string& token,
    const std::string& body)
{
    HttpResponse response;

    std::wstring host;
    std::wstring path;

    INTERNET_PORT port =
        INTERNET_DEFAULT_HTTP_PORT;

    bool secure = false;

    if (!crackUrl(
            url,
            host,
            path,
            port,
            secure,
            response.error))
    {
        return response;
    }

    HINTERNET session =
        WinHttpOpen(
            L"ForenWipe-LiveProgress/1.0",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

    if (!session)
    {
        response.error =
            "WinHttpOpen failed with error " +
            std::to_string(
                GetLastError()) +
            ".";

        return response;
    }

    /*
     * Progress updates must not hang the native operation
     * for a long period because of an unavailable backend.
     */
    WinHttpSetTimeouts(
        session,
        3000,
        3000,
        5000,
        5000);

    HINTERNET connection =
        WinHttpConnect(
            session,
            host.c_str(),
            port,
            0);

    if (!connection)
    {
        response.error =
            "WinHttpConnect failed with error " +
            std::to_string(
                GetLastError()) +
            ".";

        WinHttpCloseHandle(
            session);

        return response;
    }

    const std::wstring wideMethod =
        widenAscii(method);

    HINTERNET request =
        WinHttpOpenRequest(
            connection,
            wideMethod.c_str(),
            path.c_str(),
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            secure
                ? WINHTTP_FLAG_SECURE
                : 0);

    if (!request)
    {
        response.error =
            "WinHttpOpenRequest failed with error " +
            std::to_string(
                GetLastError()) +
            ".";

        WinHttpCloseHandle(
            connection);

        WinHttpCloseHandle(
            session);

        return response;
    }

    std::wstring headers =
        L"Accept: application/json\r\n"
        L"Content-Type: application/json; charset=utf-8\r\n";

    if (!token.empty())
    {
        headers +=
            L"Authorization: Bearer " +
            widenAscii(token) +
            L"\r\n";
    }

    const BOOL sent =
        WinHttpSendRequest(
            request,
            headers.c_str(),
            static_cast<DWORD>(-1L),
            body.empty()
                ? WINHTTP_NO_REQUEST_DATA
                : reinterpret_cast<LPVOID>(
                      const_cast<char*>(
                          body.data())),
            body.empty()
                ? 0
                : static_cast<DWORD>(
                      body.size()),
            body.empty()
                ? 0
                : static_cast<DWORD>(
                      body.size()),
            0);

    if (!sent)
    {
        response.error =
            "WinHttpSendRequest failed with error " +
            std::to_string(
                GetLastError()) +
            ".";

        WinHttpCloseHandle(
            request);

        WinHttpCloseHandle(
            connection);

        WinHttpCloseHandle(
            session);

        return response;
    }

    if (!WinHttpReceiveResponse(
            request,
            nullptr))
    {
        response.error =
            "WinHttpReceiveResponse failed with error " +
            std::to_string(
                GetLastError()) +
            ".";

        WinHttpCloseHandle(
            request);

        WinHttpCloseHandle(
            connection);

        WinHttpCloseHandle(
            session);

        return response;
    }

    DWORD statusCode = 0;

    DWORD statusSize =
        sizeof(statusCode);

    if (!WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE |
                WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &statusCode,
            &statusSize,
            WINHTTP_NO_HEADER_INDEX))
    {
        response.error =
            "Could not read HTTP status code.";
    }
    else
    {
        response.statusCode =
            statusCode;
    }

    while (true)
    {
        DWORD available = 0;

        if (!WinHttpQueryDataAvailable(
                request,
                &available))
        {
            response.error =
                "WinHttpQueryDataAvailable failed with error " +
                std::to_string(
                    GetLastError()) +
                ".";

            break;
        }

        if (available == 0)
        {
            break;
        }

        std::vector<char> buffer(
            static_cast<std::size_t>(
                available));

        DWORD read = 0;

        if (!WinHttpReadData(
                request,
                buffer.data(),
                available,
                &read))
        {
            response.error =
                "WinHttpReadData failed with error " +
                std::to_string(
                    GetLastError()) +
                ".";

            break;
        }

        if (read == 0)
        {
            break;
        }

        response.body.append(
            buffer.data(),
            read);
    }

    response.transportOk =
        response.statusCode > 0;

    WinHttpCloseHandle(
        request);

    WinHttpCloseHandle(
        connection);

    WinHttpCloseHandle(
        session);

    return response;
}

} // namespace

LiveProgressReporter::LiveProgressReporter(
    OperationType operationType,
    const std::string& resourceId,
    const std::string& operationId,
    const Config& config)
    : operationType_(
          operationType),
      operationId_(
          operationId),
      config_(
          config)
{
    enabled_ =
        !config_.baseUrl.empty() &&
        !resourceId_.empty() &&
        !operationId_.empty();
}

LiveProgressReporter::~LiveProgressReporter() =
    default;

void
LiveProgressReporter::setOperationId(
    const std::string& operationId)
{
    operationId_ =
        operationId;
}

std::uint64_t
LiveProgressReporter::nowMs()
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
            std::chrono::steady_clock::now()
                .time_since_epoch())
            .count());
}

int
LiveProgressReporter::calculatePercentage(
    std::uint64_t processedBytes,
    std::uint64_t totalBytes)
{
    if (totalBytes == 0)
    {
        return -1;
    }

    if (processedBytes >= totalBytes)
    {
        return 100;
    }

    /*
     * Use division after scaling carefully so that
     * overflow is avoided for normal storage sizes.
     */
    const long double percentage =
        (
            static_cast<long double>(
                processedBytes) *
            100.0L) /
        static_cast<long double>(
            totalBytes);

    int result =
        static_cast<int>(
            percentage);

    result =
        std::clamp(
            result,
            0,
            100);

    return result;
}

bool
LiveProgressReporter::shouldSend(
    std::uint64_t processedBytes,
    std::uint64_t totalBytes,
    bool force) const
{
    if (force)
    {
        return true;
    }

    if (!enabled_)
    {
        return false;
    }

    const std::uint64_t current =
        nowMs();

    if (lastSentAtMs_ == 0)
    {
        return true;
    }

    if (
        current -
            lastSentAtMs_ >=
        config_.minimumUpdateIntervalMs)
    {
        return true;
    }

    const int percentage =
        calculatePercentage(
            processedBytes,
            totalBytes);

    /*
     * If percentage changed while throttled,
     * keep throttling based on time. This prevents
     * a huge number of HTTP requests.
     */
    (void)percentage;

    return false;
}

std::string
LiveProgressReporter::escapeJson(
    const std::string& value)
{
    std::ostringstream output;

    for (const unsigned char character :
         value)
    {
        switch (character)
        {
        case '"':
            output << "\\\"";
            break;

        case '\\':
            output << "\\\\";
            break;

        case '\b':
            output << "\\b";
            break;

        case '\f':
            output << "\\f";
            break;

        case '\n':
            output << "\\n";
            break;

        case '\r':
            output << "\\r";
            break;

        case '\t':
            output << "\\t";
            break;

        default:
            if (character < 0x20)
            {
                output
                    << "\\u00"
                    << std::hex
                    << std::setw(2)
                    << std::setfill('0')
                    << static_cast<int>(
                           character)
                    << std::dec
                    << std::setfill(' ');
            }
            else
            {
                output
                    << static_cast<char>(
                           character);
            }

            break;
        }
    }

    return output.str();
}

std::string
LiveProgressReporter::joinUrl(
    const std::string& baseUrl,
    const std::string& path)
{
    if (baseUrl.empty())
    {
        return path;
    }

    if (path.empty())
    {
        return baseUrl;
    }

    if (
        baseUrl.back() == '/' &&
        path.front() == '/')
    {
        return
            baseUrl.substr(
                0,
                baseUrl.size() - 1) +
            path;
    }

    if (
        baseUrl.back() != '/' &&
        path.front() != '/')
    {
        return
            baseUrl +
            "/" +
            path;
    }

    return baseUrl + path;
}

bool
LiveProgressReporter::patchJson(
    const std::string& url,
    const std::string& jsonBody)
{
    if (!enabled_)
    {
        return false;
    }

    const HttpResponse response =
        sendHttpRequest(
            "PATCH",
            url,
            config_.token,
            jsonBody);

    if (
        !response.transportOk ||
        response.statusCode < 200 ||
        response.statusCode >= 300)
    {
        /*
         * Progress reporting must be non-fatal.
         *
         * The actual sanitization/forensic operation must
         * continue even if the web server is temporarily
         * unavailable.
         */
        return false;
    }

    return true;
}

bool
LiveProgressReporter::reportSanitizationProgress(
    std::uint64_t processedBytes,
    std::uint64_t totalBytes,
    const std::string& phase,
    const std::string& message)
{
    if (!enabled_)
    {
        return false;
    }

    const int percentage =
        calculatePercentage(
            processedBytes,
            totalBytes);

    const bool boundary =
        percentage == 0 ||
        percentage == 100;

    const bool force =
        config_.alwaysSendBoundaryProgress &&
        boundary;

    if (!shouldSend(
            processedBytes,
            totalBytes,
            force))
    {
        return true;
    }

    return sendSanitizationRequest(
        processedBytes,
        totalBytes,
        phase,
        message,
        "IN_PROGRESS",
        force);
}

bool
LiveProgressReporter::sendSanitizationRequest(
    std::uint64_t processedBytes,
    std::uint64_t totalBytes,
    const std::string& phase,
    const std::string& message,
    const std::string& status,
    bool force)
{
    if (!enabled_)
    {
        return false;
    }

    const int percentage =
        calculatePercentage(
            processedBytes,
            totalBytes);

    std::ostringstream json;

    json
        << "{"
        << "\"operationId\":\""
        << escapeJson(
               operationId_)
        << "\","
        << "\"progress\":"
        << (percentage < 0
                ? 0
                : percentage)
        << ","
        << "\"progressKnown\":"
        << (totalBytes > 0
                ? "true"
                : "false")
        << ","
        << "\"processedBytes\":"
        << processedBytes
        << ","
        << "\"totalBytes\":"
        << totalBytes
        << ","
        << "\"remainingBytes\":"
        << (
               totalBytes > processedBytes
                   ? totalBytes -
                         processedBytes
                   : 0)
        << ","
        << "\"phase\":\""
        << escapeJson(
               phase)
        << "\","
        << "\"message\":\""
        << escapeJson(
               message)
        << "\",\""
        << "\"status\":\""
        << escapeJson(
               status)
        << "\""
        << "}";

    /*
     * This is the backend live-progress endpoint introduced
     * in Part 1.
     */
    const std::string url =
        joinUrl(
            config_.baseUrl,
            "/api/live-progress/sanitization/" +
                resourceId_);

    const bool success =
        patchJson(
            url,
            json.str());

    if (success || force)
    {
        lastSentAtMs_ =
            nowMs();

        lastPercentage_ =
            percentage;
    }

    return success;
}

bool
LiveProgressReporter::reportForensicProgress(
    std::uint64_t bytesScanned,
    std::uint64_t totalBytes,
    std::uint64_t candidatesFound,
    std::uint64_t recoveredArtifacts,
    std::uint64_t validatedArtifacts,
    std::uint64_t rejectedArtifacts,
    std::uint64_t highConfidenceArtifacts,
    std::uint64_t recoveredBytes,
    const std::string& phase,
    const std::string& message)
{
    if (!enabled_)
    {
        return false;
    }

    const int percentage =
        calculatePercentage(
            bytesScanned,
            totalBytes);

    const bool boundary =
        percentage == 0 ||
        percentage == 100;

    const bool force =
        config_.alwaysSendBoundaryProgress &&
        boundary;

    if (!shouldSend(
            bytesScanned,
            totalBytes,
            force))
    {
        return true;
    }

    return sendForensicRequest(
        bytesScanned,
        totalBytes,
        candidatesFound,
        recoveredArtifacts,
        validatedArtifacts,
        rejectedArtifacts,
        highConfidenceArtifacts,
        recoveredBytes,
        phase,
        message,
        "ACQUIRING",
        force);
}

bool
LiveProgressReporter::sendForensicRequest(
    std::uint64_t bytesScanned,
    std::uint64_t totalBytes,
    std::uint64_t candidatesFound,
    std::uint64_t recoveredArtifacts,
    std::uint64_t validatedArtifacts,
    std::uint64_t rejectedArtifacts,
    std::uint64_t highConfidenceArtifacts,
    std::uint64_t recoveredBytes,
    const std::string& phase,
    const std::string& message,
    const std::string& status,
    bool force)
{
    if (!enabled_)
    {
        return false;
    }

    const int percentage =
        calculatePercentage(
            bytesScanned,
            totalBytes);

    std::ostringstream json;

    json
        << "{"
        << "\"operationId\":\""
        << escapeJson(
               operationId_)
        << "\","
        << "\"progress\":"
        << (percentage < 0
                ? 0
                : percentage)
        << ","
        << "\"progressKnown\":"
        << (totalBytes > 0
                ? "true"
                : "false")
        << ","
        << "\"bytesScanned\":"
        << bytesScanned
        << ","
        << "\"totalBytes\":"
        << totalBytes
        << ","
        << "\"remainingBytes\":"
        << (
               totalBytes > bytesScanned
                   ? totalBytes -
                         bytesScanned
                   : 0)
        << ","
        << "\"candidatesFound\":"
        << candidatesFound
        << ","
        << "\"recoveredArtifacts\":"
        << recoveredArtifacts
        << ","
        << "\"validatedArtifacts\":"
        << validatedArtifacts
        << ","
        << "\"rejectedArtifacts\":"
        << rejectedArtifacts
        << ","
        << "\"highConfidenceArtifacts\":"
        << highConfidenceArtifacts
        << ","
        << "\"recoveredBytes\":"
        << recoveredBytes
        << ","
        << "\"phase\":\""
        << escapeJson(
               phase)
        << "\","
        << "\"message\":\""
        << escapeJson(
               message)
        << "\",\""
        << "\"status\":\""
        << escapeJson(
               status)
        << "\""
        << "}";

    const std::string url =
        joinUrl(
            config_.baseUrl,
            "/api/live-progress/forensics/" +
                resourceId_);

    const bool success =
        patchJson(
            url,
            json.str());

    if (success || force)
    {
        lastSentAtMs_ =
            nowMs();

        lastPercentage_ =
            percentage;
    }

    return success;
}

bool
LiveProgressReporter::finishSanitization(
    std::uint64_t processedBytes,
    std::uint64_t totalBytes,
    const std::string& phase,
    const std::string& message)
{
    return sendSanitizationRequest(
        processedBytes,
        totalBytes,
        phase,
        message,
        "COMPLETED",
        true);
}

bool
LiveProgressReporter::finishForensic(
    std::uint64_t bytesScanned,
    std::uint64_t totalBytes,
    std::uint64_t candidatesFound,
    std::uint64_t recoveredArtifacts,
    std::uint64_t validatedArtifacts,
    std::uint64_t rejectedArtifacts,
    std::uint64_t highConfidenceArtifacts,
    std::uint64_t recoveredBytes,
    const std::string& phase,
    const std::string& message)
{
    return sendForensicRequest(
        bytesScanned,
        totalBytes,
        candidatesFound,
        recoveredArtifacts,
        validatedArtifacts,
        rejectedArtifacts,
        highConfidenceArtifacts,
        recoveredBytes,
        phase,
        message,
        "COMPLETED",
        true);
}

void
LiveProgressReporter::disable()
{
    enabled_ = false;
}

bool
LiveProgressReporter::enabled() const
{
    return enabled_;
}