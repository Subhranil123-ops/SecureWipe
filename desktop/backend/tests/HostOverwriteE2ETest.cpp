#include <Windows.h>
#include <bcrypt.h>
#include <conio.h>
#include <winhttp.h>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <exception>
#include <cstdlib>
#include <filesystem>
#include <iterator>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <utility>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")

#ifndef SECUREWIPE_API_BASE_URL
#define SECUREWIPE_API_BASE_URL "https://securewipe-kuo0.onrender.com"
#endif

#include "StorageDevice.h"
#include "WindowsStorageDiscovery.h"
#include "SanitizationCapability.h"
#include "SanitizationEngine.h"
#include "SanitizationMethod.h"
#include "SanitizationPipeline.h"
#include "SanitizationResult.h"
#include "SanitizationCertificate.h"
#include "SanitizationEvent.h"
#include "SafetyEngine.h"
#include "SafetyResult.h"
#include "AuditChainVerifier.h"
#include "CertificateVerifier.h"

using namespace SecureWipe;

namespace
{

// ============================================================
// CONSOLE STYLE
// ============================================================

namespace Console
{
    constexpr const char* RESET =
        "\x1b[0m";

    constexpr const char* CYAN =
        "\x1b[96m";

    constexpr const char* GREEN =
        "\x1b[92m";

    constexpr const char* YELLOW =
        "\x1b[93m";

    constexpr const char* RED =
        "\x1b[91m";

    constexpr const char* DIM =
        "\x1b[90m";

    constexpr const char* BOLD_CYAN =
        "\x1b[1;96m";

    constexpr const char* BOLD_GREEN =
        "\x1b[1;92m";

    constexpr const char* BOLD_RED =
        "\x1b[1;91m";

    constexpr const char* BOLD_YELLOW =
        "\x1b[1;93m";

    void enable()
    {
        HANDLE output =
            GetStdHandle(
                STD_OUTPUT_HANDLE);

        if (
            output == INVALID_HANDLE_VALUE ||
            output == nullptr)
        {
            return;
        }

        DWORD mode = 0;

        if (
            !GetConsoleMode(
                output,
                &mode))
        {
            return;
        }

        mode |=
            ENABLE_VIRTUAL_TERMINAL_PROCESSING;

        SetConsoleMode(
            output,
            mode);
    }

    void title(
        const std::string& text)
    {
        std::cout
            << "\n"
            << BOLD_CYAN
            << "+==================================================================+\n"
            << "| "
            << text;

        if (
            text.size() < 64)
        {
            std::cout
                << std::string(
                       64 - text.size(),
                       ' ');
        }

        std::cout
            << "|\n"
            << "+==================================================================+"
            << RESET
            << "\n";
    }

    void pass(
        const std::string& text)
    {
        std::cout
            << BOLD_GREEN
            << "  [PASS] "
            << RESET
            << text
            << '\n';
    }

    void fail(
        const std::string& text)
    {
        std::cout
            << BOLD_RED
            << "  [FAIL] "
            << RESET
            << text
            << '\n';
    }

    void stop(
        const std::string& text)
    {
        std::cout
            << BOLD_YELLOW
            << "  [STOP] "
            << RESET
            << text
            << '\n';
    }

    void warning(
        const std::string& text)
    {
        std::cout
            << BOLD_YELLOW
            << "  [WARN] "
            << RESET
            << text
            << '\n';
    }

    void info(
        const std::string& text)
    {
        std::cout
            << CYAN
            << text
            << RESET;
    }
}

// ============================================================
// INPUT
// ============================================================

std::string readLine()
{
    std::string value;

    std::getline(
        std::cin,
        value);

    return value;
}

// ============================================================
// SEPARATOR
// ============================================================

void separator()
{
    std::cout
        << "\n"
        << Console::DIM
        << "------------------------------------------------------------------"
        << Console::RESET
        << "\n";
}

// ============================================================
// STRING HELPERS
// ============================================================

std::string trim(
    std::string value)
{
    while (
        !value.empty() &&
        std::isspace(
            static_cast<unsigned char>(
                value.front())))
    {
        value.erase(
            value.begin());
    }

    while (
        !value.empty() &&
        std::isspace(
            static_cast<unsigned char>(
                value.back())))
    {
        value.pop_back();
    }

    return value;
}

std::string toUpper(
    std::string value)
{
    for (
        char& character :
        value)
    {
        character =
            static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(
                        character)));
    }

    return value;
}

// ============================================================
// ENUM DISPLAY HELPERS
// ============================================================

std::string methodToString(
    SanitizationMethod method)
{
    switch (
        method)
    {
    case SanitizationMethod::NvmeSanitize:
        return "NVMe Sanitize";

    case SanitizationMethod::AtaSanitize:
        return "ATA Sanitize";

    case SanitizationMethod::HostOverwrite:
        return "Host Overwrite";

    case SanitizationMethod::Unsupported:
        return "Unsupported";
    }

    return "Unknown";
}

std::string sanitizationStatusToString(
    SanitizationStatus status)
{
    switch (
        status)
    {
    case SanitizationStatus::NOT_STARTED:
        return "NOT_STARTED";

    case SanitizationStatus::IN_PROGRESS:
        return "IN_PROGRESS";

    case SanitizationStatus::COMPLETED:
        return "COMPLETED";

    case SanitizationStatus::FAILED:
        return "FAILED";

    case SanitizationStatus::ABORTED:
        return "ABORTED";
    }

    return "UNKNOWN";
}

std::string verificationStatusToString(
    VerificationStatus status)
{
    switch (
        status)
    {
    case VerificationStatus::NOT_PERFORMED:
        return "NOT_PERFORMED";

    case VerificationStatus::IN_PROGRESS:
        return "IN_PROGRESS";

    case VerificationStatus::PASSED:
        return "PASSED";

    case VerificationStatus::FAILED:
        return "FAILED";
    }

    return "UNKNOWN";
}

// ============================================================
// WINDOWS ACTOR
// ============================================================

bool getCurrentWindowsUser(
    std::string& user)
{
    char buffer[256]{};

    DWORD size =
        static_cast<DWORD>(
            sizeof(buffer));

    if (
        !GetUserNameA(
            buffer,
            &size))
    {
        return false;
    }

    if (
        size > 0)
    {
        --size;
    }

    user.assign(
        buffer,
        size);

    return !user.empty();
}

// ============================================================
// WEB API / JSON HELPERS
// ============================================================

struct HttpResponse
{
    bool transportOk = false;
    DWORD statusCode = 0;
    std::string body;
    std::string error;
};

struct AuthSession
{
    std::string baseUrl;
    std::string token;
    std::string userId;
    std::string email;
    std::string name;
    std::string role;

    // Kept only in process memory so a new short-lived JWT can be
    // obtained automatically if a long-running sanitization operation
    // outlives the 15-minute access token. Never written to disk/logs.
    std::string password;
};

struct WebSanitizationRequestBinding
{
    std::string requestId;
    std::string status;
    std::string deviceType;
    std::string capacity;
    std::string serialNumber;
    std::string assetIdentifier;

    std::string workstationMongoId;
    std::string workstationId;
    std::string workstationName;
    std::string workstationStatus;

    std::string assignedEmployeeId;
    std::string assignedEmployeeEmail;
};

std::string jsonEscape(
    const std::string& value)
{
    std::ostringstream output;

    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '\\':
            output << "\\\\";
            break;

        case '"':
            output << "\\\"";
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

        case '\b':
            output << "\\b";
            break;

        case '\f':
            output << "\\f";
            break;

        default:
            if (character < 0x20)
            {
                output
                    << "\\u00"
                    << std::hex
                    << std::uppercase
                    << static_cast<int>(character)
                    << std::nouppercase
                    << std::dec;
            }
            else
            {
                output << static_cast<char>(character);
            }
            break;
        }
    }

    return output.str();
}

std::wstring widenAscii(
    const std::string& value)
{
    std::wstring wide;
    wide.reserve(value.size());

    for (const unsigned char character : value)
    {
        wide.push_back(
            static_cast<wchar_t>(character));
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

    components.lpszHostName = hostBuffer;
    components.dwHostNameLength =
        static_cast<DWORD>(sizeof(hostBuffer) / sizeof(hostBuffer[0]));

    components.lpszUrlPath = pathBuffer;
    components.dwUrlPathLength =
        static_cast<DWORD>(sizeof(pathBuffer) / sizeof(pathBuffer[0]));

    components.lpszExtraInfo = extraBuffer;
    components.dwExtraInfoLength =
        static_cast<DWORD>(sizeof(extraBuffer) / sizeof(extraBuffer[0]));

    if (!WinHttpCrackUrl(
            wideUrl.c_str(),
            static_cast<DWORD>(wideUrl.size()),
            0,
            &components))
    {
        error =
            "WinHttpCrackUrl failed with error " +
            std::to_string(GetLastError()) +
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

    port = components.nPort;
    secure =
        components.nScheme == INTERNET_SCHEME_HTTPS;

    return true;
}

HttpResponse sendHttpRequest(
    const std::string& method,
    const std::string& url,
    const std::string& token,
    const std::string& body = {})
{
    HttpResponse response;

    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = INTERNET_DEFAULT_HTTP_PORT;
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
            L"ForenWipe-HostOverwrite-E2E/2.0",
            WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);

    if (!session)
    {
        response.error =
            "WinHttpOpen failed with error " +
            std::to_string(GetLastError()) +
            ".";
        return response;
    }

    WinHttpSetTimeouts(
        session,
        30000,
        30000,
        120000,
        120000);

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
            std::to_string(GetLastError()) +
            ".";

        WinHttpCloseHandle(session);
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
            std::to_string(GetLastError()) +
            ".";

        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
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
            std::to_string(GetLastError()) +
            ".";

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return response;
    }

    if (!WinHttpReceiveResponse(
            request,
            nullptr))
    {
        response.error =
            "WinHttpReceiveResponse failed with error " +
            std::to_string(GetLastError()) +
            ".";

        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return response;
    }

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);

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
                std::to_string(GetLastError()) +
                ".";
            break;
        }

        if (available == 0)
        {
            break;
        }

        std::vector<char> buffer(
            static_cast<std::size_t>(available));

        DWORD read = 0;

        if (!WinHttpReadData(
                request,
                buffer.data(),
                available,
                &read))
        {
            response.error =
                "WinHttpReadData failed with error " +
                std::to_string(GetLastError()) +
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

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    return response;
}

bool expectHttpSuccess(
    const HttpResponse& response,
    const std::string& operation)
{
    if (
        !response.transportOk ||
        response.statusCode < 200 ||
        response.statusCode >= 300)
    {
        Console::fail(
            operation +
            " failed (HTTP " +
            std::to_string(response.statusCode) +
            ")" +
            (response.error.empty()
                ? ""
                : ": " + response.error));

        if (!response.body.empty())
        {
            std::cout
                << Console::RED
                << "\n  Server response:\n"
                << response.body
                << Console::RESET
                << "\n";
        }

        return false;
    }

    return true;
}

bool extractJsonStringField(
    const std::string& json,
    const std::string& key,
    std::string& value,
    std::size_t searchFrom = 0);

bool extractJsonObjectField(
    const std::string& json,
    const std::string& key,
    std::string& objectJson,
    std::size_t searchFrom = 0);


bool refreshWebSession(
    const std::string& baseUrl,
    AuthSession& session)
{
    if (
        session.email.empty() ||
        session.password.empty())
    {
        Console::fail(
            "Automatic JWT refresh is unavailable because the in-memory workstation credentials are missing.");
        return false;
    }

    std::ostringstream body;

    body
        << "{"
        << "\"email\":\""
        << jsonEscape(session.email)
        << "\","
        << "\"password\":\""
        << jsonEscape(session.password)
        << "\""
        << "}";

    const HttpResponse response =
        sendHttpRequest(
            "POST",
            baseUrl + "/api/auth/login",
            {},
            body.str());

    if (
        !response.transportOk ||
        response.statusCode < 200 ||
        response.statusCode >= 300)
    {
        Console::fail(
            "Automatic JWT refresh login failed (HTTP " +
            std::to_string(response.statusCode) +
            ").");

        if (!response.body.empty())
        {
            std::cout
                << Console::RED
                << "  Refresh response: "
                << response.body
                << Console::RESET
                << "\n";
        }

        return false;
    }

    std::string newToken;

    if (!extractJsonStringField(
            response.body,
            "token",
            newToken) ||
        newToken.empty())
    {
        Console::fail(
            "Automatic JWT refresh response did not contain a usable token.");
        return false;
    }

    session.token = newToken;

    std::string userJson;

    if (extractJsonObjectField(
            response.body,
            "user",
            userJson))
    {
        std::string refreshedRole;
        std::string refreshedEmail;
        std::string refreshedName;
        std::string refreshedUserId;

        extractJsonStringField(
            userJson,
            "id",
            refreshedUserId);

        if (refreshedUserId.empty())
        {
            extractJsonStringField(
                userJson,
                "_id",
                refreshedUserId);
        }

        extractJsonStringField(
            userJson,
            "email",
            refreshedEmail);

        extractJsonStringField(
            userJson,
            "name",
            refreshedName);

        extractJsonStringField(
            userJson,
            "role",
            refreshedRole);

        if (!refreshedUserId.empty())
        {
            session.userId = refreshedUserId;
        }

        if (!refreshedEmail.empty())
        {
            session.email = refreshedEmail;
        }

        if (!refreshedName.empty())
        {
            session.name = refreshedName;
        }

        if (!refreshedRole.empty())
        {
            session.role = refreshedRole;
        }
    }

    if (session.role != "WORKSTATION_EMPLOYEE")
    {
        Console::fail(
            "Automatic JWT refresh returned an account that is no longer a WORKSTATION_EMPLOYEE.");
        return false;
    }

    Console::pass(
        "Fresh 15-minute JWT obtained automatically after token expiry.");

    return true;
}

HttpResponse sendAuthenticatedHttpRequest(
    const std::string& method,
    const std::string& url,
    AuthSession& session,
    const std::string& body = {})
{
    HttpResponse response =
        sendHttpRequest(
            method,
            url,
            session.token,
            body);

    if (response.statusCode != 401)
    {
        return response;
    }

    Console::warning(
        "The access token expired during the long-running operation. Refreshing authentication and retrying the same request once.");

    if (!refreshWebSession(
            session.baseUrl,
            session))
    {
        return response;
    }

    return sendHttpRequest(
        method,
        url,
        session.token,
        body);
}

std::string apiBaseUrl()
{
    const char* overrideValue =
        std::getenv(
            "SECUREWIPE_API_BASE_URL_OVERRIDE");

    if (
        overrideValue &&
        *overrideValue)
    {
        return trim(overrideValue);
    }

    std::string base =
        SECUREWIPE_API_BASE_URL;

    while (
        !base.empty() &&
        base.back() == '/')
    {
        base.pop_back();
    }

    return base;
}

std::string readEnvironmentValue(
    const char* name)
{
    const char* value =
        std::getenv(name);

    if (
        value &&
        *value)
    {
        return trim(value);
    }

    return {};
}

std::string readSecret(
    const std::string& label)
{
    std::string value;

    std::cout
        << "  "
        << label
        << ": "
        << std::flush;

    while (true)
    {
        const int character =
            _getch();

        if (
            character == '\r' ||
            character == '\n')
        {
            break;
        }

        if (character == 8)
        {
            if (!value.empty())
            {
                value.pop_back();
            }

            continue;
        }

        if (character == 3)
        {
            std::cout << '\n';
            return {};
        }

        if (
            character >= 32 &&
            character <= 126)
        {
            value.push_back(
                static_cast<char>(character));
        }
    }

    std::cout << '\n';
    return trim(value);
}

std::size_t findJsonStringEnd(
    const std::string& json,
    std::size_t quotePosition)
{
    bool escaped = false;

    for (
        std::size_t index = quotePosition + 1;
        index < json.size();
        ++index)
    {
        const char character =
            json[index];

        if (escaped)
        {
            escaped = false;
            continue;
        }

        if (character == '\\')
        {
            escaped = true;
            continue;
        }

        if (character == '"')
        {
            return index;
        }
    }

    return std::string::npos;
}

std::size_t findMatchingJsonDelimiter(
    const std::string& json,
    std::size_t openingPosition,
    char opening,
    char closing)
{
    if (
        openingPosition >= json.size() ||
        json[openingPosition] != opening)
    {
        return std::string::npos;
    }

    std::size_t depth = 0;
    bool inString = false;
    bool escaped = false;

    for (
        std::size_t index = openingPosition;
        index < json.size();
        ++index)
    {
        const char character =
            json[index];

        if (inString)
        {
            if (escaped)
            {
                escaped = false;
                continue;
            }

            if (character == '\\')
            {
                escaped = true;
                continue;
            }

            if (character == '"')
            {
                inString = false;
            }

            continue;
        }

        if (character == '"')
        {
            inString = true;
            continue;
        }

        if (character == opening)
        {
            ++depth;
            continue;
        }

        if (character == closing)
        {
            if (depth == 0)
            {
                return std::string::npos;
            }

            --depth;

            if (depth == 0)
            {
                return index;
            }
        }
    }

    return std::string::npos;
}

std::string decodeJsonString(
    const std::string& value)
{
    std::string output;
    output.reserve(value.size());

    bool escaped = false;

    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const char character = value[index];

        if (!escaped)
        {
            if (character == '\\')
            {
                escaped = true;
            }
            else
            {
                output.push_back(character);
            }

            continue;
        }

        switch (character)
        {
        case '"':
            output.push_back('"');
            break;

        case '\\':
            output.push_back('\\');
            break;

        case '/':
            output.push_back('/');
            break;

        case 'b':
            output.push_back('\b');
            break;

        case 'f':
            output.push_back('\f');
            break;

        case 'n':
            output.push_back('\n');
            break;

        case 'r':
            output.push_back('\r');
            break;

        case 't':
            output.push_back('\t');
            break;

        default:
            output.push_back(character);
            break;
        }

        escaped = false;
    }

    return output;
}

std::size_t findJsonKey(
    const std::string& json,
    const std::string& key,
    std::size_t searchFrom = 0)
{
    return json.find(
        "\"" + key + "\"",
        searchFrom);
}

std::size_t skipJsonWhitespace(
    const std::string& json,
    std::size_t position)
{
    while (
        position < json.size() &&
        (
            json[position] == ' ' ||
            json[position] == '\t' ||
            json[position] == '\r' ||
            json[position] == '\n'))
    {
        ++position;
    }

    return position;
}

bool extractJsonStringField(
    const std::string& json,
    const std::string& key,
    std::string& value,
    std::size_t searchFrom)
{
    const std::size_t keyPosition =
        findJsonKey(
            json,
            key,
            searchFrom);

    if (keyPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t colonPosition =
        json.find(
            ':',
            keyPosition + key.size() + 2);

    if (colonPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t valuePosition =
        skipJsonWhitespace(
            json,
            colonPosition + 1);

    if (
        valuePosition >= json.size() ||
        json[valuePosition] != '"')
    {
        return false;
    }

    const std::size_t endPosition =
        findJsonStringEnd(
            json,
            valuePosition);

    if (endPosition == std::string::npos)
    {
        return false;
    }

    value =
        decodeJsonString(
            json.substr(
                valuePosition + 1,
                endPosition - valuePosition - 1));

    return true;
}


// Extract a string-valued field only when the key belongs to the
// outermost JSON object represented by this string. This is needed for
// fields such as `status`, because the request contains nested objects
// (for example assignedWorkstation.status) that can otherwise be picked
// up by the generic recursive string-field search.
bool extractTopLevelJsonStringField(
    const std::string& json,
    const std::string& key,
    std::string& value)
{
    std::size_t position =
        skipJsonWhitespace(json, 0);

    if (
        position >= json.size() ||
        json[position] != '{')
    {
        return false;
    }

    int objectDepth = 0;
    int arrayDepth = 0;
    bool inString = false;
    bool escaped = false;

    for (
        std::size_t index = position;
        index < json.size();
        ++index)
    {
        const char character =
            json[index];

        if (inString)
        {
            if (escaped)
            {
                escaped = false;
                continue;
            }

            if (character == '\\')
            {
                escaped = true;
                continue;
            }

            if (character == '"')
            {
                inString = false;
            }

            continue;
        }

        if (character == '"')
        {
            const std::size_t stringEnd =
                findJsonStringEnd(
                    json,
                    index);

            if (stringEnd == std::string::npos)
            {
                return false;
            }

            if (
                objectDepth == 1 &&
                arrayDepth == 0)
            {
                const std::string candidateKey =
                    decodeJsonString(
                        json.substr(
                            index + 1,
                            stringEnd - index - 1));

                const std::size_t afterKey =
                    skipJsonWhitespace(
                        json,
                        stringEnd + 1);

                if (
                    candidateKey == key &&
                    afterKey < json.size() &&
                    json[afterKey] == ':')
                {
                    const std::size_t valuePosition =
                        skipJsonWhitespace(
                            json,
                            afterKey + 1);

                    if (
                        valuePosition >= json.size() ||
                        json[valuePosition] != '"')
                    {
                        return false;
                    }

                    const std::size_t valueEnd =
                        findJsonStringEnd(
                            json,
                            valuePosition);

                    if (valueEnd == std::string::npos)
                    {
                        return false;
                    }

                    value =
                        decodeJsonString(
                            json.substr(
                                valuePosition + 1,
                                valueEnd - valuePosition - 1));

                    return true;
                }
            }

            index = stringEnd;
            continue;
        }

        if (character == '{')
        {
            ++objectDepth;
            continue;
        }

        if (character == '}')
        {
            --objectDepth;
            continue;
        }

        if (character == '[')
        {
            ++arrayDepth;
            continue;
        }

        if (character == ']')
        {
            --arrayDepth;
            continue;
        }
    }

    return false;
}

bool extractJsonObjectField(
    const std::string& json,
    const std::string& key,
    std::string& objectJson,
    std::size_t searchFrom)
{
    const std::size_t keyPosition =
        findJsonKey(
            json,
            key,
            searchFrom);

    if (keyPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t colonPosition =
        json.find(
            ':',
            keyPosition + key.size() + 2);

    if (colonPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t valuePosition =
        skipJsonWhitespace(
            json,
            colonPosition + 1);

    if (
        valuePosition >= json.size() ||
        json[valuePosition] != '{')
    {
        return false;
    }

    const std::size_t endPosition =
        findMatchingJsonDelimiter(
            json,
            valuePosition,
            '{',
            '}');

    if (endPosition == std::string::npos)
    {
        return false;
    }

    objectJson =
        json.substr(
            valuePosition,
            endPosition - valuePosition + 1);

    return true;
}

bool extractJsonArrayField(
    const std::string& json,
    const std::string& key,
    std::string& arrayJson,
    std::size_t searchFrom = 0)
{
    const std::size_t keyPosition =
        findJsonKey(
            json,
            key,
            searchFrom);

    if (keyPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t colonPosition =
        json.find(
            ':',
            keyPosition + key.size() + 2);

    if (colonPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t valuePosition =
        skipJsonWhitespace(
            json,
            colonPosition + 1);

    if (
        valuePosition >= json.size() ||
        json[valuePosition] != '[')
    {
        return false;
    }

    const std::size_t endPosition =
        findMatchingJsonDelimiter(
            json,
            valuePosition,
            '[',
            ']');

    if (endPosition == std::string::npos)
    {
        return false;
    }

    arrayJson =
        json.substr(
            valuePosition,
            endPosition - valuePosition + 1);

    return true;
}

bool extractJsonBoolField(
    const std::string& json,
    const std::string& key,
    bool& value,
    std::size_t searchFrom = 0)
{
    const std::size_t keyPosition =
        findJsonKey(
            json,
            key,
            searchFrom);

    if (keyPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t colonPosition =
        json.find(
            ':',
            keyPosition + key.size() + 2);

    if (colonPosition == std::string::npos)
    {
        return false;
    }

    const std::size_t valuePosition =
        skipJsonWhitespace(
            json,
            colonPosition + 1);

    if (
        json.compare(
            valuePosition,
            4,
            "true") == 0)
    {
        value = true;
        return true;
    }

    if (
        json.compare(
            valuePosition,
            5,
            "false") == 0)
    {
        value = false;
        return true;
    }

    return false;
}

std::vector<std::string> splitJsonArrayObjects(
    const std::string& arrayJson)
{
    std::vector<std::string> objects;

    const std::size_t openPosition =
        arrayJson.find('[');

    if (openPosition == std::string::npos)
    {
        return objects;
    }

    const std::size_t closePosition =
        findMatchingJsonDelimiter(
            arrayJson,
            openPosition,
            '[',
            ']');

    if (closePosition == std::string::npos)
    {
        return objects;
    }

    std::size_t position =
        openPosition + 1;

    while (position < closePosition)
    {
        position =
            skipJsonWhitespace(
                arrayJson,
                position);

        if (position >= closePosition)
        {
            break;
        }

        if (arrayJson[position] != '{')
        {
            return {};
        }

        const std::size_t endPosition =
            findMatchingJsonDelimiter(
                arrayJson,
                position,
                '{',
                '}');

        if (
            endPosition == std::string::npos ||
            endPosition >= closePosition)
        {
            return {};
        }

        objects.push_back(
            arrayJson.substr(
                position,
                endPosition - position + 1));

        position =
            endPosition + 1;

        position =
            skipJsonWhitespace(
                arrayJson,
                position);

        if (
            position < closePosition &&
            arrayJson[position] == ',')
        {
            ++position;
            continue;
        }

        if (position != closePosition)
        {
            return {};
        }
    }

    return objects;
}

bool stringsEqualIgnoreCase(
    std::string left,
    std::string right)
{
    left = trim(std::move(left));
    right = trim(std::move(right));

    std::transform(
        left.begin(),
        left.end(),
        left.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(
                std::tolower(character));
        });

    std::transform(
        right.begin(),
        right.end(),
        right.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(
                std::tolower(character));
        });

    return left == right;
}

// ============================================================
// WEB AUTHENTICATION
// ============================================================

bool loginWebUser(
    const std::string& baseUrl,
    AuthSession& session)
{
    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 1 - WEB AUTHENTICATION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::string email =
        readEnvironmentValue(
            "FORENWIPE_DESKTOP_EMAIL");

    std::string password =
        readEnvironmentValue(
            "FORENWIPE_DESKTOP_PASSWORD");

    if (email.empty())
    {
        std::cout
            << "\nWorkstation employee email: ";
        email =
            trim(readLine());
    }
    else
    {
        std::cout
            << "\nWorkstation employee email: "
            << email
            << " (environment)\n";
    }

    if (email.empty())
    {
        Console::fail(
            "Employee email is required.");
        return false;
    }

    if (password.empty())
    {
        password =
            readSecret(
                "Password");
    }
    else
    {
        std::cout
            << "Password: (environment)\n";
    }

    if (password.empty())
    {
        Console::fail(
            "Employee password is required.");
        return false;
    }

    session.baseUrl = baseUrl;
    session.email = email;
    session.password = password;

    std::ostringstream body;

    body
        << "{"
        << "\"email\":\""
        << jsonEscape(email)
        << "\","
        << "\"password\":\""
        << jsonEscape(password)
        << "\""
        << "}";

    const HttpResponse response =
        sendHttpRequest(
            "POST",
            baseUrl + "/api/auth/login",
            {},
            body.str());

    if (!expectHttpSuccess(
            response,
            "Web authentication"))
    {
        return false;
    }

    std::string userJson;

    if (!extractJsonStringField(
            response.body,
            "token",
            session.token) ||
        session.token.empty())
    {
        Console::fail(
            "Login succeeded but the web response did not contain a usable JWT token.");
        return false;
    }

    extractJsonObjectField(
        response.body,
        "user",
        userJson);

    if (!userJson.empty())
    {
        extractJsonStringField(
            userJson,
            "id",
            session.userId);

        if (session.userId.empty())
        {
            extractJsonStringField(
                userJson,
                "_id",
                session.userId);
        }

        extractJsonStringField(
            userJson,
            "email",
            session.email);

        extractJsonStringField(
            userJson,
            "name",
            session.name);

        extractJsonStringField(
            userJson,
            "role",
            session.role);
    }

    if (session.email.empty())
    {
        session.email = email;
    }

    if (session.role !=
        "WORKSTATION_EMPLOYEE")
    {
        Console::fail(
            "Authenticated account is not a WORKSTATION_EMPLOYEE account.");
        return false;
    }

    Console::pass(
        "Web authentication succeeded.");

    std::cout
        << "\nAuthenticated employee : "
        << (session.name.empty()
                ? session.email
                : session.name)
        << "\nEmail                  : "
        << session.email
        << "\nRole                   : "
        << session.role
        << "\nUser ID                : "
        << (session.userId.empty()
                ? "(not returned)"
                : session.userId)
        << '\n';

    return true;
}

bool loadAssignedSanitizationRequest(
    const std::string& baseUrl,
    AuthSession& session,
    const std::string& requestedRequestId,
    WebSanitizationRequestBinding& binding)
{
    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 2 - LOAD EXACT WEB SANITIZATION REQUEST"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::string requestId =
        trim(requestedRequestId);

    if (requestId.empty())
    {
        std::cout
            << "\nEnter REAL sanitization request ID: ";
    }

    if (requestId.empty())
    {
        requestId =
            trim(readLine());
    }

    if (requestId.empty())
    {
        Console::fail(
            "Sanitization request ID is required.");
        return false;
    }

    const HttpResponse response =
        sendAuthenticatedHttpRequest(
            "GET",
            baseUrl +
                "/api/sanitization-requests/employee",
            session);

    if (!expectHttpSuccess(
            response,
            "Fetch assigned sanitization requests"))
    {
        return false;
    }

    std::string dataArray;

    if (!extractJsonArrayField(
            response.body,
            "data",
            dataArray))
    {
        Console::fail(
            "Employee request API returned no data array.");
        return false;
    }

    const auto records =
        splitJsonArrayObjects(
            dataArray);

    if (records.empty())
    {
        Console::fail(
            "No sanitization requests are currently assigned to this employee.");
        return false;
    }

    for (const auto& record : records)
    {
        std::string recordRequestId;

        if (!extractJsonStringField(
                record,
                "requestId",
                recordRequestId))
        {
            continue;
        }

        if (recordRequestId != requestId)
        {
            continue;
        }

        binding.requestId =
            recordRequestId;

        extractTopLevelJsonStringField(
            record,
            "status",
            binding.status);

        extractJsonStringField(
            record,
            "deviceType",
            binding.deviceType);

        extractJsonStringField(
            record,
            "capacity",
            binding.capacity);

        extractJsonStringField(
            record,
            "serialNumber",
            binding.serialNumber);

        extractJsonStringField(
            record,
            "assetIdentifier",
            binding.assetIdentifier);

        std::string employeeJson;
        std::string workstationJson;

        extractJsonObjectField(
            record,
            "assignedEmployee",
            employeeJson);

        extractJsonObjectField(
            record,
            "assignedWorkstation",
            workstationJson);

        if (!employeeJson.empty())
        {
            extractJsonStringField(
                employeeJson,
                "_id",
                binding.assignedEmployeeId);

            if (binding.assignedEmployeeId.empty())
            {
                extractJsonStringField(
                    employeeJson,
                    "id",
                    binding.assignedEmployeeId);
            }

            extractJsonStringField(
                employeeJson,
                "email",
                binding.assignedEmployeeEmail);
        }

        if (!workstationJson.empty())
        {
            extractJsonStringField(
                workstationJson,
                "_id",
                binding.workstationMongoId);

            if (binding.workstationMongoId.empty())
            {
                extractJsonStringField(
                    workstationJson,
                    "id",
                    binding.workstationMongoId);
            }

            extractJsonStringField(
                workstationJson,
                "workstationId",
                binding.workstationId);

            extractJsonStringField(
                workstationJson,
                "name",
                binding.workstationName);

            extractJsonStringField(
                workstationJson,
                "status",
                binding.workstationStatus);
        }

        if (
            !binding.assignedEmployeeEmail.empty() &&
            !stringsEqualIgnoreCase(
                binding.assignedEmployeeEmail,
                session.email))
        {
            Console::fail(
                "The returned request is not assigned to the authenticated employee email.");
            return false;
        }

        if (binding.status != "ASSIGNED")
        {
            Console::fail(
                "Request " +
                requestId +
                " is not in ASSIGNED state. Current state: " +
                binding.status);
            return false;
        }

        if (binding.serialNumber.empty())
        {
            Console::fail(
                "The selected request has no authorized physical serial number.");
            return false;
        }

        if (binding.deviceType.empty())
        {
            Console::fail(
                "The selected request has no authorized device type.");
            return false;
        }

        if (binding.workstationId.empty())
        {
            Console::fail(
                "The selected request has no assigned workstation ID.");
            return false;
        }

        if (binding.workstationStatus != "ACTIVE")
        {
            Console::fail(
                "Assigned workstation is not ACTIVE. Current state: " +
                binding.workstationStatus);
            return false;
        }

        Console::pass(
            "Exact request retrieved from the authenticated employee's assigned workload.");

        std::cout
            << "\nRequest ID       : "
            << binding.requestId
            << "\nRequest status   : "
            << binding.status
            << "\nDevice type      : "
            << binding.deviceType
            << "\nRequested capacity: "
            << binding.capacity
            << "\nAuthorized serial: "
            << binding.serialNumber
            << "\nAsset identifier : "
            << (binding.assetIdentifier.empty()
                    ? "(none)"
                    : binding.assetIdentifier)
            << "\nAssigned employee: "
            << binding.assignedEmployeeEmail
            << "\nWorkstation ID   : "
            << binding.workstationId
            << "\nWorkstation name : "
            << binding.workstationName
            << "\nWorkstation state: "
            << binding.workstationStatus
            << '\n';

        return true;
    }

    Console::fail(
        "Request " +
        requestId +
        " is not present in the authenticated employee's assigned requests.");

    return false;
}

// ============================================================
// LOCAL WORKSTATION IDENTITY
// ============================================================

bool readMachineGuid(
    std::string& machineGuid)
{
    HKEY key = nullptr;

    const LONG openResult =
        RegOpenKeyExA(
            HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\Cryptography",
            0,
            KEY_READ | KEY_WOW64_64KEY,
            &key);

    if (openResult != ERROR_SUCCESS)
    {
        return false;
    }

    char buffer[256]{};
    DWORD bufferSize =
        static_cast<DWORD>(sizeof(buffer));
    DWORD type = 0;

    const LONG queryResult =
        RegQueryValueExA(
            key,
            "MachineGuid",
            nullptr,
            &type,
            reinterpret_cast<LPBYTE>(buffer),
            &bufferSize);

    RegCloseKey(key);

    if (
        queryResult != ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ) ||
        bufferSize == 0)
    {
        return false;
    }

    buffer[sizeof(buffer) - 1] = '\0';
    machineGuid =
        trim(std::string(buffer));

    return !machineGuid.empty();
}

bool calculateSha256Hex(
    const std::string& input,
    std::string& hashHex)
{
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;

    DWORD hashObjectLength = 0;
    DWORD hashLength = 0;
    DWORD bytesWritten = 0;

    NTSTATUS status =
        BCryptOpenAlgorithmProvider(
            &algorithm,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            0);

    if (status < 0)
    {
        return false;
    }

    status =
        BCryptGetProperty(
            algorithm,
            BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(
                &hashObjectLength),
            sizeof(hashObjectLength),
            &bytesWritten,
            0);

    if (status < 0 || hashObjectLength == 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithm,
            0);
        return false;
    }

    status =
        BCryptGetProperty(
            algorithm,
            BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(
                &hashLength),
            sizeof(hashLength),
            &bytesWritten,
            0);

    if (status < 0 || hashLength == 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithm,
            0);
        return false;
    }

    std::vector<unsigned char> hashObject(
        hashObjectLength);

    std::vector<unsigned char> digest(
        hashLength);

    status =
        BCryptCreateHash(
            algorithm,
            &hash,
            hashObject.data(),
            hashObjectLength,
            nullptr,
            0,
            0);

    if (status < 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithm,
            0);
        return false;
    }

    status =
        BCryptHashData(
            hash,
            reinterpret_cast<PUCHAR>(
                const_cast<char*>(
                    input.data())),
            static_cast<ULONG>(
                input.size()),
            0);

    if (status >= 0)
    {
        status =
            BCryptFinishHash(
                hash,
                digest.data(),
                hashLength,
                0);
    }

    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(
        algorithm,
        0);

    if (status < 0)
    {
        return false;
    }

    static constexpr char hex[] =
        "0123456789abcdef";

    hashHex.clear();
    hashHex.reserve(
        static_cast<std::size_t>(hashLength) * 2);

    for (const unsigned char byte : digest)
    {
        hashHex.push_back(
            hex[(byte >> 4) & 0x0F]);
        hashHex.push_back(
            hex[byte & 0x0F]);
    }

    return true;
}

bool bindWorkstationIdentity(
    const std::string& baseUrl,
    AuthSession& session,
    const std::string& workstationId)
{
    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 3 - WORKSTATION IDENTITY VERIFICATION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::string machineGuid;

    if (!readMachineGuid(machineGuid))
    {
        Console::fail(
            "Windows MachineGuid could not be read. Destructive execution is blocked.");
        return false;
    }

    std::string machineFingerprint;
    std::string legacyMachineFingerprint;

    if (!calculateSha256Hex(
            "SecureWipe|MachineGuid|" + machineGuid,
            machineFingerprint) ||
        !calculateSha256Hex(
            machineGuid,
            legacyMachineFingerprint))
    {
        Console::fail(
            "Could not calculate the SecureWipe workstation fingerprint.");
        return false;
    }

    char hostnameBuffer[256]{};
    DWORD hostnameSize =
        static_cast<DWORD>(sizeof(hostnameBuffer));

    if (!GetComputerNameA(
            hostnameBuffer,
            &hostnameSize))
    {
        hostnameBuffer[0] = '\0';
    }

    std::ostringstream body;

    body
        << "{"
        << "\"workstationId\":\""
        << jsonEscape(workstationId)
        << "\","
        << "\"machineFingerprint\":\""
        << machineFingerprint
        << "\","
        << "\"legacyMachineFingerprint\":\""
        << legacyMachineFingerprint
        << "\","
        << "\"hostname\":\""
        << jsonEscape(
               std::string(hostnameBuffer))
        << "\","
        << "\"operatingSystem\":{"
        << "\"name\":\"Windows\""
        << "}"
        << "}";

    const HttpResponse response =
        sendAuthenticatedHttpRequest(
            "POST",
            baseUrl +
                "/api/workstations/identity",
            session,
            body.str());

    if (!expectHttpSuccess(
            response,
            "Workstation identity verification"))
    {
        return false;
    }

    std::string returnedWorkstationId;
    std::string returnedFingerprint;

    std::string dataJson;
    extractJsonObjectField(
        response.body,
        "data",
        dataJson);

    if (!dataJson.empty())
    {
        extractJsonStringField(
            dataJson,
            "workstationId",
            returnedWorkstationId);

        extractJsonStringField(
            dataJson,
            "machineFingerprint",
            returnedFingerprint);
    }

    if (
        !returnedWorkstationId.empty() &&
        returnedWorkstationId != workstationId)
    {
        Console::fail(
            "Backend returned a different workstation identity than the assigned request.");
        return false;
    }

    if (
        !returnedFingerprint.empty() &&
        returnedFingerprint != machineFingerprint)
    {
        Console::fail(
            "Backend returned a different machine fingerprint than this physical desktop.");
        return false;
    }

    Console::pass(
        "Authenticated employee is using the exact workstation assigned to the request.");

    return true;
}

// ============================================================
// DEVICE / REQUEST MATCHING
// ============================================================
//
// The web request's authorized SERIAL NUMBER is the sole device
// identity criterion here.
//
// IMPORTANT:
// - requestBinding.deviceType is NOT used for physical-device matching.
// - USB / NVMe / SATA are transport/interface characteristics and
//   must not be compared directly with the request's logical media
//   type (for example HDD).
// - Safety checks later in the pipeline still decide whether the
//   serial-matched device is safe to sanitize.
//

std::optional<StorageDevice> findRequestTarget(
    const std::vector<StorageDevice>& devices,
    const WebSanitizationRequestBinding& binding)
{
    std::vector<StorageDevice> matches;

    const std::string authorizedSerial =
        trim(binding.serialNumber);

    if (authorizedSerial.empty())
    {
        return std::nullopt;
    }

    for (const auto& device : devices)
    {
        const std::string detectedSerial =
            trim(device.getSerialNumber());

        if (detectedSerial.empty())
        {
            continue;
        }

        // EXACT PHYSICAL DEVICE IDENTITY MATCH:
        // Only the authorized serial number is used.
        if (!stringsEqualIgnoreCase(
                detectedSerial,
                authorizedSerial))
        {
            continue;
        }

        matches.push_back(device);
    }

    if (matches.empty())
    {
        return std::nullopt;
    }

    // The serial number must resolve to exactly one currently
    // detected physical device. Never guess when it is ambiguous.
    if (matches.size() != 1)
    {
        Console::fail(
            "More than one currently detected physical device has the authorized serial number. Destructive execution is blocked to avoid ambiguity.");
        return std::nullopt;
    }

    return matches.front();
}

bool updateSanitizationRequestStatus(
    const std::string& baseUrl,
    AuthSession& session,
    const std::string& requestId,
    const std::string& status)
{
    std::ostringstream body;

    body
        << "{"
        << "\"status\":\""
        << jsonEscape(status)
        << "\""
        << "}";

    const HttpResponse response =
        sendAuthenticatedHttpRequest(
            "PATCH",
            baseUrl +
                "/api/sanitization-requests/" +
                requestId +
                "/employee-status",
            session,
            body.str());

    return expectHttpSuccess(
        response,
        "Sanitization request status update to " +
            status);
}

// ============================================================
// WEB RESULT / CERTIFICATE / AUDIT PUBLICATION
// ============================================================

std::string sanitizationResultToJson(
    const SanitizationPipelineResult& pipelineResult,
    const std::string& workstationId)
{
    const SanitizationResult& result =
        pipelineResult.sanitization;

    std::ostringstream body;

    body
        << "{"
        << "\"operationId\":\""
        << jsonEscape(result.operationId)
        << "\","

        << "\"deviceId\":\""
        << jsonEscape(result.deviceId)
        << "\","

        << "\"model\":\""
        << jsonEscape(result.model)
        << "\","

        << "\"serialNumber\":\""
        << jsonEscape(result.serialNumber)
        << "\","

        << "\"capacityBytes\":"
        << result.capacityBytes
        << ","

        << "\"interfaceType\":\""
        << jsonEscape(result.interfaceType)
        << "\","

        << "\"method\":\""
        << jsonEscape(
               SanitizationEvent::methodName(
                   result.method))
        << "\","

        << "\"status\":\""
        << jsonEscape(
               sanitizationStatusToString(
                   result.status))
        << "\","

        << "\"bytesProcessed\":"
        << result.bytesProcessed
        << ","

        << "\"operationDurationMs\":"
        << result.operationDurationMs
        << ","

        << "\"verificationStatus\":\""
        << jsonEscape(
               verificationStatusToString(
                   result.verificationStatus))
        << "\","

        << "\"verificationPerformed\":"
        << (result.verificationPerformed
                ? "true"
                : "false")
        << ","

        << "\"bytesVerified\":"
        << result.bytesVerified
        << ","

        << "\"verificationSamples\":"
        << result.verificationSamples
        << ","

        << "\"verificationMessage\":\""
        << jsonEscape(result.verificationMessage)
        << "\","

        << "\"nativeErrorCode\":"
        << result.nativeErrorCode
        << ","

        << "\"deviceReportedSuccess\":false,"
        << "\"globalDataErased\":false,"

        << "\"message\":\""
        << jsonEscape(result.message)
        << "\","

        << "\"errorMessage\":\""
        << jsonEscape(result.errorMessage)
        << "\","

        << "\"workstationId\":\""
        << jsonEscape(workstationId)
        << "\""

        << "}";

    return body.str();
}

bool readTextFile(
    const std::filesystem::path& path,
    std::string& content)
{
    std::ifstream input(
        path,
        std::ios::in |
        std::ios::binary);

    if (!input)
    {
        return false;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();
    content = buffer.str();

    return !content.empty();
}

bool submitSanitizationResultToWeb(
    const std::string& baseUrl,
    AuthSession& session,
    const std::string& requestId,
    const std::string& workstationId,
    const SanitizationPipelineResult& result)
{
    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 13 - UPLOAD SANITIZATION RESULT TO WEB"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    if (!result.sanitization.isSuccess())
    {
        Console::fail(
            "Refusing to upload a non-successful sanitization result.");
        return false;
    }

    const std::string body =
        sanitizationResultToJson(
            result,
            workstationId);

    const HttpResponse response =
        sendAuthenticatedHttpRequest(
            "POST",
            baseUrl +
                "/api/sanitization-results/" +
                requestId,
            session,
            body);

    if (!expectHttpSuccess(
            response,
            "Sanitization result upload"))
    {
        return false;
    }

    std::string serverOperationId;

    std::string dataJson;
    extractJsonObjectField(
        response.body,
        "data",
        dataJson);

    if (!dataJson.empty())
    {
        extractJsonStringField(
            dataJson,
            "operationId",
            serverOperationId);
    }

    if (
        !serverOperationId.empty() &&
        serverOperationId != result.sanitization.operationId)
    {
        Console::fail(
            "Backend accepted a result with a different operation ID.");
        return false;
    }

    Console::pass(
        "Sanitization result accepted by backend; request is now in VERIFYING state.");

    return true;
}

bool uploadCertificateToWeb(
    const std::string& baseUrl,
    AuthSession& session,
    const std::string& requestId,
    const std::string& workstationId,
    const SanitizationPipelineResult& result)
{
    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 14 - UPLOAD TAMPER-EVIDENT CERTIFICATE"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    if (
        !result.certificateGenerated ||
        result.certificatePath.empty() ||
        !std::filesystem::exists(
            result.certificatePath))
    {
        Console::fail(
            "Persisted native certificate JSON is missing.");
        return false;
    }

    std::string certificateJson;

    if (!readTextFile(
            result.certificatePath,
            certificateJson))
    {
        Console::fail(
            "Could not read the persisted certificate JSON.");
        return false;
    }

    std::string certificateId;
    std::string certificateRequestId;
    std::string certificateOperationId;
    std::string certificateWorkstationId;
    std::string certificateHash;

    extractJsonStringField(
        certificateJson,
        "certificateId",
        certificateId);

    extractJsonStringField(
        certificateJson,
        "requestId",
        certificateRequestId);

    extractJsonStringField(
        certificateJson,
        "operationId",
        certificateOperationId);

    extractJsonStringField(
        certificateJson,
        "workstationId",
        certificateWorkstationId);

    extractJsonStringField(
        certificateJson,
        "certificateHash",
        certificateHash);

    if (
        certificateRequestId != requestId ||
        certificateOperationId !=
            result.sanitization.operationId ||
        certificateWorkstationId != workstationId ||
        certificateHash.empty())
    {
        Console::fail(
            "Persisted certificate metadata does not match the active web request/operation/workstation.");
        return false;
    }

    std::cout
        << "Certificate ID : "
        << certificateId
        << "\nOperation ID   : "
        << certificateOperationId
        << "\nRequest ID     : "
        << certificateRequestId
        << "\nWorkstation ID : "
        << certificateWorkstationId
        << "\nSHA-256        : "
        << certificateHash
        << '\n';

    const HttpResponse uploadResponse =
        sendAuthenticatedHttpRequest(
            "POST",
            baseUrl +
                "/api/sanitization-certificates/" +
                requestId,
            session,
            certificateJson);

    if (!expectHttpSuccess(
            uploadResponse,
            "Certificate upload"))
    {
        return false;
    }

    Console::pass(
        "Certificate accepted by backend; server-side SHA-256 validation succeeded.");

    const HttpResponse verificationResponse =
        sendAuthenticatedHttpRequest(
            "GET",
            baseUrl +
                "/api/sanitization-certificates/" +
                certificateId +
                "/verify",
            session);

    if (!expectHttpSuccess(
            verificationResponse,
            "Remote certificate verification"))
    {
        return false;
    }

    bool valid = false;

    if (!extractJsonBoolField(
            verificationResponse.body,
            "valid",
            valid) ||
        !valid)
    {
        Console::fail(
            "Website certificate verification endpoint did not report VALID.");
        return false;
    }

    Console::pass(
        "Website independently re-verified the stored certificate SHA-256 as VALID.");

    return true;
}

bool uploadAuditChainToWeb(
    const std::string& baseUrl,
    AuthSession& session,
    const std::string& requestId,
    const std::string& workstationId,
    const SanitizationPipelineResult& result)
{
    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 15 - UPLOAD NATIVE AUDIT HASH CHAIN"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    if (
        result.auditLogPath.empty() ||
        !std::filesystem::exists(
            result.auditLogPath))
    {
        Console::fail(
            "Native audit JSONL file is missing.");
        return false;
    }

    std::string auditLog;

    if (!readTextFile(
            result.auditLogPath,
            auditLog))
    {
        Console::fail(
            "Could not read the native audit JSONL file.");
        return false;
    }

    const std::size_t auditBytes =
        auditLog.size();

    if (
        auditBytes >
        9ULL * 1024ULL * 1024ULL)
    {
        Console::fail(
            "Native audit JSONL is too large for the configured web upload envelope.");
        return false;
    }

    std::ostringstream body;

    body
        << "{"
        << "\"operationId\":\""
        << jsonEscape(
               result.sanitization.operationId)
        << "\","

        << "\"certificateId\":\""
        << jsonEscape(
               result.certificate.certificateId)
        << "\","

        << "\"workstationId\":\""
        << jsonEscape(workstationId)
        << "\","

        << "\"auditLog\":\""
        << jsonEscape(auditLog)
        << "\""

        << "}";

    const HttpResponse uploadResponse =
        sendAuthenticatedHttpRequest(
            "POST",
            baseUrl +
                "/api/sanitization-audit/" +
                requestId,
            session,
            body.str());

    if (!expectHttpSuccess(
            uploadResponse,
            "Audit chain upload"))
    {
        return false;
    }

    Console::pass(
        "Complete native audit ledger accepted and cryptographically verified by backend.");

    const HttpResponse verificationResponse =
        sendAuthenticatedHttpRequest(
            "GET",
            baseUrl +
                "/api/sanitization-audit/" +
                requestId +
                "/verify",
            session);

    if (!expectHttpSuccess(
            verificationResponse,
            "Remote audit-chain verification"))
    {
        return false;
    }

    bool valid = false;

    if (!extractJsonBoolField(
            verificationResponse.body,
            "valid",
            valid) ||
        !valid)
    {
        Console::fail(
            "Website audit-chain verification endpoint did not report VALID.");
        return false;
    }

    Console::pass(
        "Website re-verified the stored native audit SHA-256 hash chain as VALID.");

    return true;
}

bool verifyFinalWebRequestState(
    const std::string& baseUrl,
    AuthSession& session,
    const WebSanitizationRequestBinding& originalBinding,
    const SanitizationPipelineResult& result)
{
    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 16 - FINAL WEB STATE VERIFICATION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    const HttpResponse response =
        sendAuthenticatedHttpRequest(
            "GET",
            baseUrl +
                "/api/sanitization-requests/employee",
            session);

    if (!expectHttpSuccess(
            response,
            "Final employee request-state fetch"))
    {
        return false;
    }

    std::string dataArray;

    if (!extractJsonArrayField(
            response.body,
            "data",
            dataArray))
    {
        Console::fail(
            "Final request-state response has no data array.");
        return false;
    }

    for (const auto& record :
         splitJsonArrayObjects(dataArray))
    {
        std::string requestId;

        if (!extractJsonStringField(
                record,
                "requestId",
                requestId) ||
            requestId != originalBinding.requestId)
        {
            continue;
        }

        std::string status;
        std::string serialNumber;
        std::string workstationJson;
        std::string workstationId;

        extractTopLevelJsonStringField(
            record,
            "status",
            status);

        extractJsonStringField(
            record,
            "serialNumber",
            serialNumber);

        extractJsonObjectField(
            record,
            "assignedWorkstation",
            workstationJson);

        if (!workstationJson.empty())
        {
            extractJsonStringField(
                workstationJson,
                "workstationId",
                workstationId);
        }

        if (
            status != "COMPLETED" ||
            !stringsEqualIgnoreCase(
                serialNumber,
                result.sanitization.serialNumber) ||
            workstationId !=
                originalBinding.workstationId)
        {
            Console::fail(
                "Final website state does not show the exact sanitization request as COMPLETED with the same target and workstation.");
            return false;
        }

        Console::pass(
            "Website request is COMPLETED and still bound to the same physical serial/workstation.");
        return true;
    }

    Console::fail(
        "The completed sanitization request disappeared from the authenticated employee workload.");
    return false;
}

// ============================================================
// DEVICE SELECTION
// ============================================================

bool parseIndex(
    const std::string& input,
    std::size_t count,
    std::size_t& index)
{
    try
    {
        std::size_t consumed = 0;

        const unsigned long long value =
            std::stoull(
                input,
                &consumed);

        if (
            consumed != input.size() ||
            value == 0 ||
            value > count)
        {
            return false;
        }

        index =
            static_cast<std::size_t>(
                value - 1);

        return true;
    }
    catch (...)
    {
        return false;
    }
}

// ============================================================
// DEVICE DISPLAY
// ============================================================

void printDevice(
    const StorageDevice& device,
    std::size_t number)
{
    std::cout
        << "\n"
        << Console::CYAN
        << "  [" << number << "]"
        << Console::RESET
        << "\n"

        << "      Device ID   : "
        << device.getDeviceId()
        << '\n'

        << "      Model       : "
        << device.getModel()
        << '\n'

        << "      Serial      : "
        << device.getSerialNumber()
        << '\n'

        << "      Interface   : "
        << device.getInterfaceType()
        << '\n'

        << "      Capacity    : "
        << device.getCapacityBytes()
        << " bytes\n"

        << "      System Disk : ";

    if (
        device.isSystemDisk())
    {
        std::cout
            << Console::BOLD_RED
            << "YES - BLOCKED"
            << Console::RESET;
    }
    else
    {
        std::cout
            << Console::GREEN
            << "NO"
            << Console::RESET;
    }

    std::cout
        << '\n'

        << "      Removable   : "
        << (
            device.isRemovable()
                ? "YES"
                : "NO"
        )
        << '\n';
}

// ============================================================
// DESTRUCTIVE CONFIRMATION
// ============================================================

bool confirmTarget(
    const StorageDevice& device)
{
    separator();

    std::cout
        << Console::BOLD_YELLOW
        << "FINAL DESTRUCTIVE AUTHORIZATION"
        << Console::RESET
        << "\n"

        << "------------------------------------------------------------\n"

        << "\n"
        << "This operation permanently overwrites the selected device.\n"
        << "Use ONLY a disposable/sacrificial device.\n\n"

        << "Device ID : "
        << device.getDeviceId()
        << '\n'

        << "Model     : "
        << device.getModel()
        << '\n'

        << "Serial    : "
        << device.getSerialNumber()
        << '\n'

        << "Capacity  : "
        << device.getCapacityBytes()
        << " bytes\n"

        << "Method    : Host Overwrite\n\n"

        << "Enter EXACT serial number: ";

    if (
        trim(
            readLine()) !=
        device.getSerialNumber())
    {
        Console::stop(
            "Serial number mismatch. Operation cancelled.");

        return false;
    }

    std::cout
        << "\nType EXACTLY:\n"
        << Console::BOLD_YELLOW
        << "START HOST OVERWRITE"
        << Console::RESET
        << "\n"
        << "Confirmation: ";

    if (
        toUpper(
            trim(
                readLine())) !=
        "START HOST OVERWRITE")
    {
        Console::stop(
            "Final destructive authorization failed.");

        return false;
    }

    Console::pass(
        "Destructive authorization accepted.");

    return true;
}

// ============================================================
// AUDIT FILE CONTENT CHECK
// ============================================================

bool containsText(
    const std::filesystem::path& file,
    const std::string& text)
{
    std::ifstream input(
        file,
        std::ios::in |
        std::ios::binary);

    if (!input)
        return false;

    std::string line;

    while (
        std::getline(
            input,
            line))
    {
        if (
            line.find(text) !=
            std::string::npos)
        {
            return true;
        }
    }

    return false;
}

// ============================================================
// AUDIT EVENT PRESENCE CHECK
// ============================================================

bool validateAuditTrail(
    const SanitizationPipelineResult& result)
{
    separator();

    std::cout
        << Console::CYAN
        << "AUDIT EVENT PERSISTENCE"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    const std::filesystem::path auditPath(
        result.auditLogPath);

    if (
        !std::filesystem::exists(
            auditPath))
    {
        Console::fail(
            "Audit log does not exist: " +
            auditPath.string());

        return false;
    }

    Console::pass(
        "Audit log exists.");

    const std::vector<std::string>
        requiredEvents = {
            "PIPELINE_STARTED",
            "SAFETY_CHECK_COMPLETED",
            "TARGET_VALIDATED",
            "SANITIZATION_STARTED",
            "SANITIZATION_COMPLETED",
            "VERIFICATION_COMPLETED",
            "CERTIFICATE_GENERATED",
            "CERTIFICATE_PERSISTED",
            "PIPELINE_COMPLETED"
        };

    bool passed = true;

    for (
        const auto& event :
        requiredEvents)
    {
        if (
            containsText(
                auditPath,
                "\"eventType\":\"" +
                    event +
                    "\""))
        {
            Console::pass(
                event);
        }
        else
        {
            Console::fail(
                "Missing audit event: " +
                event);

            passed = false;
        }
    }

    return passed;
}

// ============================================================
// CERTIFICATE EVIDENCE CHECK
// ============================================================

bool validateCertificate(
    const SanitizationPipelineResult& result,
    const StorageDevice& target,
    const std::string& expectedRequestId,
    const std::string& expectedWorkstationId)
{
    separator();

    std::cout
        << Console::CYAN
        << "CERTIFICATE EVIDENCE"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    bool passed = true;

    const auto& certificate =
        result.certificate;

    const auto& sanitization =
        result.sanitization;

    if (
        certificate.certificateId.empty())
    {
        Console::fail(
            "Certificate ID is empty.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Certificate ID generated.");
    }

    if (
        certificate.operationId.empty() ||
        certificate.operationId !=
            sanitization.operationId)
    {
        Console::fail(
            "Certificate operation ID mismatch.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Operation ID matches sanitization result.");
    }

    if (
        certificate.requestId.empty())
    {
        Console::fail(
            "Certificate request ID is empty.");

        passed = false;
    }
    else if (
        certificate.requestId !=
        expectedRequestId)
    {
        Console::fail(
            "Certificate request ID does not match the real web request.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Real request ID preserved.");
    }

    if (
        certificate.workstationId.empty())
    {
        Console::fail(
            "Certificate workstation ID is empty.");

        passed = false;
    }
    else if (
        certificate.workstationId !=
        expectedWorkstationId)
    {
        Console::fail(
            "Certificate workstation ID does not match the assigned workstation.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Real workstation ID preserved.");
    }

    if (
        certificate.deviceId !=
        target.getDeviceId())
    {
        Console::fail(
            "Certificate device ID mismatch.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Device ID matches.");
    }

    if (
        certificate.serialNumber !=
        target.getSerialNumber())
    {
        Console::fail(
            "Certificate serial number mismatch.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Serial number matches.");
    }

    if (
        certificate.method !=
        SanitizationMethod::HostOverwrite)
    {
        Console::fail(
            "Certificate method is not Host Overwrite.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Certificate method is Host Overwrite.");
    }

    if (
        certificate.status !=
        SanitizationStatus::COMPLETED)
    {
        Console::fail(
            "Certificate status is not COMPLETED.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Certificate status is COMPLETED.");
    }

    if (
        !certificate.verificationPerformed ||
        certificate.verificationStatus !=
            VerificationStatus::PASSED ||
        !certificate.verificationPassed)
    {
        Console::fail(
            "Certificate verification evidence is incomplete.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Verification evidence is valid.");
    }

    if (
        certificate.bytesVerified !=
            sanitization.bytesVerified ||
        certificate.verificationSamples !=
            sanitization.verificationSamples)
    {
        Console::fail(
            "Certificate verification counters do not match.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Verification counters match.");
    }

    if (
        certificate.certificateHash.empty() ||
        certificate.certificateHash.size() !=
            64)
    {
        Console::fail(
            "Certificate SHA-256 hash is missing or invalid.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Certificate SHA-256 hash is present.");
    }

    if (
        !certificate.isValid())
    {
        Console::fail(
            "Certificate isValid() returned FALSE.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Certificate isValid() returned TRUE.");
    }

    const std::filesystem::path certificatePath(
        result.certificatePath);

    if (
        !result.certificatePersisted ||
        result.certificatePath.empty() ||
        !std::filesystem::exists(
            certificatePath))
    {
        Console::fail(
            "Persisted certificate file is missing.");

        passed = false;
    }
    else
    {
        Console::pass(
            "Persisted certificate file exists.");
    }

    return passed;
}

// ============================================================
// REAL SYSTEM-DISK SAFETY TEST
// ============================================================

bool runRealSystemDiskRejectionTest(
    const StorageDevice& systemDisk,
    const std::string& actorId)
{
    separator();

    std::cout
        << Console::CYAN
        << "CASE 1 - SYSTEM DISK PROTECTION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "\nThis case uses the ACTUAL Windows system disk.\n"
        << "No destructive write must occur.\n\n";

    printDevice(
        systemDisk,
        1);

    SanitizationPipeline pipeline;

    const SanitizationPipelineResult result =
        pipeline.execute(
            systemDisk,
            {},
            actorId);

    std::cout
        << "\nPipeline status   : "
        << sanitizationStatusToString(
               result.sanitization.status)
        << '\n'

        << "Pipeline message  : "
        << result.pipelineMessage
        << '\n'

        << "Bytes processed   : "
        << result.sanitization.bytesProcessed
        << '\n';

    bool passed = true;

    if (
        result.sanitization.status ==
        SanitizationStatus::COMPLETED)
    {
        Console::fail(
            "System disk was reported as sanitized.");

        passed = false;
    }
    else
    {
        Console::pass(
            "System disk sanitization was not completed.");
    }

    if (
        result.sanitization.bytesProcessed !=
        0)
    {
        Console::fail(
            "Bytes were processed on the system disk.");

        passed = false;
    }
    else
    {
        Console::pass(
            "No device bytes were processed.");
    }

    if (
        !result.auditTrailPersisted)
    {
        Console::fail(
            "System-disk safety rejection was not fully audited.");

        passed = false;
    }
    else
    {
        Console::pass(
            "System-disk safety rejection was audited.");
    }

    return passed;
}

// ============================================================
// MAIN
// ============================================================

} // namespace

int main()
{
    Console::enable();

    Console::title(
        "FORENWIPE  |  REAL WEB-BOUND HOST OVERWRITE E2E");

    Console::info(
        "Authenticated Request -> Exact Device -> Safety -> Host Overwrite -> Verify -> Result -> Certificate -> Audit -> Web Verification\n");

    separator();

    std::cout
        << Console::BOLD_YELLOW
        << "WARNING: THIS IS A REAL DESTRUCTIVE TEST."
        << Console::RESET
        << "\n\n"

        << "Use ONLY a disposable/sacrificial physical storage device.\n"
        << "NEVER use the Windows system disk.\n"
        << "The request ID and target serial are loaded from the authenticated\n"
        << "SecureWipe web account; the workstation ID is NOT manually entered.\n";

    // =========================================================
    // SESSION INITIALIZATION
    // =========================================================

    std::string actorId;

    if (!getCurrentWindowsUser(actorId))
    {
        Console::fail(
            "Could not determine the Windows actor.");
        return 1;
    }

    const std::string baseUrl =
        apiBaseUrl();

    if (baseUrl.empty())
    {
        Console::fail(
            "SecureWipe API base URL is empty.");
        return 1;
    }

    std::cout
        << "\nWindows actor : "
        << actorId
        << "\nWeb API       : "
        << baseUrl
        << '\n';

    // =========================================================
    // STEP 1 - AUTHENTICATE
    // =========================================================

    AuthSession session;

    if (!loginWebUser(
            baseUrl,
            session))
    {
        return 1;
    }

    // =========================================================
    // STEP 2 - REQUEST BINDING
    // =========================================================

    std::cout
        << "\n"
        << "Request ID to execute: ";

    const std::string requestId =
        trim(readLine());

    if (requestId.empty())
    {
        Console::fail(
            "Request ID is required.");
        return 2;
    }

    WebSanitizationRequestBinding requestBinding;

    if (!loadAssignedSanitizationRequest(
            baseUrl,
            session,
            requestId,
            requestBinding))
    {
        return 2;
    }

    // =========================================================
    // STEP 3 - WORKSTATION IDENTITY
    // =========================================================

    if (!bindWorkstationIdentity(
            baseUrl,
            session,
            requestBinding.workstationId))
    {
        return 3;
    }

    // =========================================================
    // STEP 4 - REAL DEVICE DISCOVERY
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 4 - REAL DEVICE DISCOVERY / EXACT REQUEST MATCH"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    WindowsStorageDiscovery discovery;

    std::vector<StorageDevice> devices;

    try
    {
        devices =
            discovery.discover();
    }
    catch (const std::exception& exception)
    {
        Console::fail(
            "Device discovery exception: " +
            std::string(exception.what()));
        return 4;
    }
    catch (...)
    {
        Console::fail(
            "Unknown exception during device discovery.");
        return 4;
    }

    if (devices.empty())
    {
        Console::fail(
            "No physical storage devices detected.");
        return 4;
    }

    Console::pass(
        "Detected " +
        std::to_string(devices.size()) +
        " physical device(s).");

    for (
        std::size_t index = 0;
        index < devices.size();
        ++index)
    {
        printDevice(
            devices[index],
            index + 1);
    }

    const auto systemDiskIt =
        std::find_if(
            devices.begin(),
            devices.end(),
            [](const StorageDevice& device)
            {
                return device.isSystemDisk();
            });

    if (systemDiskIt == devices.end())
    {
        Console::fail(
            "Windows system disk could not be identified.");
        return 5;
    }

    const StorageDevice systemDisk =
        *systemDiskIt;

    // =========================================================
    // STEP 5 - SYSTEM DISK REJECTION
    // =========================================================

    if (!runRealSystemDiskRejectionTest(
            systemDisk,
            actorId))
    {
        Console::fail(
            "System-disk safety test failed.");
        return 5;
    }

    Console::pass(
        "System-disk protection test passed.");

    // =========================================================
    // STEP 6 - EXACT REQUEST TARGET
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 6 - AUTOMATIC TARGET BINDING FROM WEB REQUEST"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    const auto targetOptional =
        findRequestTarget(
            devices,
            requestBinding);

    if (!targetOptional.has_value())
    {
        Console::fail(
            "No unique physical device matched the request's authorized serial number.");
        return 6;
    }

    StorageDevice selectedDevice =
        *targetOptional;

    if (selectedDevice.isSystemDisk())
    {
        Console::stop(
            "The request's authorized serial resolves to the Windows system disk. Destructive execution is blocked.");
        return 6;
    }

    if (
        selectedDevice.getDeviceId().empty() ||
        selectedDevice.getSerialNumber().empty() ||
        selectedDevice.getCapacityBytes() == 0)
    {
        Console::fail(
            "Request-matched target is missing required physical identity/capacity data.");
        return 6;
    }

    printDevice(
        selectedDevice,
        1);

    Console::pass(
        "The physical device was selected automatically from the exact web request authorized serial number; no device type matching or manual workstation/device ID was used.");

    // =========================================================
    // STEP 7 - PRE-FLIGHT SAFETY
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 7 - PRE-FLIGHT SAFETY"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    SafetyEngine safetyEngine;

    safetyEngine.setExpectedTarget(
        selectedDevice);

    const SafetyResult safetyResult =
        safetyEngine.evaluateWithResult(
            selectedDevice);

    std::cout
        << "\nDecision : "
        << safetyResult.decision
        << "\nSummary  : "
        << safetyResult.summary
        << "\n\n";

    for (const auto& check : safetyResult.checks)
    {
        if (check.passed)
        {
            Console::pass(
                check.checkName +
                " - " +
                check.message);
        }
        else
        {
            Console::fail(
                check.checkName +
                " - " +
                check.message);
        }
    }

    if (!safetyResult.isOverallSafe)
    {
        Console::stop(
            "Pre-flight safety rejected the request-matched physical target.");
        return 7;
    }

    Console::pass(
        "Pre-flight safety validation passed.");

    // =========================================================
    // STEP 8 - METHOD SELECTION
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 8 - SANITIZATION METHOD"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    const SanitizationCapability capability =
        detectSanitizationCapability(
            selectedDevice);

    SanitizationEngine sanitizationEngine;

    const SanitizationMethod method =
        sanitizationEngine.selectMethod(
            selectedDevice,
            capability);

    std::cout
        << "Selected method : "
        << methodToString(method)
        << '\n';

    if (method !=
        SanitizationMethod::HostOverwrite)
    {
        Console::stop(
            "This executable is the Host Overwrite E2E test and will not perform a different destructive method.");
        return 8;
    }

    Console::pass(
        "Host Overwrite selected for the request-matched target.");

    // =========================================================
    // STEP 9 - FINAL REVALIDATION
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 9 - FINAL PHYSICAL TARGET REVALIDATION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::vector<StorageDevice> freshDevices;

    try
    {
        freshDevices =
            discovery.discover();
    }
    catch (const std::exception& exception)
    {
        Console::fail(
            "Final device discovery failed: " +
            std::string(exception.what()));
        return 9;
    }
    catch (...)
    {
        Console::fail(
            "Unknown exception during final device discovery.");
        return 9;
    }

    const auto freshTargetOptional =
        findRequestTarget(
            freshDevices,
            requestBinding);

    if (!freshTargetOptional.has_value())
    {
        Console::stop(
            "The request-matched target disappeared or became ambiguous before execution.");
        return 9;
    }

    StorageDevice freshTarget =
        *freshTargetOptional;

    if (freshTarget.isSystemDisk())
    {
        Console::stop(
            "The request-matched target is now the Windows system disk.");
        return 9;
    }

    if (
        freshTarget.getDeviceId() !=
            selectedDevice.getDeviceId() ||
        !stringsEqualIgnoreCase(
            freshTarget.getSerialNumber(),
            selectedDevice.getSerialNumber()) ||
        freshTarget.getCapacityBytes() !=
            selectedDevice.getCapacityBytes())
    {
        Console::stop(
            "Physical target identity changed between discovery passes.");
        return 9;
    }

    SafetyEngine finalSafetyEngine;

    finalSafetyEngine.setExpectedTarget(
        freshTarget);

    const SafetyResult finalSafetyResult =
        finalSafetyEngine.evaluateWithResult(
            freshTarget);

    if (!finalSafetyResult.isOverallSafe)
    {
        Console::stop(
            "Final safety validation failed: " +
            finalSafetyResult.summary);
        return 9;
    }

    selectedDevice =
        freshTarget;

    Console::pass(
        "Final physical identity and safety checks passed.");

    // =========================================================
    // STEP 10 - FINAL DESTRUCTIVE AUTHORIZATION
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_YELLOW
        << "STEP 10 - FINAL DESTRUCTIVE AUTHORIZATION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    if (!confirmTarget(selectedDevice))
    {
        return 10;
    }

    // =========================================================
    // STEP 11 - IMMEDIATE POST-CONFIRMATION RECHECK
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 11 - IMMEDIATE POST-CONFIRMATION RECHECK"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::vector<StorageDevice> finalDevices;

    try
    {
        finalDevices =
            discovery.discover();
    }
    catch (...)
    {
        Console::stop(
            "The physical device could not be rediscovered after destructive confirmation. No write was started.");
        return 11;
    }

    const auto finalTargetOptional =
        findRequestTarget(
            finalDevices,
            requestBinding);

    if (!finalTargetOptional.has_value())
    {
        Console::stop(
            "The exact request-matched target could not be rediscovered after confirmation. No write was started.");
        return 11;
    }

    const StorageDevice finalTarget =
        *finalTargetOptional;

    if (
        finalTarget.isSystemDisk() ||
        finalTarget.getDeviceId() !=
            selectedDevice.getDeviceId() ||
        finalTarget.getCapacityBytes() !=
            selectedDevice.getCapacityBytes() ||
        !stringsEqualIgnoreCase(
            finalTarget.getSerialNumber(),
            requestBinding.serialNumber))
    {
        Console::stop(
            "Post-confirmation target identity no longer matches the web request. No write was started.");
        return 11;
    }

    selectedDevice =
        finalTarget;

    Console::pass(
        "The exact request-bound target is still present after confirmation.");

    // =========================================================
    // STEP 12 - MARK REQUEST IN PROGRESS
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 12 - MARK WEB REQUEST IN_PROGRESS"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    if (!updateSanitizationRequestStatus(
            baseUrl,
            session,
            requestBinding.requestId,
            "IN_PROGRESS"))
    {
        Console::stop(
            "The web request could not be moved to IN_PROGRESS. No destructive write was started.");
        return 12;
    }

    Console::pass(
        "The exact web request is now IN_PROGRESS.");

    // =========================================================
    // REAL HOST OVERWRITE
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "REAL HOST OVERWRITE SANITIZATION"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::cout
        << Console::BOLD_YELLOW
        << "\n  Request        : "
        << requestBinding.requestId
        << "\n  Workstation    : "
        << requestBinding.workstationId
        << "\n  Authorized     : "
        << requestBinding.serialNumber
        << "\n  Actual target  : "
        << selectedDevice.getSerialNumber()
        << "\n  Method         : HOST_OVERWRITE\n"
        << Console::RESET;

    SanitizationPipeline pipeline;

    const SanitizationPipelineResult result =
        pipeline.execute(
            selectedDevice,
            requestBinding.requestId,
            session.userId.empty()
                ? actorId
                : session.userId,
            requestBinding.workstationId,
            requestBinding.serialNumber);

    // =========================================================
    // STEP 13 - LOCAL SANITIZATION RESULT
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_CYAN
        << "STEP 13A - NATIVE SANITIZATION RESULT"
        << Console::RESET
        << "\n"
        << "------------------------------------------------------------\n";

    std::cout
        << "Status             : "
        << sanitizationStatusToString(
               result.sanitization.status)
        << "\nMethod             : "
        << methodToString(
               result.sanitization.method)
        << "\nOperation ID       : "
        << result.sanitization.operationId
        << "\nDevice ID          : "
        << result.sanitization.deviceId
        << "\nSerial             : "
        << result.sanitization.serialNumber
        << "\nBytes Processed    : "
        << result.sanitization.bytesProcessed
        << "\nVerification       : "
        << verificationStatusToString(
               result.sanitization.verificationStatus)
        << "\nVerification Done  : "
        << (result.sanitization.verificationPerformed
                ? "YES"
                : "NO")
        << "\nSamples            : "
        << result.sanitization.verificationSamples
        << "\nBytes Verified     : "
        << result.sanitization.bytesVerified
        << "\nVerification Msg   : "
        << result.sanitization.verificationMessage
        << "\nError              : "
        << result.sanitization.errorMessage
        << "\nPipeline Message   : "
        << result.pipelineMessage
        << '\n';

    if (!result.sanitization.isSuccess())
    {
        Console::fail(
            "Sanitization did not reach a verified-success state. Certificate publication is blocked.");
        return 13;
    }

    Console::pass(
        "Native Host Overwrite completed and sampled read-back verification passed.");

    // =========================================================
    // STEP 13B - LOCAL CERTIFICATE / AUDIT VALIDATION
    // =========================================================

    if (
        !validateCertificate(
            result,
            selectedDevice,
            requestBinding.requestId,
            requestBinding.workstationId))
    {
        Console::fail(
            "Native certificate evidence validation failed. Web publication is blocked.");
        return 14;
    }

    CertificateVerifier certificateVerifier;

    const CertificateVerificationResult certificateVerification =
        certificateVerifier.verify(
            result.certificatePath);

    if (!certificateVerification.valid)
    {
        Console::fail(
            "Native certificate SHA-256 verification failed.");

        std::cout
            << "\nStored Hash:\n  "
            << certificateVerification.storedHash
            << "\n\nCalculated Hash:\n  "
            << certificateVerification.calculatedHash
            << '\n';

        return 14;
    }

    Console::pass(
        "Native certificate SHA-256 verified before web publication.");

    if (!validateAuditTrail(result))
    {
        Console::fail(
            "Required native audit events are missing. Web publication is blocked.");
        return 14;
    }

    AuditChainVerifier auditVerifier;

    const AuditChainVerificationResult auditVerification =
        auditVerifier.verify(
            result.auditLogPath);

    if (!auditVerification.valid)
    {
        Console::fail(
            "Native audit hash chain is invalid. Web publication is blocked.");
        return 14;
    }

    Console::pass(
        "Native audit SHA-256 hash chain verified before web publication.");

    // =========================================================
    // STEP 14 - SANITIZATION RESULT UPLOAD
    // =========================================================

    if (!submitSanitizationResultToWeb(
            baseUrl,
            session,
            requestBinding.requestId,
            requestBinding.workstationId,
            result))
    {
        return 15;
    }

    // =========================================================
    // STEP 15 - CERTIFICATE UPLOAD + REMOTE VERIFY
    // =========================================================

    if (!uploadCertificateToWeb(
            baseUrl,
            session,
            requestBinding.requestId,
            requestBinding.workstationId,
            result))
    {
        return 16;
    }

    // =========================================================
    // STEP 16 - AUDIT CHAIN UPLOAD + REMOTE VERIFY
    // =========================================================

    if (!uploadAuditChainToWeb(
            baseUrl,
            session,
            requestBinding.requestId,
            requestBinding.workstationId,
            result))
    {
        return 17;
    }

    // =========================================================
    // STEP 17 - FINAL SERVER STATE
    // =========================================================

    if (!verifyFinalWebRequestState(
            baseUrl,
            session,
            requestBinding,
            result))
    {
        return 18;
    }

    // =========================================================
    // FINAL SUMMARY
    // =========================================================

    separator();

    std::cout
        << Console::BOLD_GREEN
        << "+==================================================================+\n"
        << "|             FORENWIPE WEB-BOUND E2E TEST PASSED                |\n"
        << "+==================================================================+"
        << Console::RESET
        << "\n\n";

    std::cout
        << Console::GREEN
        << "  Authentication        : PASS\n"
        << "  Exact Request Binding : PASS\n"
        << "  Workstation Identity  : PASS\n"
        << "  Physical Target Match : PASS\n"
        << "  System Disk Block     : PASS\n"
        << "  Safety Validation     : PASS\n"
        << "  Host Overwrite        : PASS\n"
        << "  Post-write Verify     : PASS\n"
        << "  Native Certificate    : PASS\n"
        << "  Native Cert SHA-256   : PASS\n"
        << "  Native Audit Chain    : PASS\n"
        << "  Web Result Upload     : PASS\n"
        << "  Web Certificate       : PASS\n"
        << "  Web Certificate Verify: PASS\n"
        << "  Web Audit Upload      : PASS\n"
        << "  Web Audit Verify      : PASS\n"
        << "  Final Request State   : COMPLETED\n"
        << Console::RESET
        << '\n';

    separator();

    std::cout
        << "Request ID              : "
        << requestBinding.requestId
        << "\nWorkstation ID          : "
        << requestBinding.workstationId
        << "\nAuthorized Serial       : "
        << requestBinding.serialNumber
        << "\nActual Sanitized Serial: "
        << result.sanitization.serialNumber
        << "\nOperation ID            : "
        << result.sanitization.operationId
        << "\nCertificate ID          : "
        << result.certificate.certificateId
        << "\nCertificate File        : "
        << result.certificatePath
        << "\nAudit Log               : "
        << result.auditLogPath
        << "\nCertificate SHA-256     : "
        << result.certificate.certificateHash
        << "\n\nWeb Certificate Verify  : "
        << baseUrl
        << "/api/sanitization-certificates/"
        << result.certificate.certificateId
        << "/verify"
        << "\nWeb Audit Verify        : "
        << baseUrl
        << "/api/sanitization-audit/"
        << requestBinding.requestId
        << "/verify"
        << '\n';

    separator();

    return 0;
}
