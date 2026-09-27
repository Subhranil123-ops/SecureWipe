#include "EvidenceCollector.h"
#include "StorageDevice.h"
#include "WindowsStorageDiscovery.h"

#include <Windows.h>
#include <Lmcons.h>
#include <WinHttp.h>
#include <bcrypt.h>
#include <conio.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")

#ifndef SECUREWIPE_API_BASE_URL
#define SECUREWIPE_API_BASE_URL "https://securewipe-kuo0.onrender.com"
#endif

namespace
{

constexpr const char *APP_NAME = "FORENWIPE";
constexpr const char *API_BASE_URL_DEFAULT = SECUREWIPE_API_BASE_URL;
constexpr std::uint64_t MAX_UPLOAD_BYTES = 5ULL * 1024ULL * 1024ULL;

namespace Console
{
    constexpr const char *RESET = "\x1b[0m";
    constexpr const char *BOLD = "\x1b[1m";
    constexpr const char *CYAN = "\x1b[36m";
    constexpr const char *BLUE = "\x1b[34m";
    constexpr const char *GREEN = "\x1b[32m";
    constexpr const char *YELLOW = "\x1b[33m";
    constexpr const char *RED = "\x1b[31m";
    constexpr const char *MAGENTA = "\x1b[35m";
    constexpr const char *WHITE = "\x1b[37m";
    constexpr const char *BOLD_CYAN = "\x1b[1;36m";
    constexpr const char *BOLD_BLUE = "\x1b[1;34m";
    constexpr const char *BOLD_MAGENTA = "\x1b[1;35m";
    constexpr const char *BOLD_GREEN = "\x1b[1;32m";
    constexpr const char *BOLD_YELLOW = "\x1b[1;33m";
    constexpr const char *BOLD_RED = "\x1b[1;31m";

    void enable()
    {
        HANDLE handle =
            GetStdHandle(STD_OUTPUT_HANDLE);

        if (handle == INVALID_HANDLE_VALUE)
        {
            return;
        }

        DWORD mode = 0;

        if (!GetConsoleMode(handle, &mode))
        {
            return;
        }

        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(handle, mode);
    }

    void title(const std::string &value)
    {
        std::cout
            << BOLD_CYAN
            << "\n+==================================================================+\n"
            << "|  "
            << value;

        const std::size_t width = 62;
        const std::size_t current =
            std::min<std::size_t>(value.size(), width);

        if (current < width)
        {
            std::cout
                << std::string(
                       width - current,
                       ' ');
        }

        std::cout
            << "|\n"
            << "+==================================================================+\n"
            << RESET;
    }

    void section(const std::string &value)
    {
        std::cout
            << "\n"
            << BOLD_CYAN
            << "+- "
            << value
            << " "
            << std::string(
                   value.size() < 54
                       ? 54 - value.size()
                       : 1,
                   '-')
            << "+"
            << RESET
            << '\n';
    }

    void pass(const std::string &value)
    {
        std::cout
            << GREEN
            << "  [OK] "
            << RESET
            << value
            << '\n';
    }

    void fail(const std::string &value)
    {
        std::cout
            << RED
            << "  [FAIL] "
            << RESET
            << value
            << '\n';
    }

    void warn(const std::string &value)
    {
        std::cout
            << YELLOW
            << "  [!] "
            << RESET
            << value
            << '\n';
    }

    void info(const std::string &value)
    {
        std::cout
            << CYAN
            << "  [i] "
            << RESET
            << value
            << '\n';
    }

    void step(int number, const std::string &value)
    {
        std::cout
            << BOLD_BLUE
            << "  "
            << std::setw(2)
            << std::setfill('0')
            << number
            << RESET
            << "  "
            << BOLD
            << value
            << RESET
            << '\n';
    }
}

std::string readLine()
{
    std::string value;
    std::getline(std::cin, value);
    return value;
}

std::string trim(const std::string &value)
{
    const auto first =
        value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
    {
        return {};
    }

    const auto last =
        value.find_last_not_of(" \t\r\n");

    return value.substr(
        first,
        last - first + 1);
}

std::string toUpper(std::string value)
{
    for (char &character : value)
    {
        if (character >= 'a' && character <= 'z')
        {
            character =
                static_cast<char>(
                    character - 'a' + 'A');
        }
    }

    return value;
}

bool parseIndex(
    const std::string &input,
    std::size_t count,
    std::size_t &index)
{
    try
    {
        std::size_t consumed = 0;

        const unsigned long long value =
            std::stoull(
                trim(input),
                &consumed);

        const std::string cleaned = trim(input);

        if (
            consumed != cleaned.size() ||
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

bool getCurrentWindowsUser(std::string &user)
{
    char buffer[UNLEN + 1] = {};
    DWORD size = UNLEN + 1;

    if (!GetUserNameA(buffer, &size))
    {
        return false;
    }

    user.assign(buffer, size > 0 ? size - 1 : 0);
    return !user.empty();
}

void printDevice(
    const StorageDevice &device,
    std::size_t number)
{
    std::cout
        << "\n"
        << Console::MAGENTA
        << "  [DEVICE "
        << number
        << "]"
        << Console::RESET
        << '\n'

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

    if (device.isSystemDisk())
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
        << "\n"
        << "      Removable   : "
        << (device.isRemovable() ? "YES" : "NO")
        << '\n';
}

bool readFileBytes(
    const std::string &path,
    std::vector<std::uint8_t> &bytes)
{
    std::ifstream input(
        path,
        std::ios::binary);

    if (!input)
    {
        return false;
    }

    input.seekg(
        0,
        std::ios::end);

    const std::streamoff size =
        input.tellg();

    if (size < 0)
    {
        return false;
    }

    input.seekg(
        0,
        std::ios::beg);

    bytes.resize(
        static_cast<std::size_t>(size));

    if (!bytes.empty())
    {
        input.read(
            reinterpret_cast<char *>(
                bytes.data()),
            static_cast<std::streamsize>(
                bytes.size()));
    }

    return static_cast<bool>(input) || input.eof();
}

std::string sha256(
    const std::uint8_t *data,
    std::size_t length)
{
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;

    DWORD objectLength = 0;
    DWORD resultLength = 0;

    if (
        BCryptOpenAlgorithmProvider(
            &algorithm,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            0) != 0)
    {
        return {};
    }

    if (
        BCryptGetProperty(
            algorithm,
            BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&objectLength),
            sizeof(objectLength),
            &resultLength,
            0) != 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithm,
            0);
        return {};
    }

    std::vector<UCHAR> hashObject(objectLength);
    std::array<UCHAR, 32> digest{};

    if (
        BCryptCreateHash(
            algorithm,
            &hash,
            hashObject.data(),
            objectLength,
            nullptr,
            0,
            0) != 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithm,
            0);
        return {};
    }

    if (
        length > 0 &&
        BCryptHashData(
            hash,
            const_cast<PUCHAR>(
                reinterpret_cast<const UCHAR *>(data)),
            static_cast<ULONG>(length),
            0) != 0)
    {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(
            algorithm,
            0);
        return {};
    }

    if (
        BCryptFinishHash(
            hash,
            digest.data(),
            static_cast<ULONG>(digest.size()),
            0) != 0)
    {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(
            algorithm,
            0);
        return {};
    }

    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(
        algorithm,
        0);

    std::ostringstream output;
    output << std::hex << std::setfill('0');

    for (UCHAR byte : digest)
    {
        output
            << std::setw(2)
            << static_cast<unsigned int>(byte);
    }

    return output.str();
}

std::string sha256String(
    const std::string &value)
{
    return sha256(
        reinterpret_cast<const std::uint8_t *>(
            value.data()),
        value.size());
}

std::string sha256File(
    const std::string &path)
{
    std::ifstream input(
        path,
        std::ios::binary);

    if (!input)
    {
        return {};
    }

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;

    DWORD objectLength = 0;
    DWORD resultLength = 0;

    if (
        BCryptOpenAlgorithmProvider(
            &algorithm,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            0) != 0)
    {
        return {};
    }

    if (
        BCryptGetProperty(
            algorithm,
            BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(&objectLength),
            sizeof(objectLength),
            &resultLength,
            0) != 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithm,
            0);
        return {};
    }

    std::vector<UCHAR> hashObject(objectLength);
    std::array<UCHAR, 32> digest{};

    if (
        BCryptCreateHash(
            algorithm,
            &hash,
            hashObject.data(),
            objectLength,
            nullptr,
            0,
            0) != 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithm,
            0);
        return {};
    }

    std::vector<char> buffer(
        1024 * 1024);

    while (input)
    {
        input.read(
            buffer.data(),
            static_cast<std::streamsize>(
                buffer.size()));

        const std::streamsize count =
            input.gcount();

        if (count <= 0)
        {
            break;
        }

        if (
            BCryptHashData(
                hash,
                reinterpret_cast<PUCHAR>(
                    buffer.data()),
                static_cast<ULONG>(count),
                0) != 0)
        {
            BCryptDestroyHash(hash);
            BCryptCloseAlgorithmProvider(
                algorithm,
                0);
            return {};
        }
    }

    if (
        BCryptFinishHash(
            hash,
            digest.data(),
            static_cast<ULONG>(digest.size()),
            0) != 0)
    {
        BCryptDestroyHash(hash);
        BCryptCloseAlgorithmProvider(
            algorithm,
            0);
        return {};
    }

    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(
        algorithm,
        0);

    std::ostringstream output;
    output << std::hex << std::setfill('0');

    for (UCHAR byte : digest)
        {
        output
            << std::setw(2)
            << static_cast<unsigned int>(byte);
    }

    return output.str();
}

std::string base64Encode(
    const std::vector<std::uint8_t> &data)
{
    static constexpr char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string output;
    output.reserve(
        ((data.size() + 2) / 3) * 4);

    for (std::size_t index = 0;
         index < data.size();
         index += 3)
    {
        const std::uint32_t a =
            data[index];

        const std::uint32_t b =
            index + 1 < data.size()
                ? data[index + 1]
                : 0;

        const std::uint32_t c =
            index + 2 < data.size()
                ? data[index + 2]
                : 0;

        const std::uint32_t triple =
            (a << 16) |
            (b << 8) |
            c;

        output.push_back(
            table[(triple >> 18) & 0x3F]);

        output.push_back(
            table[(triple >> 12) & 0x3F]);

        output.push_back(
            index + 1 < data.size()
                ? table[(triple >> 6) & 0x3F]
                : '=');

        output.push_back(
            index + 2 < data.size()
                ? table[triple & 0x3F]
                : '=');
    }

    return output;
}

std::string jsonEscape(
    const std::string &value)
{
    std::ostringstream output;

    for (unsigned char character : value)
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
                    << "\\u"
                    << std::hex
                    << std::setw(4)
                    << std::setfill('0')
                    << static_cast<unsigned int>(character)
                    << std::dec;
            }
            else
            {
                output
                    << static_cast<char>(character);
            }
        }
    }

    return output.str();
}

std::string canonicalValue(
    const std::string &value)
{
    std::string normalized = value;

    for (char &character : normalized)
    {
        if (
            character == '\r' ||
            character == '\n' ||
            character == '\0')
        {
            character = ' ';
        }
    }

    return normalized;
}

std::string utcTimestamp()
{
    const auto now =
        std::chrono::system_clock::now();

    const std::time_t time =
        std::chrono::system_clock::to_time_t(now);

    std::tm utc{};
    gmtime_s(&utc, &time);

    std::ostringstream output;

    output
        << std::put_time(
               &utc,
               "%Y-%m-%dT%H:%M:%SZ");

    return output.str();
}

std::string createRunId()
{
    const auto now =
        std::chrono::system_clock::now();

    const auto milliseconds =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
                now.time_since_epoch())
            .count();

    const DWORD processId =
        GetCurrentProcessId();

    return
        "FRUN-" +
        std::to_string(milliseconds) +
        "-" +
        std::to_string(processId);
}

struct NativeAuditEvent
{
    int sequence = 0;
    std::string eventType;
    std::string artifactId;
    std::string timestampUtc;
    std::string details;
    std::string previousEventHash;
    std::string eventHash;
};

std::string canonicalAuditEvent(
    const std::string &caseId,
    const std::string &runId,
    const std::string &workstationId,
    const std::string &sourceIdentifier,
    const NativeAuditEvent &event)
{
    std::ostringstream output;

    output
        << "schemaVersion=1\n"
        << "caseId="
        << canonicalValue(caseId)
        << "\n"
        << "runId="
        << canonicalValue(runId)
        << "\n"
        << "workstationId="
        << canonicalValue(workstationId)
        << "\n"
        << "sourceIdentifier="
        << canonicalValue(sourceIdentifier)
        << "\n"
        << "sequence="
        << event.sequence
        << "\n"
        << "eventType="
        << canonicalValue(event.eventType)
        << "\n"
        << "artifactId="
        << canonicalValue(event.artifactId)
        << "\n"
        << "timestampUtc="
        << canonicalValue(event.timestampUtc)
        << "\n"
        << "details="
        << canonicalValue(event.details)
        << "\n"
        << "previousEventHash="
        << canonicalValue(event.previousEventHash)
        << "\n";

    return output.str();
}

NativeAuditEvent appendAuditEvent(
    std::vector<NativeAuditEvent> &events,
    const std::string &caseId,
    const std::string &runId,
    const std::string &workstationId,
    const std::string &sourceIdentifier,
    const std::string &eventType,
    const std::string &artifactId,
    const std::string &details)
{
    NativeAuditEvent event;

    event.sequence =
        static_cast<int>(events.size() + 1);

    event.eventType =
        eventType;

    event.artifactId =
        artifactId;

    event.timestampUtc =
        utcTimestamp();

    event.details =
        details;

    event.previousEventHash =
        events.empty()
            ? ""
            : events.back().eventHash;

    event.eventHash =
        sha256String(
            canonicalAuditEvent(
                caseId,
                runId,
                workstationId,
                sourceIdentifier,
                event));

    events.push_back(event);
    return event;
}

struct CertificateArtifact
{
    std::string artifactId;
    std::string fileName;
    std::string fileType;
    std::uint64_t offset = 0;
    std::uint64_t size = 0;
    int confidenceScore = 0;
    std::string confidenceLevel;
    std::string sha256;
    bool validated = false;
    bool headerValid = false;
    bool footerValid = false;
    bool structureValid = false;
    bool sizeValid = false;
    bool decodable = false;
};

struct NativeCertificate
{
    std::string certificateId;
    std::string runId;
    std::string caseId;
    std::string workstationId;
    std::string sourceIdentifier;
    std::string sourceName;
    std::string model;
    std::string serialNumber;
    std::uint64_t capacityBytes = 0;
    std::string interfaceType;

    std::uint64_t bytesScanned = 0;
    std::uint64_t totalBytes = 0;
    std::uint64_t candidatesFound = 0;
    std::uint64_t recoveredArtifacts = 0;
    std::uint64_t validatedArtifacts = 0;
    std::uint64_t rejectedArtifacts = 0;
    std::uint64_t highConfidenceArtifacts = 0;
    std::uint64_t recoveredBytes = 0;

    std::string auditAnchorHash;
    std::string generatedAt;
    std::string hashAlgorithm = "SHA-256";

    std::vector<CertificateArtifact> artifacts;
    std::string certificateHash;
};

std::string boolString(bool value)
{
    return value ? "true" : "false";
}

std::string canonicalCertificate(
    const NativeCertificate &certificate)
{
    std::vector<CertificateArtifact> artifacts =
        certificate.artifacts;

    std::sort(
        artifacts.begin(),
        artifacts.end(),
        [](const auto &left, const auto &right)
        {
            return left.artifactId < right.artifactId;
        });

    std::ostringstream output;

    output
        << "schemaVersion=1\n"
        << "certificateId="
        << canonicalValue(certificate.certificateId)
        << "\n"
        << "runId="
        << canonicalValue(certificate.runId)
        << "\n"
        << "caseId="
        << canonicalValue(certificate.caseId)
        << "\n"
        << "workstationId="
        << canonicalValue(certificate.workstationId)
        << "\n"
        << "sourceIdentifier="
        << canonicalValue(certificate.sourceIdentifier)
        << "\n"
        << "sourceName="
        << canonicalValue(certificate.sourceName)
        << "\n"
        << "model="
        << canonicalValue(certificate.model)
        << "\n"
        << "serialNumber="
        << canonicalValue(certificate.serialNumber)
        << "\n"
        << "capacityBytes="
        << certificate.capacityBytes
        << "\n"
        << "interfaceType="
        << canonicalValue(certificate.interfaceType)
        << "\n"
        << "bytesScanned="
        << certificate.bytesScanned
        << "\n"
        << "totalBytes="
        << certificate.totalBytes
        << "\n"
        << "candidatesFound="
        << certificate.candidatesFound
        << "\n"
        << "recoveredArtifacts="
        << certificate.recoveredArtifacts
        << "\n"
        << "validatedArtifacts="
        << certificate.validatedArtifacts
        << "\n"
        << "rejectedArtifacts="
        << certificate.rejectedArtifacts
        << "\n"
        << "highConfidenceArtifacts="
        << certificate.highConfidenceArtifacts
        << "\n"
        << "recoveredBytes="
        << certificate.recoveredBytes
        << "\n"
        << "auditAnchorHash="
        << canonicalValue(certificate.auditAnchorHash)
        << "\n"
        << "generatedAt="
        << canonicalValue(certificate.generatedAt)
        << "\n"
        << "hashAlgorithm="
        << canonicalValue(certificate.hashAlgorithm)
        << "\n"
        << "artifactCount="
        << artifacts.size()
        << "\n";

    for (const auto &artifact : artifacts)
    {
        output
            << "artifact.artifactId="
            << canonicalValue(artifact.artifactId)
            << "\n"
            << "artifact.fileName="
            << canonicalValue(artifact.fileName)
            << "\n"
            << "artifact.fileType="
            << canonicalValue(artifact.fileType)
            << "\n"
            << "artifact.offset="
            << artifact.offset
            << "\n"
            << "artifact.size="
            << artifact.size
            << "\n"
            << "artifact.confidenceScore="
            << artifact.confidenceScore
            << "\n"
            << "artifact.confidenceLevel="
            << canonicalValue(artifact.confidenceLevel)
            << "\n"
            << "artifact.sha256="
            << canonicalValue(artifact.sha256)
            << "\n"
            << "artifact.validated="
            << boolString(artifact.validated)
            << "\n"
            << "artifact.headerValid="
            << boolString(artifact.headerValid)
            << "\n"
            << "artifact.footerValid="
            << boolString(artifact.footerValid)
            << "\n"
            << "artifact.structureValid="
            << boolString(artifact.structureValid)
            << "\n"
            << "artifact.sizeValid="
            << boolString(artifact.sizeValid)
            << "\n"
            << "artifact.decodable="
            << boolString(artifact.decodable)
            << "\n";
    }

    return output.str();
}

std::string randomCertificateId()
{
    static std::uint64_t counter = 0;

    const auto now =
        std::chrono::system_clock::now();

    const auto milliseconds =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
                now.time_since_epoch())
            .count();

    ++counter;

    return
        "FEC-" +
        std::to_string(milliseconds) +
        "-" +
        std::to_string(counter);
}

std::string artifactJson(
    const EvidenceItem &item,
    const std::string &contentBase64)
{
    std::ostringstream output;

    output
        << "{"
        << "\"artifactId\":\""
        << jsonEscape(item.artifactId)
        << "\","

        << "\"fileName\":\""
        << jsonEscape(item.fileName)
        << "\","

        << "\"fileType\":\""
        << jsonEscape(item.fileType)
        << "\","

        << "\"offset\":"
        << item.offset
        << ","

        << "\"size\":"
        << item.size
        << ","

        << "\"recoveredPath\":\""
        << jsonEscape(item.recoveredPath)
        << "\","

        << "\"headerValid\":"
        << boolString(item.headerValid)
        << ","

        << "\"footerValid\":"
        << boolString(item.footerValid)
        << ","

        << "\"structureValid\":"
        << boolString(item.structureValid)
        << ","

        << "\"sizeValid\":"
        << boolString(item.sizeValid)
        << ","

        << "\"decodable\":"
        << boolString(item.decodable)
        << ","

        << "\"confidenceScore\":"
        << item.confidenceScore
        << ","

        << "\"confidenceLevel\":\""
        << jsonEscape(item.getConfidenceString())
        << "\","

        << "\"confidenceReasons\":[";

    for (std::size_t index = 0;
         index < item.confidenceReasons.size();
         ++index)
    {
        if (index > 0)
        {
            output << ',';
        }

        output
            << "\""
            << jsonEscape(
                   item.confidenceReasons[index])
            << "\"";
    }

    output
        << "],"

        << "\"sha256\":\""
        << jsonEscape(item.sha256)
        << "\","

        << "\"recovered\":"
        << boolString(item.recovered)
        << ","

        << "\"validated\":"
        << boolString(item.validated)
        << ","

        << "\"contentBase64\":\""
        << contentBase64
        << "\""

        << "}";

    return output.str();
}

std::string certificateArtifactJson(
    const CertificateArtifact &artifact)
{
    std::ostringstream output;
        output
        << "{"
        << "\"artifactId\":\""
        << jsonEscape(artifact.artifactId)
        << "\","
        << "\"fileName\":\""
        << jsonEscape(artifact.fileName)
        << "\","
        << "\"fileType\":\""
        << jsonEscape(artifact.fileType)
        << "\","
        << "\"offset\":"
        << artifact.offset
        << ","
        << "\"size\":"
        << artifact.size
        << ","
        << "\"confidenceScore\":"
        << artifact.confidenceScore
        << ","
        << "\"confidenceLevel\":\""
        << jsonEscape(artifact.confidenceLevel)
        << "\","
        << "\"sha256\":\""
        << jsonEscape(artifact.sha256)
        << "\","
        << "\"validated\":"
        << boolString(artifact.validated)
        << ","
        << "\"headerValid\":"
        << boolString(artifact.headerValid)
        << ","
        << "\"footerValid\":"
        << boolString(artifact.footerValid)
        << ","
        << "\"structureValid\":"
        << boolString(artifact.structureValid)
        << ","
        << "\"sizeValid\":"
        << boolString(artifact.sizeValid)
        << ","
        << "\"decodable\":"
        << boolString(artifact.decodable)
        << "}";

    return output.str();
}

std::string certificateJson(
    const NativeCertificate &certificate)
{
    std::ostringstream output;

    output
        << "{"
        << "\"certificateId\":\""
        << jsonEscape(certificate.certificateId)
        << "\","
        << "\"runId\":\""
        << jsonEscape(certificate.runId)
        << "\","
        << "\"caseId\":\""
        << jsonEscape(certificate.caseId)
        << "\","
        << "\"workstationId\":\""
        << jsonEscape(certificate.workstationId)
        << "\","
        << "\"sourceIdentifier\":\""
        << jsonEscape(certificate.sourceIdentifier)
        << "\","
        << "\"sourceName\":\""
        << jsonEscape(certificate.sourceName)
        << "\","
        << "\"model\":\""
        << jsonEscape(certificate.model)
        << "\","
        << "\"serialNumber\":\""
        << jsonEscape(certificate.serialNumber)
        << "\","
        << "\"capacityBytes\":"
        << certificate.capacityBytes
        << ","
        << "\"interfaceType\":\""
        << jsonEscape(certificate.interfaceType)
        << "\","
        << "\"bytesScanned\":"
        << certificate.bytesScanned
        << ","
        << "\"totalBytes\":"
        << certificate.totalBytes
        << ","
        << "\"candidatesFound\":"
        << certificate.candidatesFound
        << ","
        << "\"recoveredArtifacts\":"
        << certificate.recoveredArtifacts
        << ","
        << "\"validatedArtifacts\":"
        << certificate.validatedArtifacts
        << ","
        << "\"rejectedArtifacts\":"
        << certificate.rejectedArtifacts
        << ","
        << "\"highConfidenceArtifacts\":"
        << certificate.highConfidenceArtifacts
        << ","
        << "\"recoveredBytes\":"
        << certificate.recoveredBytes
        << ","
        << "\"auditAnchorHash\":\""
        << jsonEscape(certificate.auditAnchorHash)
        << "\","
        << "\"generatedAt\":\""
        << jsonEscape(certificate.generatedAt)
        << "\","
        << "\"hashAlgorithm\":\""
        << jsonEscape(certificate.hashAlgorithm)
        << "\","
        << "\"artifactCount\":"
        << certificate.artifacts.size()
        << ","
        << "\"artifacts\":[";

    for (std::size_t index = 0;
         index < certificate.artifacts.size();
         ++index)
    {
        if (index > 0)
        {
            output << ',';
        }

        output
            << certificateArtifactJson(
                   certificate.artifacts[index]);
    }

    output
        << "],"
        << "\"certificateHash\":\""
        << jsonEscape(certificate.certificateHash)
        << "\""
        << "}";

    return output.str();
}

std::string auditEventJson(
    const NativeAuditEvent &event)
{
    std::ostringstream output;

    output
        << "{"
        << "\"sequence\":"
        << event.sequence
        << ","
        << "\"eventType\":\""
        << jsonEscape(event.eventType)
        << "\","
        << "\"artifactId\":\""
        << jsonEscape(event.artifactId)
        << "\","
        << "\"timestampUtc\":\""
        << jsonEscape(event.timestampUtc)
        << "\","
        << "\"details\":\""
        << jsonEscape(event.details)
        << "\","
        << "\"previousEventHash\":\""
        << jsonEscape(event.previousEventHash)
        << "\","
        << "\"eventHash\":\""
        << jsonEscape(event.eventHash)
        << "\""
        << "}";

    return output.str();
}

std::string auditJson(
    const std::vector<NativeAuditEvent> &events)
{
    std::ostringstream output;

    output
        << "{"
        << "\"hashAlgorithm\":\"SHA-256\","
        << "\"events\":[";

    for (std::size_t index = 0;
         index < events.size();
         ++index)
    {
        if (index > 0)
        {
            output << ',';
        }

        output
            << auditEventJson(events[index]);
    }

    output
        << "]"
        << "}";

    return output.str();
}

struct HttpResponse
{
    bool transportOk = false;
    DWORD statusCode = 0;
    std::string body;
    std::string error;
};

std::wstring widenAscii(
    const std::string &value)
{
    if (value.empty())
    {
        return {};
    }

    const int required =
        MultiByteToWideChar(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0);

    if (required <= 0)
    {
        return {};
    }

    std::wstring output(required, L'\0');

    MultiByteToWideChar(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        output.data(),
        required);

    return output;
}

std::string narrowUtf8(
    const std::wstring &value)
{
    if (value.empty())
    {
        return {};
    }

    const int required =
        WideCharToMultiByte(
            CP_UTF8,
            0,
            value.data(),
            static_cast<int>(value.size()),
            nullptr,
            0,
            nullptr,
            nullptr);

    if (required <= 0)
    {
        return {};
    }

    std::string output(
        required,
        '\0');

    WideCharToMultiByte(
        CP_UTF8,
        0,
        value.data(),
        static_cast<int>(value.size()),
        output.data(),
        required,
        nullptr,
        nullptr);

    return output;
}

bool crackUrl(
    const std::string &url,
    std::wstring &host,
    std::wstring &path,
    INTERNET_PORT &port,
    bool &secure,
    std::string &error)
{
    const std::wstring wide =
        widenAscii(url);

    if (wide.empty())
    {
        error = "API URL is empty or could not be converted.";
        return false;
    }

    URL_COMPONENTS components{};
    components.dwStructSize =
        sizeof(components);

    wchar_t hostBuffer[256] = {};
    wchar_t pathBuffer[4096] = {};
    wchar_t extraBuffer[2048] = {};

    components.lpszHostName =
        hostBuffer;
    components.dwHostNameLength =
        static_cast<DWORD>(std::size(hostBuffer));

    components.lpszUrlPath =
        pathBuffer;
    components.dwUrlPathLength =
        static_cast<DWORD>(std::size(pathBuffer));

    components.lpszExtraInfo =
        extraBuffer;
    components.dwExtraInfoLength =
        static_cast<DWORD>(std::size(extraBuffer));

    if (
        !WinHttpCrackUrl(
            wide.c_str(),
            static_cast<DWORD>(wide.size()),
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

    port = components.nPort;
    secure =
        components.nScheme == INTERNET_SCHEME_HTTPS;

    return true;
}

HttpResponse sendHttpRequest(
    const std::string &method,
    const std::string &url,
    const std::string &token,
    const std::string &body = {})
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
            L"ForenWipe-Forensic-E2E/1.0",
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
        15000,
        15000,
        60000,
        60000);

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

    BOOL sent =
        WinHttpSendRequest(
            request,
            headers.c_str(),
            static_cast<DWORD>(-1L),
            body.empty()
                ? WINHTTP_NO_REQUEST_DATA
                : reinterpret_cast<LPVOID>(
                      const_cast<char *>(
                          body.data())),
            body.empty()
                ? 0
                : static_cast<DWORD>(body.size()),
            body.empty()
                ? 0
                : static_cast<DWORD>(body.size()),
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

    if (!WinHttpReceiveResponse(request, nullptr))
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
    DWORD statusSize =
        sizeof(statusCode);

    if (
        !WinHttpQueryHeaders(
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
    const HttpResponse &response,
    const std::string &operation)
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
std::string apiBaseUrl()
{
    const char *overrideValue =
        std::getenv(
            "SECUREWIPE_API_BASE_URL_OVERRIDE");

    if (
        overrideValue &&
        *overrideValue)
    {
        return trim(overrideValue);
    }

    return API_BASE_URL_DEFAULT;
}

std::string readSecret(
    const std::string &label)
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

        if (character == '\r' || character == '\n')
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

        if (character >= 32 && character <= 126)
        {
            value.push_back(
                static_cast<char>(character));
        }
    }

    std::cout << "\n";
    return trim(value);
}

struct AuthSession
{
    std::string baseUrl;
    std::string token;
    std::string email;
    std::string password;
    std::string userId;
    std::string name;
    std::string role;
};

std::string readEnvironmentValue(
    const char *name)
{
    const char *environment =
        std::getenv(name);

    if (
        environment &&
        *environment)
    {
        return trim(environment);
    }

    return {};
}

bool extractJsonStringField(
    const std::string &json,
    const std::string &key,
    std::string &value,
    std::size_t searchFrom = 0);

bool extractJsonObjectField(
    const std::string &json,
    const std::string &key,
    std::string &objectJson,
    std::size_t searchFrom = 0);

bool refreshWebSession(
    const std::string &baseUrl,
    AuthSession &session)
{
    if (session.email.empty() || session.password.empty())
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
            "",
            body.str());

    if (!response.transportOk ||
        response.statusCode < 200 ||
        response.statusCode >= 300)
    {
        Console::fail(
            "Automatic JWT refresh login failed (HTTP " +
            std::to_string(response.statusCode) +
            ").");
        return false;
    }

    std::string token;

    if (!extractJsonStringField(
            response.body,
            "token",
            token) ||
        token.empty())
    {
        Console::fail(
            "Automatic JWT refresh response did not contain a usable token.");
        return false;
    }

    session.token = token;

    std::string userObject;

    if (extractJsonObjectField(
            response.body,
            "user",
            userObject))
    {
        extractJsonStringField(userObject, "name", session.name);
        extractJsonStringField(userObject, "role", session.role);
        extractJsonStringField(userObject, "email", session.email);
        extractJsonStringField(userObject, "id", session.userId);

        if (session.userId.empty())
        {
            extractJsonStringField(userObject, "_id", session.userId);
        }
    }

    if (session.role != "WORKSTATION_EMPLOYEE" &&
        session.role != "ADMIN")
    {
        Console::fail(
            "Automatic JWT refresh returned an account without a desktop-operator role.");
        return false;
    }

    Console::pass(
        "Fresh 15-minute JWT obtained automatically after token expiry.");

    return true;
}

HttpResponse sendAuthenticatedHttpRequest(
    const std::string &method,
    const std::string &url,
    AuthSession &session,
    const std::string &body = {})
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

    Console::warn(
        "The access token expired during the long-running forensic operation. Refreshing authentication and retrying the same request once.");

    if (!refreshWebSession(session.baseUrl, session))
    {
        return response;
    }

    return sendHttpRequest(
        method,
        url,
        session.token,
        body);
}

bool loginAndCreateSession(
    const std::string &baseUrl,
    AuthSession &session)
{
    Console::section(
        "DESKTOP AUTHENTICATION");

    std::string email =
        readEnvironmentValue(
            "FORENWIPE_DESKTOP_EMAIL");

    std::string password =
        readEnvironmentValue(
            "FORENWIPE_DESKTOP_PASSWORD");

    if (email.empty())
    {
        std::cout
            << "  Workstation email : ";
        email =
            trim(readLine());
    }
    else
    {
        std::cout
            << "  Workstation email : "
            << email
            << " (environment)\n";
    }

    if (email.empty())
    {
        Console::fail(
            "Workstation account email is required.");
        return false;
    }

    if (password.empty())
    {
        password =
            readSecret("Password");
    }
    else
    {
        std::cout
            << "  Password           : (environment)\n";
    }

    if (password.empty())
    {
        Console::fail(
            "Workstation account password is required.");
        return false;
    }

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
            baseUrl +
                "/api/auth/login",
            "",
            body.str());

    if (!expectHttpSuccess(
            response,
            "Desktop authentication"))
    {
        return false;
    }

    std::string token;

    if (!extractJsonStringField(
            response.body,
            "token",
            token))
    {
        Console::fail(
            "Login response did not contain an authentication token.");
        return false;
    }

    if (token.empty())
    {
        Console::fail(
            "Login returned an empty authentication token.");
        return false;
    }

    std::string userObject;
    std::string role;
    std::string userName;

    if (extractJsonObjectField(
            response.body,
            "user",
            userObject))
    {
        extractJsonStringField(
            userObject,
            "name",
            userName);

        extractJsonStringField(
            userObject,
            "role",
            role);

        extractJsonStringField(
            userObject,
            "email",
            session.email);

        extractJsonStringField(
            userObject,
            "id",
            session.userId);

        if (session.userId.empty())
        {
            extractJsonStringField(
                userObject,
                "_id",
                session.userId);
        }
    }

    if (
        role != "WORKSTATION_EMPLOYEE" &&
        role != "ADMIN")
    {
        Console::fail(
            "The logged-in account does not have a desktop-operator role."
            " Received role: " +
            (role.empty() ? "(unknown)" : role));
        return false;
    }

    Console::pass(
        "Backend authentication successful.");

    std::cout
        << "  Authenticated user : "
        << (userName.empty() ? email : userName)
        << '\n'
        << "  Role                : "
        << (role.empty() ? "(not returned)" : role)
        << '\n';

    session.token = token;
    session.baseUrl = baseUrl;
    session.email = email;
    session.password = password;
    session.name = userName;
    session.role = role;
    return true;
}

struct ForensicCaseBinding
{
    std::string caseId;
    std::string status;
    std::string sourceType;
    std::string sourceIdentifier;
    std::string sourceName;

    std::string assignedEmployeeId;
    std::string assignedEmployeeName;

    std::string workstationMongoId;
    std::string workstationCode;
    std::string workstationName;
    std::string workstationStatus;
};

std::size_t findJsonStringEnd(
    const std::string &json,
    std::size_t quotePosition)
{
    bool escaped = false;

    for (
        std::size_t index = quotePosition + 1;
        index < json.size();
        ++index)
    {
        const char character = json[index];

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

std::string decodeJsonString(
    const std::string &value)
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
    const std::string &json,
    const std::string &key,
    std::size_t searchFrom = 0)
{
    const std::string needle =
        "\"" + key + "\"";

    return json.find(needle, searchFrom);
}

bool extractJsonStringField(
    const std::string &json,
    const std::string &key,
    std::string &value,
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
        json.find(':', keyPosition + key.size() + 2);

    if (colonPosition == std::string::npos)
    {
        return false;
    }

    std::size_t valuePosition = colonPosition + 1;

    while (
        valuePosition < json.size() &&
        (json[valuePosition] == ' ' ||
         json[valuePosition] == '\t' ||
         json[valuePosition] == '\r' ||
         json[valuePosition] == '\n'))
    {
        ++valuePosition;
    }

    if (valuePosition >= json.size() || json[valuePosition] != '"')
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

bool extractJsonObjectField(
    const std::string &json,
    const std::string &key,
    std::string &objectJson,
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
        json.find(':', keyPosition + key.size() + 2);

    if (colonPosition == std::string::npos)
    {
        return false;
    }

    std::size_t valuePosition = colonPosition + 1;

    while (
        valuePosition < json.size() &&
        (json[valuePosition] == ' ' ||
         json[valuePosition] == '\t' ||
         json[valuePosition] == '\r' ||
         json[valuePosition] == '\n'))
    {
        ++valuePosition;
    }

    if (valuePosition >= json.size() || json[valuePosition] != '{')
    {
        return false;
    }

    int depth = 0;
    bool insideString = false;
    bool escaped = false;

    for (
        std::size_t index = valuePosition;
        index < json.size();
        ++index)
    {
        const char character = json[index];

        if (insideString)
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
                insideString = false;
            }

            continue;
        }

        if (character == '"')
        {
            insideString = true;
            continue;
        }

        if (character == '{')
        {
            ++depth;
        }
        else if (character == '}')
        {
            --depth;

            if (depth == 0)
            {
                objectJson =
                    json.substr(
                        valuePosition,
                        index - valuePosition + 1);

                return true;
            }
        }
    }

    return false;
}

bool loadForensicCaseBinding(
    const std::string &baseUrl,
    const std::string &caseId,
    AuthSession &session,
    ForensicCaseBinding &binding)
{
    const HttpResponse response =
        sendAuthenticatedHttpRequest(
            "GET",
            baseUrl +
                "/api/forensics/" +
                caseId,
            session);

    if (!expectHttpSuccess(
            response,
            "Forensic case lookup"))
    {
        return false;
    }

    if (response.body.empty())
    {
        Console::fail(
            "Forensic case lookup returned an empty response.");
        return false;
    }

    binding.caseId = caseId;

    extractJsonStringField(
        response.body,
        "caseId",
        binding.caseId);

    if (binding.caseId.empty())
    {
        binding.caseId = caseId;
    }

    if (!extractJsonStringField(
            response.body,
            "status",
            binding.status))
    {
        Console::fail(
            "Forensic case response does not contain a status.");
        return false;
    }

    extractJsonStringField(
        response.body,
        "sourceType",
        binding.sourceType);

    extractJsonStringField(
        response.body,
        "sourceIdentifier",
        binding.sourceIdentifier);

    extractJsonStringField(
        response.body,
        "sourceName",
        binding.sourceName);

    std::string workstationObject;

    if (!extractJsonObjectField(
            response.body,
            "assignedWorkstation",
            workstationObject))
    {
        Console::fail(
            "This forensic case has no assigned workstation.");
        return false;
    }

    extractJsonStringField(
        workstationObject,
        "_id",
        binding.workstationMongoId);

    extractJsonStringField(
        workstationObject,
        "workstationId",
        binding.workstationCode);

    extractJsonStringField(
        workstationObject,
        "name",
        binding.workstationName);

    extractJsonStringField(
        workstationObject,
        "status",
        binding.workstationStatus);

    if (binding.workstationMongoId.empty())
    {
        Console::fail(
            "Assigned workstation does not expose its database ID.");
        return false;
    }

    if (binding.workstationCode.empty())
    {
        Console::fail(
            "Assigned workstation does not expose its workstation ID.");
        return false;
    }

    std::string employeeObject;

    if (extractJsonObjectField(
            response.body,
            "assignedEmployee",
            employeeObject))
    {
        extractJsonStringField(
            employeeObject,
            "_id",
            binding.assignedEmployeeId);

        extractJsonStringField(
            employeeObject,
            "name",
            binding.assignedEmployeeName);
    }

    if (binding.sourceType.empty())
    {
        binding.sourceType = "PHYSICAL_DEVICE";
    }

    return true;
}

bool sourceIdentifierMatchesDevice(
    const std::string &caseIdentifier,
    const StorageDevice &device)
{
    auto normalizeIdentifier = [](std::string value)
    {
        value = trim(value);

        std::transform(
            value.begin(),
            value.end(),
            value.begin(),
            [](unsigned char character)
            {
                return static_cast<char>(
                    std::tolower(character));
            });

        return value;
    };

    const std::string normalizedCaseIdentifier =
        normalizeIdentifier(caseIdentifier);

    if (normalizedCaseIdentifier.empty())
    {
        return false;
    }

    return
        normalizedCaseIdentifier ==
            normalizeIdentifier(device.getSerialNumber()) ||
        normalizedCaseIdentifier ==
            normalizeIdentifier(device.getDeviceId());
}

bool updateCaseStatus(
    const std::string &baseUrl,
    const std::string &caseId,
    const std::string &workstationId,
    AuthSession &session,
    const std::string &status,
    const std::string &note)
{
    std::ostringstream body;

    body
        << "{"
        << "\"status\":\""
        << jsonEscape(status)
        << "\","
        << "\"note\":\""
        << jsonEscape(note)
        << "\","
        << "\"workstationId\":\""
        << jsonEscape(workstationId)
        << "\""
        << "}";

    const std::string url =
        baseUrl +
        "/api/forensics/" +
        caseId +
        "/status";

    const HttpResponse response =
        sendAuthenticatedHttpRequest(
            "PATCH",
            url,
            session,
            body.str());

    return expectHttpSuccess(
        response,
        "Case status update to " + status);
}

bool uploadEvidencePackage(
    const std::string &baseUrl,
    const std::string &caseId,
    AuthSession &session,
    const std::string &workstationId,
    const std::string &sourceIdentifier,
    const std::string &runId,
    const StorageDevice &device,
    const EvidenceCollectionSummary &summary,
    const std::vector<EvidenceItem> &evidence,
    const std::vector<NativeAuditEvent> &auditEvents,
    const NativeCertificate &certificate)
{
    std::ostringstream body;

    body
        << "{"
        << "\"schemaVersion\":1,"
        << "\"runId\":\""
        << jsonEscape(runId)
        << "\","
        << "\"workstationId\":\""
        << jsonEscape(workstationId)
        << "\","
        << "\"sourceType\":\"PHYSICAL_DEVICE\","
        << "\"sourceIdentifier\":\""
        << jsonEscape(sourceIdentifier)
        << "\","
        << "\"status\":\"COMPLETED\","
        << "\"source\":{"
        << "\"deviceId\":\""
        << jsonEscape(device.getDeviceId())
        << "\","
        << "\"model\":\""
        << jsonEscape(device.getModel())
        << "\","
        << "\"serialNumber\":\""
        << jsonEscape(device.getSerialNumber())
        << "\","
        << "\"capacityBytes\":"
        << device.getCapacityBytes()
        << ","
        << "\"interfaceType\":\""
        << jsonEscape(device.getInterfaceType())
        << "\""
        << "},"
        << "\"summary\":{"
        << "\"sourceOpened\":"
        << boolString(summary.sourceOpened)
        << ","
        << "\"completed\":"
        << boolString(summary.completed)
        << ","
        << "\"bytesScanned\":"
        << summary.bytesScanned
        << ","
        << "\"totalBytes\":"
        << summary.totalBytes
        << ","
        << "\"candidatesFound\":"
        << summary.candidatesFound
        << ","
        << "\"recoveredArtifacts\":"
        << summary.recoveredArtifacts
        << ","
        << "\"validatedArtifacts\":"
        << summary.validatedArtifacts
        << ","
        << "\"rejectedArtifacts\":"
        << summary.rejectedArtifacts
        << ","
        << "\"highConfidenceArtifacts\":"
        << summary.highConfidenceArtifacts
        << ","
        << "\"recoveredBytes\":"
        << summary.recoveredBytes
        << "},"
        << "\"artifacts\":[";

    for (std::size_t index = 0;
         index < evidence.size();
         ++index)
    {
        if (index > 0)
        {
            body << ',';
        }

        std::vector<std::uint8_t> bytes;

        if (!readFileBytes(
                evidence[index].recoveredPath,
                bytes))
        {
            return false;
        }

        body
            << artifactJson(
                   evidence[index],
                   base64Encode(bytes));
    }

    body
        << "],"
        << "\"nativeAudit\":"
        << auditJson(auditEvents)
        << ","
        << "\"certificate\":"
        << certificateJson(certificate)
        << ""
        << "}";

    if (
        body.str().size() >
        32ULL * 1024ULL * 1024ULL)
    {
        Console::fail(
            "Evidence package exceeds the configured 32 MiB JSON transport limit.");
        return false;
    }

    const HttpResponse response =
        sendAuthenticatedHttpRequest(
            "POST",
            baseUrl +
                "/api/forensics/" +
                caseId +
                "/evidence-package",
            session,
            body.str());

    return expectHttpSuccess(
        response,
        "Forensic evidence package upload");
}

void printArtifact(
    const EvidenceItem &item,
    std::size_t number)
{
    std::cout
        << "\n"
        << Console::BOLD_MAGENTA
        << "  +- ARTIFACT "
        << std::setw(2)
        << std::setfill('0')
        << number
        << " ---------------------------------------------+"
        << Console::RESET
        << '\n'

        << "  | ID         : "
        << item.artifactId
        << '\n'

        << "  | File       : "
        << item.fileName
        << '\n'

        << "  | Type       : "
        << item.fileType
        << '\n'

        << "  | Offset     : 0x"
        << std::hex
        << std::uppercase
        << item.offset
        << std::dec
        << '\n'

        << "  | Size       : "
        << item.size
        << " bytes\n"

        << "  | Validation : "
        << (item.validated ? "VALIDATED" : "REJECTED")
        << '\n'

        << "  | Confidence : "
        << item.getConfidenceString()
        << " ("
        << item.confidenceScore
        << "%)\n"

        << "  | SHA-256    : "
        << item.sha256
        << '\n'

        << "  | Path       : "
        << item.recoveredPath
        << '\n'

        << "  +--------------------------------------------------------+\n";

    for (const auto &reason : item.confidenceReasons)
    {
        std::cout
            << "      "
            << Console::BLUE
            << "- "
            << Console::RESET
            << reason
            << '\n';
    }
}

bool buildCertificateAndAudit(
    const std::string &caseId,
    const std::string &workstationId,
    const std::string &sourceIdentifier,
    const std::string &runId,
    const StorageDevice &device,
    const EvidenceCollectionSummary &summary,
    const std::vector<EvidenceItem> &evidence,
    std::vector<NativeAuditEvent> &auditEvents,
    NativeCertificate &certificate)
{
    appendAuditEvent(
        auditEvents,
        caseId,
        runId,
        workstationId,
        sourceIdentifier,
        "ACQUISITION_STARTED",
        "",
        "sourceModel=" + device.getModel() +
            ";sourceSerial=" + device.getSerialNumber() +
            ";sourceCapacityBytes=" +
            std::to_string(device.getCapacityBytes()) +
            ";sourceInterface=" +
            device.getInterfaceType());

    appendAuditEvent(
        auditEvents,
        caseId,
        runId,
        workstationId,
        sourceIdentifier,
        "SCAN_COMPLETED",
        "",
        "bytesScanned=" +
            std::to_string(summary.bytesScanned) +
            ";totalBytes=" +
            std::to_string(summary.totalBytes) +
            ";candidatesFound=" +
            std::to_string(summary.candidatesFound) +
            ";recoveredArtifacts=" +
            std::to_string(summary.recoveredArtifacts) +
            ";validatedArtifacts=" +
            std::to_string(summary.validatedArtifacts) +
            ";rejectedArtifacts=" +
            std::to_string(summary.rejectedArtifacts));

    for (const auto &item : evidence)
    {
        appendAuditEvent(
            auditEvents,
            caseId,
            runId,
            workstationId,
            sourceIdentifier,
            "ARTIFACT_HASHED",
            item.artifactId,
            "fileType=" + item.fileType +
                ";size=" +
                std::to_string(item.size) +
                ";offset=" +
                std::to_string(item.offset) +
                ";sha256=" +
                item.sha256 +
                ";validated=" +
                boolString(item.validated) +
                ";confidence=" +
                item.getConfidenceString());
    }

    appendAuditEvent(
        auditEvents,
        caseId,
        runId,
        workstationId,
        sourceIdentifier,
        "ACQUISITION_SUMMARY",
        "",
        "highConfidenceArtifacts=" +
            std::to_string(summary.highConfidenceArtifacts) +
            ";recoveredBytes=" +
            std::to_string(summary.recoveredBytes));

    certificate.certificateId =
        randomCertificateId();

    certificate.runId =
        runId;

    certificate.caseId =
        caseId;

    certificate.workstationId =
        workstationId;

    certificate.sourceIdentifier =
        sourceIdentifier;

    certificate.sourceName =
        device.getDeviceId();

    certificate.model =
        device.getModel();

    certificate.serialNumber =
        device.getSerialNumber();

    certificate.capacityBytes =
        device.getCapacityBytes();

    certificate.interfaceType =
        device.getInterfaceType();

    certificate.bytesScanned =
        summary.bytesScanned;

    certificate.totalBytes =
        summary.totalBytes;

    certificate.candidatesFound =
        summary.candidatesFound;

    certificate.recoveredArtifacts =
        summary.recoveredArtifacts;

    certificate.validatedArtifacts =
        summary.validatedArtifacts;

    certificate.rejectedArtifacts =
        summary.rejectedArtifacts;

    certificate.highConfidenceArtifacts =
        summary.highConfidenceArtifacts;

    certificate.recoveredBytes =
        summary.recoveredBytes;

    certificate.auditAnchorHash =
        auditEvents.back().eventHash;

    certificate.generatedAt =
        utcTimestamp();

    certificate.artifacts.clear();

    for (const auto &item : evidence)
    {
        CertificateArtifact artifact;

        artifact.artifactId =
            item.artifactId;

        artifact.fileName =
            item.fileName;

        artifact.fileType =
            item.fileType;

        artifact.offset =
            item.offset;

        artifact.size =
            item.size;

        artifact.confidenceScore =
            item.confidenceScore;

        artifact.confidenceLevel =
            item.getConfidenceString();

        artifact.sha256 =
            item.sha256;

        artifact.validated =
            item.validated;

        artifact.headerValid =
            item.headerValid;

        artifact.footerValid =
            item.footerValid;

        artifact.structureValid =
            item.structureValid;

        artifact.sizeValid =
            item.sizeValid;

        artifact.decodable =
            item.decodable;

        certificate.artifacts.push_back(
            artifact);
    }

    certificate.certificateHash =
        sha256String(
            canonicalCertificate(
                certificate));

    appendAuditEvent(
        auditEvents,
        caseId,
        runId,
        workstationId,
        sourceIdentifier,
        "CERTIFICATE_GENERATED",
        "",
        "certificateId=" +
            certificate.certificateId +
            ";certificateHash=" +
            certificate.certificateHash +
            ";auditAnchorHash=" +
            certificate.auditAnchorHash);

    appendAuditEvent(
        auditEvents,
        caseId,
        runId,
        workstationId,
        sourceIdentifier,
        "ACQUISITION_COMPLETED",
        "",
        "certificateHash=" +
            certificate.certificateHash +
            ";completionMarker=1");

    return
        !certificate.certificateHash.empty() &&
        certificate.certificateHash.size() == 64 &&
        auditEvents.size() >= 6;
}

bool verifyLocalAuditChain(
    const std::string &caseId,
    const std::string &workstationId,
    const std::string &sourceIdentifier,
    const std::string &runId,
    const std::vector<NativeAuditEvent> &events)
{
    std::string previousHash;

    for (const auto &event : events)
    {
        const std::string expected =
            sha256String(
                canonicalAuditEvent(
                    caseId,
                    runId,
                    workstationId,
                    sourceIdentifier,
                    event));

        if (expected != event.eventHash)
                {
            return false;
        }

        if (event.previousEventHash != previousHash)
        {
            return false;
        }

        previousHash =
            event.eventHash;
    }

    return !events.empty();
}

bool verifyLocalCertificate(
    const NativeCertificate &certificate)
{
    const std::string calculated =
        sha256String(
            canonicalCertificate(
                certificate));

    return
        calculated ==
        certificate.certificateHash;
}

bool writeEvidenceManifest(
    const std::filesystem::path &directory,
    const std::string &caseId,
    const std::string &runId,
    const std::vector<NativeAuditEvent> &events,
    const NativeCertificate &certificate)
{
    std::error_code error;

    std::filesystem::create_directories(
        directory,
        error);

    if (error)
    {
        return false;
    }

    const auto manifestPath =
        directory /
        (runId + "_evidence_manifest.json");

    std::ofstream output(
        manifestPath,
        std::ios::binary);

    if (!output)
    {
        return false;
    }

    output
        << "{\n"
        << "  \"schemaVersion\": 1,\n"
        << "  \"caseId\": \""
        << jsonEscape(caseId)
        << "\",\n"
        << "  \"runId\": \""
        << jsonEscape(runId)
        << "\",\n"
        << "  \"certificate\": "
        << certificateJson(certificate)
        << ",\n"
        << "  \"nativeAudit\": "
        << auditJson(events)
        << "\n"
        << "}\n";

    return static_cast<bool>(output);
}

bool confirmReadOnlyAcquisition(
    const StorageDevice &device)
{
    Console::section(
        "READ-ONLY ACQUISITION AUTHORIZATION");

    std::cout
        << "  Source       : "
        << device.getDeviceId()
        << '\n'

        << "  Serial       : "
        << device.getSerialNumber()
        << '\n'

        << "  Mode         : READ ONLY\n"
        << "  Writes       : BLOCKED BY FORENSIC COLLECTOR\n\n"

        << Console::BOLD_YELLOW
        << "  Type exactly: START FORENSIC READ\n"
        << Console::RESET
        << "  Confirmation: ";

    return
        trim(readLine()) ==
        "START FORENSIC READ";
}

} // namespace

int main()
{
    Console::enable();

    Console::title(
        "FORENWIPE // FORENSIC ACQUISITION CONSOLE");

    std::cout
        << Console::CYAN
        << "  Native C++ engine  |  Physical device acquisition  |  Read-only mode"
        << Console::RESET
        << "\n"
        << "  Evidence -> SHA-256 -> Native Audit Chain -> Certificate -> Web\n";

    Console::section(
        "SESSION INITIALIZATION");

    std::string actorId;

    if (!getCurrentWindowsUser(actorId))
    {
        Console::fail(
            "Could not determine the Windows actor.");
        return 1;
    }

    const std::string baseUrl =
        apiBaseUrl();

    const std::string runId =
        createRunId();

    std::cout
        << "  Actor          : "
        << actorId
        << '\n'
        << "  API            : "
        << baseUrl
        << '\n'
        << "  Run ID         : "
        << runId
        << '\n';

    if (baseUrl.empty())
    {
        Console::fail(
            "SecureWipe API base URL is empty.");
        return 1;
    }

    Console::section(
        "WEB CASE BINDING");

    std::cout
        << "  Forensic Case ID : ";

    const std::string caseId =
        trim(readLine());

    if (caseId.empty())
    {
        Console::fail(
            "Case ID is required.");
        return 1;
    }

    AuthSession session;

    if (!loginAndCreateSession(
            baseUrl,
            session))
    {
        return 1;
    }

    ForensicCaseBinding caseBinding;

    if (!loadForensicCaseBinding(
            baseUrl,
            caseId,
            session,
            caseBinding))
    {
        return 1;
    }

    if (caseBinding.status != "ASSIGNED")
    {
        Console::fail(
            "Case is not ready for workstation acquisition. Current status: " +
            caseBinding.status);
        return 1;
    }

    if (caseBinding.workstationStatus != "" &&
        caseBinding.workstationStatus != "ACTIVE")
    {
        Console::fail(
            "Assigned workstation is not ACTIVE.");
        return 1;
    }

    if (caseBinding.sourceType != "PHYSICAL_DEVICE")
    {
        Console::fail(
            "This E2E test requires a PHYSICAL_DEVICE forensic case.");
        return 1;
    }

    if (caseBinding.sourceIdentifier.empty())
    {
        Console::fail(
            "The forensic case has no physical source identifier.");
        return 1;
    }

    Console::pass(
        "Authenticated forensic case loaded from web.");

    std::cout
        << "  Case Status          : "
        << caseBinding.status
        << '\n'
        << "  Assigned Employee   : "
        << (caseBinding.assignedEmployeeName.empty()
                ? "(not populated)"
                : caseBinding.assignedEmployeeName)
        << '\n'
        << "  Workstation ID      : "
        << caseBinding.workstationCode
        << '\n'
        << "  Workstation DB ID   : "
        << caseBinding.workstationMongoId
        << '\n'
        << "  Workstation Name    : "
        << (caseBinding.workstationName.empty()
                ? "(not populated)"
                : caseBinding.workstationName)
        << '\n'
        << "  Case Source         : "
        << caseBinding.sourceIdentifier
        << '\n';

    Console::info(
        "Workstation binding was obtained automatically from the assigned web case.");

    Console::section(
        "01 // REAL DEVICE DISCOVERY");

    WindowsStorageDiscovery discovery;

    const std::vector<StorageDevice> devices =
        discovery.discover();

    if (devices.empty())
    {
        Console::fail(
            "No physical storage devices were discovered.");
        return 2;
    }

    Console::pass(
        "Discovered " +
        std::to_string(devices.size()) +
        " physical device(s).");

    for (std::size_t index = 0;
         index < devices.size();
         ++index)
    {
        printDevice(
            devices[index],
            index + 1);
    }

    Console::section(
        "02 // AUTO-MATCH CASE SOURCE");

    std::size_t matchedIndex =
        devices.size();

    for (std::size_t index = 0;
         index < devices.size();
         ++index)
    {
        if (sourceIdentifierMatchesDevice(
                caseBinding.sourceIdentifier,
                devices[index]))
        {
            if (matchedIndex != devices.size())
            {
                Console::fail(
                    "More than one physical device matches the source identifier stored in this case.");
                return 3;
            }

            matchedIndex = index;
        }
    }

    if (matchedIndex == devices.size())
    {
        Console::fail(
            "No discovered physical device matches the source identifier stored in the forensic case.");

        std::cout
            << "  Case source identifier : "
            << caseBinding.sourceIdentifier
            << '\n';

        for (std::size_t index = 0;
             index < devices.size();
             ++index)
        {
            std::cout
                << "  Discovered device "
                << (index + 1)
                << " serial="
                << devices[index].getSerialNumber()
                << " path="
                << devices[index].getDeviceId()
                << '\n';
        }

        return 3;
    }

    const StorageDevice selectedDevice =
        devices[matchedIndex];

    Console::pass(
        "Case source automatically matched to the discovered physical device.");

    printDevice(
        selectedDevice,
        matchedIndex + 1);

    if (selectedDevice.isSystemDisk())
    {
        Console::fail(
            "Selected device is the Windows system disk. Forensic acquisition is blocked in this E2E test.");
        return 4;
    }

    if (selectedDevice.getDeviceId().empty())
    {
        Console::fail(
            "Selected device does not expose a physical device path.");
        return 4;
    }

    if (selectedDevice.getSerialNumber().empty())
    {
        Console::fail(
            "Selected physical device has no usable serial number.");
        return 4;
    }

    Console::pass(
        "Physical target is not the system disk.");

    Console::pass(
        "Physical device identity is available.");

    if (!sourceIdentifierMatchesDevice(
            caseBinding.sourceIdentifier,
            selectedDevice))
    {
        Console::fail(
            "Selected physical device does not match the source identifier stored in the forensic case.");

        std::cout
            << "  Case source identifier : "
            << caseBinding.sourceIdentifier
            << '\n'
            << "  Selected serial        : "
            << selectedDevice.getSerialNumber()
            << '\n'
            << "  Selected device path   : "
            << selectedDevice.getDeviceId()
            << '\n';

        return 4;
    }

    Console::pass(
        "Selected physical device matches the web case source binding.");

    if (!confirmReadOnlyAcquisition(selectedDevice))
    {
        Console::warn(
            "Read-only acquisition was not authorized. Nothing was written.");
        return 5;
    }

    Console::pass(
        "Read-only authorization accepted.");

    Console::section(
        "04 // CASE ACQUISITION START");

    if (!updateCaseStatus(
            baseUrl,
            caseId,
            caseBinding.workstationMongoId,
            session,
            "ACQUIRING",
            "Native ForenWipe forensic E2E acquisition started"))
    {
        return 6;
    }

    Console::pass(
        "Server case moved to ACQUIRING.");

    Console::section(
        "05 // RAW READ + JPEG CARVING");

    EvidenceCollector collector;

    const EvidenceCollectionResult result =
        collector.collectWithSummary(
            selectedDevice.getDeviceId());

    const EvidenceCollectionSummary &summary =
        result.summary;

    if (!summary.sourceOpened)
    {
        Console::fail(
            "EvidenceCollector could not open the physical source.");
        return 7;
    }

    if (!summary.completed)
    {
        Console::fail(
            "Physical-device acquisition did not complete.");
        return 8;
    }

    Console::pass(
        "Physical source opened in read-only mode.");

    Console::pass(
        "Chunk-by-chunk acquisition completed.");

    Console::section(
        "06 // ACQUISITION TELEMETRY");

    std::cout
        << "  Total bytes         : "
        << summary.totalBytes
        << '\n'
        << "  Bytes scanned       : "
        << summary.bytesScanned
        << '\n'
        << "  Candidates found    : "
        << summary.candidatesFound
        << '\n'
        << "  Recovered artifacts : "
        << summary.recoveredArtifacts
        << '\n'
        << "  Validated artifacts : "
        << summary.validatedArtifacts
        << '\n'
        << "  Rejected artifacts  : "
        << summary.rejectedArtifacts
        << '\n'
        << "  High confidence     : "
        << summary.highConfidenceArtifacts
        << '\n'
        << "  Recovered bytes     : "
        << summary.recoveredBytes
        << '\n';

    if (result.evidence.empty())
    {
        Console::warn(
            "No validated evidence artifacts were recovered. The read-only acquisition completed, but there is nothing to upload as image evidence.");

        updateCaseStatus(
            baseUrl,
            caseId,
            caseBinding.workstationMongoId,
            session,
            "ANALYZING",
            "Native forensic scan completed without validated artifacts");

        updateCaseStatus(
            baseUrl,
            caseId,
            caseBinding.workstationMongoId,
            session,
            "FAILED",
            "No validated forensic artifacts were recovered by the native acquisition engine");

        return 9;
    }

    Console::section(
        "07 // RECOVERED EVIDENCE");

    for (std::size_t index = 0;
         index < result.evidence.size();
         ++index)
    {
        printArtifact(
            result.evidence[index],
            index + 1);
    }

    Console::section(
        "08 // NATIVE CRYPTOGRAPHIC MANIFEST");

    std::vector<NativeAuditEvent> auditEvents;
    NativeCertificate certificate;

    if (!buildCertificateAndAudit(
            caseId,
            caseBinding.workstationCode,
            caseBinding.sourceIdentifier,
            runId,
            selectedDevice,
            summary,
            result.evidence,
            auditEvents,
            certificate))
    {
        Console::fail(
            "Could not generate native forensic certificate/audit evidence.");
        return 10;
    }

    if (!verifyLocalAuditChain(
            caseId,
            caseBinding.workstationCode,
            caseBinding.sourceIdentifier,
            runId,
            auditEvents))
    {
        Console::fail(
            "Local native audit-chain verification failed.");
        return 11;
    }

    if (!verifyLocalCertificate(certificate))
    {
        Console::fail(
            "Local forensic certificate SHA-256 verification failed.");
        return 12;
    }

    Console::pass(
        "Native SHA-256 audit chain verified locally.");

    Console::pass(
        "Native forensic certificate hash verified locally.");

    std::cout
        << "\n  Certificate ID       : "
        << certificate.certificateId
        << '\n'
        << "  Certificate SHA-256 : "
        << certificate.certificateHash
        << '\n'
        << "  Audit anchor hash   : "
        << certificate.auditAnchorHash
        << '\n'
        << "  Audit events        : "
        << auditEvents.size()
        << '\n'
        << "  Chain head hash     : "
        << auditEvents.back().eventHash
        << '\n';

    Console::section(
        "09 // WEB UPLOAD PREFLIGHT");

    std::uint64_t totalArtifactBytes = 0;

    for (const auto &item : result.evidence)
    {
        std::error_code error;

        const auto size =
            std::filesystem::file_size(
                item.recoveredPath,
                error);

        if (error)
        {
            Console::fail(
                "Could not determine size of " +
                item.recoveredPath);
            return 13;
        }

        totalArtifactBytes +=
            static_cast<std::uint64_t>(size);
    }

    std::cout
        << "  Total artifact bytes : "
        << totalArtifactBytes
        << '\n'
        << "  Transport threshold  : "
        << MAX_UPLOAD_BYTES
        << " bytes\n";

    if (totalArtifactBytes > MAX_UPLOAD_BYTES)
    {
        Console::fail(
            "Recovered artifacts exceed the safe demo upload threshold. Export locally and use a larger transport/storage profile before uploading.");
        return 14;
    }

    Console::pass(
        "Recovered artifact set is within the E2E upload threshold.");

    Console::section(
        "10 // AUTHENTICATED EVIDENCE UPLOAD");

    Console::info(
        "Uploading recovered image bytes, evidence metadata, native audit chain and certificate...");

    if (!uploadEvidencePackage(
            baseUrl,
            caseId,
            session,
            caseBinding.workstationMongoId,
            caseBinding.sourceIdentifier,
            runId,
            selectedDevice,
            summary,
            result.evidence,
            auditEvents,
            certificate))
    {
        return 15;
    }

    Console::pass(
        "Evidence package accepted by the server.");

    Console::section(
        "11 // SERVER-SIDE INTEGRITY COMPLETION");

    Console::pass(
        "Server accepted the forensic result and completed the case lifecycle.");

    Console::section(
        "12 // LOCAL EVIDENCE MANIFEST");

    const std::filesystem::path manifestDirectory =
        std::filesystem::current_path() /
        "forensic_evidence";

    if (
        writeEvidenceManifest(
            manifestDirectory,
            caseId,
            runId,
            auditEvents,
            certificate))
    {
        Console::pass(
            "Local forensic evidence manifest exported.");

        std::cout
            << "  Manifest directory : "
            << manifestDirectory.string()
            << '\n';
    }
    else
    {
        Console::warn(
            "Local manifest export failed; web evidence upload already succeeded.");
    }

    Console::section(
        "13 // END-TO-END VERIFICATION SUMMARY");

    std::cout
        << Console::BOLD_GREEN
        << "\n  +==============================================================+\n"
        << "  |              FORENSIC E2E PIPELINE COMPLETE               |\n"
        << "  +==============================================================+\n"
        << Console::RESET
        << '\n';

    Console::pass(
        "Physical device discovered");
    Console::pass(
        "Read-only acquisition completed");
    Console::pass(
        "JPEG artifact recovered and validated");
    Console::pass(
        "Artifact SHA-256 calculated");
    Console::pass(
        "Native SHA-256 audit chain verified");
    Console::pass(
        "Forensic certificate SHA-256 verified");
    Console::pass(
        "Actual recovered image bytes uploaded");
    Console::pass(
        "Server independently verified the evidence package");
    Console::pass(
        "Evidence is now available from the web case");

    std::cout
        << "\n  Case ID              : "
        << caseId
        << '\n'
        << "  Workstation ID      : "
        << caseBinding.workstationCode
        << '\n'
        << "  Run ID              : "
        << runId
        << '\n'
        << "  Certificate ID      : "
        << certificate.certificateId
        << '\n'
        << "  Certificate SHA-256 : "
        << certificate.certificateHash
        << '\n'
        << "  Native chain head   : "
        << auditEvents.back().eventHash
        << '\n';

    std::cout
        << "\n"
        << Console::BOLD_CYAN
        << "  Open the same forensic case in the web application to view:\n"
        << "    - the recovered image\n"
        << "    - artifact metadata and validation\n"
        << "    - artifact SHA-256\n"
        << "    - forensic certificate\n"
        << "    - native audit hash chain\n"
        << "    - server-side integrity verification\n"
        << Console::RESET;

    return 0;
}