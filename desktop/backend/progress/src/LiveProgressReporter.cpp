#include "LiveProgressReporter.h"

#include <Windows.h>
#include <WinHttp.h>

#include <algorithm>
#include <chrono>
#include <iomanip>
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


/*
 * --------------------------------------------------------------
 * STRING / URL HELPERS
 * --------------------------------------------------------------
 */

std::wstring widenAscii(
    const std::string& value)
{
    std::wstring wide;

    wide.reserve(
        value.size());

    for (const unsigned char character :
         value)
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
        widenAscii(
            url);

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
        path =
            L"/";
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


/*
 * --------------------------------------------------------------
 * HTTP
 * --------------------------------------------------------------
 */

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

    bool secure =
        false;

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
     * Progress reporting must never leave the actual storage
     * operation blocked for an excessive period because the
     * backend is unavailable.
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
        widenAscii(
            method);

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
            widenAscii(
                token) +
            L"\r\n";
    }

    const BOOL sent =
        WinHttpSendRequest(
            request,
            headers.c_str(),
            static_cast<DWORD>(
                -1L),
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

    DWORD statusCode =
        0;

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
        DWORD available =
            0;

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

        DWORD read =
            0;

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


/*
 * --------------------------------------------------------------
 * CONSTRUCTOR
 * --------------------------------------------------------------
 */

LiveProgressReporter::LiveProgressReporter(
    OperationType operationType,
    const std::string& resourceId,
    const std::string& operationId,
    const Config& config)
    : operationType_(
          operationType)
    , resourceId_(
          resourceId)
    , operationId_(
          operationId)
    , config_(
          config)
{
    /*
     * IMPORTANT:
     *
     * operationId_ may legitimately be empty here.
     *
     * The actual native operation can generate its unique
     * operation identifier only after execution begins.
     *
     * Therefore the reporter must NOT be disabled merely
     * because operationId_ is initially empty.
     */
    enabled_ =
        !config_.baseUrl.empty() &&
        !resourceId_.empty() &&
        !config_.token.empty();
}


LiveProgressReporter::~LiveProgressReporter() =
    default;


/*
 * --------------------------------------------------------------
 * OPERATION ID
 * --------------------------------------------------------------
 */

void LiveProgressReporter::setOperationId(
    const std::string& operationId)
{
    operationId_ =
        operationId;
}


/*
 * --------------------------------------------------------------
 * TIME
 * --------------------------------------------------------------
 */

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


/*
 * --------------------------------------------------------------
 * PERCENTAGE
 *
 * This calculation is used ONLY when totalBytes is actually
 * known.
 *
 * It never manufactures totalBytes.
 * --------------------------------------------------------------
 */

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


/*
 * --------------------------------------------------------------
 * THROTTLING
 * --------------------------------------------------------------
 */

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

    if (lastSentAtMs_ == 0)
    {
        return true;
    }

    const std::uint64_t current =
        nowMs();

    if (
        current -
            lastSentAtMs_ >=
        config_.minimumUpdateIntervalMs)
    {
        return true;
    }

    /*
     * Deliberately do not bypass throttling merely because the
     * percentage changed.
     *
     * A fast native operation can generate many callbacks and
     * therefore many percentage changes in a short period.
     */
    (void)processedBytes;
    (void)totalBytes;

    return false;
}


/*
 * --------------------------------------------------------------
 * JSON ESCAPING
 * --------------------------------------------------------------
 */

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


/*
 * --------------------------------------------------------------
 * URL JOIN
 * --------------------------------------------------------------
 */

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


/*
 * --------------------------------------------------------------
 * PATCH
 * --------------------------------------------------------------
 *
 * The backend live-progress routes are PATCH endpoints.
 *
 * The network request is intentionally auxiliary.
 * A backend/network failure must never abort the native
 * sanitization or forensic operation.
 * --------------------------------------------------------------
 */

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
        return false;
    }

    return true;
}


/*
 * --------------------------------------------------------------
 * SANITIZATION PROGRESS
 * --------------------------------------------------------------
 */

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


/*
 * --------------------------------------------------------------
 * SANITIZATION REQUEST
 * --------------------------------------------------------------
 */

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
        << (
               percentage < 0
                   ? 0
                   : percentage)
        << ","
        << "\"progressKnown\":"
        << (
               totalBytes > 0
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
        << "\","
        << "\"status\":\""
        << escapeJson(
               status)
        << "\""
        << "}";

    /*
     * resourceId_ is the SanitizationRequest.requestId.
     *
     * DO NOT use operationId_ here.
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

    if (success)
    {
        lastSentAtMs_ =
            nowMs();

        lastPercentage_ =
            percentage;
    }
    else if (force)
    {
        /*
         * A failed boundary request must NOT stop the physical
         * operation.
         *
         * We deliberately do not mutate lastSentAtMs_ here.
         */
    }

    (void)force;

    return success;
}


/*
 * --------------------------------------------------------------
 * SANITIZATION FINISH
 * --------------------------------------------------------------
 */

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


/*
 * --------------------------------------------------------------
 * SANITIZATION FAILURE
 * --------------------------------------------------------------
 */

bool
LiveProgressReporter::failSanitization(
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
        "FAILED",
        true);
}


/*
 * --------------------------------------------------------------
 * FORENSIC PROGRESS
 * --------------------------------------------------------------
 */

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
        "IN_PROGRESS",
        force);
}


/*
 * --------------------------------------------------------------
 * FORENSIC REQUEST
 * --------------------------------------------------------------
 */

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
        << (
               percentage < 0
                   ? 0
                   : percentage)
        << ","
        << "\"progressKnown\":"
        << (
               totalBytes > 0
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
        << "\","
        << "\"status\":\""
        << escapeJson(
               status)
        << "\""
        << "}";

    /*
     * resourceId_ is the ForensicCase.caseId.
     *
     * DO NOT use operationId_ here.
     */
    const std::string url =
        joinUrl(
            config_.baseUrl,
            "/api/live-progress/forensics/" +
                resourceId_);

    const bool success =
        patchJson(
            url,
            json.str());

    if (success)
    {
        lastSentAtMs_ =
            nowMs();

        lastPercentage_ =
            percentage;
    }
    else if (force)
    {
        /*
         * Reporting failure remains non-fatal.
         */
    }

    (void)force;

    return success;
}


/*
 * --------------------------------------------------------------
 * FORENSIC FINISH
 * --------------------------------------------------------------
 */

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


/*
 * --------------------------------------------------------------
 * FORENSIC FAILURE
 * --------------------------------------------------------------
 */

bool
LiveProgressReporter::failForensic(
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
        "FAILED",
        true);
}


/*
 * --------------------------------------------------------------
 * ENABLE / DISABLE
 * --------------------------------------------------------------
 */

void
LiveProgressReporter::disable()
{
    enabled_ =
        false;
}


bool
LiveProgressReporter::enabled() const
{
    return enabled_;
}