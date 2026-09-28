#include "ForensicEvidencePackage.h"

#include <Windows.h>
#include <bcrypt.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <vector>

#pragma comment(lib, "bcrypt.lib")

namespace
{

std::string canonicalValue(
    const std::string& value)
{
    std::string normalized =
        value;

    for (
        char& character :
        normalized)
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

std::string jsonEscape(
    const std::string& value)
{
    std::ostringstream output;

    for (
        const unsigned char character :
        value)
    {
        switch (
            character)
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
            if (
                character < 0x20)
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

std::string utcTimestamp()
{
    const auto now =
        std::chrono::system_clock::now();

    const std::time_t timeValue =
        std::chrono::system_clock::to_time_t(
            now);

    std::tm utc{};

    gmtime_s(
        &utc,
        &timeValue);

    std::ostringstream output;

    output
        << std::put_time(
               &utc,
               "%Y-%m-%dT%H:%M:%SZ");

    return output.str();
}

std::string sha256String(
    const std::string& data)
{
    BCRYPT_ALG_HANDLE algorithmHandle =
        nullptr;

    BCRYPT_HASH_HANDLE hashHandle =
        nullptr;

    DWORD objectSize = 0;
    DWORD hashSize = 0;
    DWORD resultSize = 0;

    NTSTATUS status =
        BCryptOpenAlgorithmProvider(
            &algorithmHandle,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            0);

    if (
        status < 0)
    {
        return {};
    }

    status =
        BCryptGetProperty(
            algorithmHandle,
            BCRYPT_OBJECT_LENGTH,
            reinterpret_cast<PUCHAR>(
                &objectSize),
            sizeof(objectSize),
            &resultSize,
            0);

    if (
        status < 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        return {};
    }

    status =
        BCryptGetProperty(
            algorithmHandle,
            BCRYPT_HASH_LENGTH,
            reinterpret_cast<PUCHAR>(
                &hashSize),
            sizeof(hashSize),
            &resultSize,
            0);

    if (
        status < 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        return {};
    }

    std::vector<UCHAR>
        hashObject(
            objectSize);

    std::vector<UCHAR>
        hash(
            hashSize);

    status =
        BCryptCreateHash(
            algorithmHandle,
            &hashHandle,
            hashObject.data(),
            objectSize,
            nullptr,
            0,
            0);

    if (
        status < 0)
    {
        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        return {};
    }

    if (
        !data.empty())
    {
        status =
            BCryptHashData(
                hashHandle,
                reinterpret_cast<PUCHAR>(
                    const_cast<char*>(
                        data.data())),
                static_cast<ULONG>(
                    data.size()),
                0);

        if (
            status < 0)
        {
            BCryptDestroyHash(
                hashHandle);

            BCryptCloseAlgorithmProvider(
                algorithmHandle,
                0);

            return {};
        }
    }

    status =
        BCryptFinishHash(
            hashHandle,
            hash.data(),
            hashSize,
            0);

    if (
        status < 0)
    {
        BCryptDestroyHash(
            hashHandle);

        BCryptCloseAlgorithmProvider(
            algorithmHandle,
            0);

        return {};
    }

    std::ostringstream output;

    output
        << std::hex
        << std::setfill('0');

    for (
        const UCHAR byte :
        hash)
    {
        output
            << std::setw(2)
            << static_cast<unsigned int>(
                   byte);
    }

    BCryptDestroyHash(
        hashHandle);

    BCryptCloseAlgorithmProvider(
        algorithmHandle,
        0);

    return output.str();
}

std::string createRunIdInternal()
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
        std::to_string(
            milliseconds) +
        "-" +
        std::to_string(
            processId);
}

bool readFileBytes(
    const std::string& filePath,
    std::vector<std::uint8_t>& bytes,
    std::string& errorMessage)
{
    bytes.clear();

    std::ifstream input(
        filePath,
        std::ios::binary);

    if (!input)
    {
        errorMessage =
            "Unable to open recovered artifact: " +
            filePath;

        return false;
    }

    input.seekg(
        0,
        std::ios::end);

    const std::streamoff size =
        input.tellg();

    if (
        size < 0)
    {
        errorMessage =
            "Unable to determine recovered artifact size: " +
            filePath;

        return false;
    }

    input.seekg(
        0,
        std::ios::beg);

    bytes.resize(
        static_cast<std::size_t>(
            size));

    if (
        !bytes.empty())
    {
        input.read(
            reinterpret_cast<char*>(
                bytes.data()),
            static_cast<std::streamsize>(
                bytes.size()));

        if (
            input.gcount() !=
            static_cast<std::streamsize>(
                bytes.size()))
        {
            errorMessage =
                "Unable to read complete recovered artifact: " +
                filePath;

            bytes.clear();

            return false;
        }
    }

    return true;
}

std::string base64Encode(
    const std::vector<std::uint8_t>& bytes)
{
    static constexpr char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string output;

    output.reserve(
        ((bytes.size() + 2) / 3) * 4);

    std::size_t index = 0;

    while (
        index < bytes.size())
    {
        const std::uint32_t a =
            bytes[index++];

        const std::uint32_t b =
            index < bytes.size()
                ? bytes[index++]
                : 0;

        const std::uint32_t c =
            index < bytes.size()
                ? bytes[index++]
                : 0;

        const std::uint32_t triple =
            (a << 16) |
            (b << 8) |
            c;

        output.push_back(
            table[
                (triple >> 18) &
                0x3F]);

        output.push_back(
            table[
                (triple >> 12) &
                0x3F]);

        output.push_back(
            index - 1 < bytes.size()
                ? table[
                      (triple >> 6) &
                      0x3F]
                : '=');

        output.push_back(
            index < bytes.size() + 1
                ? table[
                      triple &
                      0x3F]
                : '=');
    }

    return output;
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
    const std::string& caseId,
    const std::string& runId,
    const std::string& workstationId,
    const std::string& sourceIdentifier,
    const NativeAuditEvent& event)
{
    std::ostringstream output;

    output
        << "schemaVersion=1\n"
        << "caseId="
        << canonicalValue(
               caseId)
        << "\n"
        << "runId="
        << canonicalValue(
               runId)
        << "\n"
        << "workstationId="
        << canonicalValue(
               workstationId)
        << "\n"
        << "sourceIdentifier="
        << canonicalValue(
               sourceIdentifier)
        << "\n"
        << "sequence="
        << event.sequence
        << "\n"
        << "eventType="
        << canonicalValue(
               event.eventType)
        << "\n"
        << "artifactId="
        << canonicalValue(
               event.artifactId)
        << "\n"
        << "timestampUtc="
        << canonicalValue(
               event.timestampUtc)
        << "\n"
        << "details="
        << canonicalValue(
               event.details)
        << "\n"
        << "previousEventHash="
        << canonicalValue(
               event.previousEventHash)
        << "\n";

    return output.str();
}

NativeAuditEvent appendAuditEvent(
    std::vector<NativeAuditEvent>& events,
    const std::string& caseId,
    const std::string& runId,
    const std::string& workstationId,
    const std::string& sourceIdentifier,
    const std::string& eventType,
    const std::string& artifactId,
    const std::string& details)
{
    NativeAuditEvent event;

    event.sequence =
        static_cast<int>(
            events.size() + 1);

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

    events.push_back(
        event);

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

    std::string hashAlgorithm =
        "SHA-256";

    std::vector<CertificateArtifact>
        artifacts;

    std::string certificateHash;
};

std::string boolString(
    bool value)
{
    return value
        ? "true"
        : "false";
}

std::string canonicalCertificate(
    const NativeCertificate& certificate)
{
    std::vector<
        CertificateArtifact>
        artifacts =
            certificate.artifacts;

    std::sort(
        artifacts.begin(),
        artifacts.end(),
        [](const auto& left,
           const auto& right)
        {
            return left.artifactId <
                   right.artifactId;
        });

    std::ostringstream output;

    output
        << "schemaVersion=1\n"
        << "certificateId="
        << canonicalValue(
               certificate.certificateId)
        << "\n"
        << "runId="
        << canonicalValue(
               certificate.runId)
        << "\n"
        << "caseId="
        << canonicalValue(
               certificate.caseId)
        << "\n"
        << "workstationId="
        << canonicalValue(
               certificate.workstationId)
        << "\n"
        << "sourceIdentifier="
        << canonicalValue(
               certificate.sourceIdentifier)
        << "\n"
        << "sourceName="
        << canonicalValue(
               certificate.sourceName)
        << "\n"
        << "model="
        << canonicalValue(
               certificate.model)
        << "\n"
        << "serialNumber="
        << canonicalValue(
               certificate.serialNumber)
        << "\n"
        << "capacityBytes="
        << certificate.capacityBytes
        << "\n"
        << "interfaceType="
        << canonicalValue(
               certificate.interfaceType)
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
        << canonicalValue(
               certificate.auditAnchorHash)
        << "\n"
        << "generatedAt="
        << canonicalValue(
               certificate.generatedAt)
        << "\n"
        << "hashAlgorithm="
        << canonicalValue(
               certificate.hashAlgorithm)
        << "\n"
        << "artifactCount="
        << artifacts.size()
        << "\n";

    for (
        const auto& artifact :
        artifacts)
    {
        output
            << "artifact.artifactId="
            << canonicalValue(
                   artifact.artifactId)
            << "\n"
            << "artifact.fileName="
            << canonicalValue(
                   artifact.fileName)
            << "\n"
            << "artifact.fileType="
            << canonicalValue(
                   artifact.fileType)
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
            << canonicalValue(
                   artifact.confidenceLevel)
            << "\n"
            << "artifact.sha256="
            << canonicalValue(
                   artifact.sha256)
            << "\n"
            << "artifact.validated="
            << boolString(
                   artifact.validated)
            << "\n"
            << "artifact.headerValid="
            << boolString(
                   artifact.headerValid)
            << "\n"
            << "artifact.footerValid="
            << boolString(
                   artifact.footerValid)
            << "\n"
            << "artifact.structureValid="
            << boolString(
                   artifact.structureValid)
            << "\n"
            << "artifact.sizeValid="
            << boolString(
                   artifact.sizeValid)
            << "\n"
            << "artifact.decodable="
            << boolString(
                   artifact.decodable)
            << "\n";
    }

    return output.str();
}

std::string randomCertificateId()
{
    static std::uint64_t counter =
        0;

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
        std::to_string(
            milliseconds) +
        "-" +
        std::to_string(
            counter);
}

std::string artifactJson(
    const EvidenceItem& item,
    const std::string& contentBase64)
{
    std::ostringstream output;

    output
        << "{"
        << "\"artifactId\":\""
        << jsonEscape(
               item.artifactId)
        << "\","
        << "\"fileName\":\""
        << jsonEscape(
               item.fileName)
        << "\","
        << "\"fileType\":\""
        << jsonEscape(
               item.fileType)
        << "\","
        << "\"offset\":"
        << item.offset
        << ","
        << "\"size\":"
        << item.size
        << ","
        << "\"recoveredPath\":\""
        << jsonEscape(
               item.recoveredPath)
        << "\","
        << "\"headerValid\":"
        << boolString(
               item.headerValid)
        << ","
        << "\"footerValid\":"
        << boolString(
               item.footerValid)
        << ","
        << "\"structureValid\":"
        << boolString(
               item.structureValid)
        << ","
        << "\"sizeValid\":"
        << boolString(
               item.sizeValid)
        << ","
        << "\"decodable\":"
        << boolString(
               item.decodable)
        << ","
        << "\"confidenceScore\":"
        << item.confidenceScore
        << ","
        << "\"confidenceLevel\":\""
        << jsonEscape(
               item.getConfidenceString())
        << "\","
        << "\"confidenceReasons\":[";

    for (
        std::size_t index = 0;
        index <
        item.confidenceReasons.size();
        ++index)
    {
        if (
            index > 0)
        {
            output
                << ',';
        }

        output
            << "\""
            << jsonEscape(
                   item.confidenceReasons[
                       index])
            << "\"";
    }

    output
        << "],"
        << "\"sha256\":\""
        << jsonEscape(
               item.sha256)
        << "\","
        << "\"recovered\":"
        << boolString(
               item.recovered)
        << ","
        << "\"validated\":"
        << boolString(
               item.validated)
        << ","
        << "\"contentBase64\":\""
        << contentBase64
        << "\""
        << "}";

    return output.str();
}

std::string certificateArtifactJson(
    const CertificateArtifact& artifact)
{
    std::ostringstream output;

    output
        << "{"
        << "\"artifactId\":\""
        << jsonEscape(
               artifact.artifactId)
        << "\","
        << "\"fileName\":\""
        << jsonEscape(
               artifact.fileName)
        << "\","
        << "\"fileType\":\""
        << jsonEscape(
               artifact.fileType)
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
        << jsonEscape(
               artifact.confidenceLevel)
        << "\","
        << "\"sha256\":\""
        << jsonEscape(
               artifact.sha256)
        << "\","
        << "\"validated\":"
        << boolString(
               artifact.validated)
        << ","
        << "\"headerValid\":"
        << boolString(
               artifact.headerValid)
        << ","
        << "\"footerValid\":"
        << boolString(
               artifact.footerValid)
        << ","
        << "\"structureValid\":"
        << boolString(
               artifact.structureValid)
        << ","
        << "\"sizeValid\":"
        << boolString(
               artifact.sizeValid)
        << ","
        << "\"decodable\":"
        << boolString(
               artifact.decodable)
        << "}";

    return output.str();
}

std::string certificateJson(
    const NativeCertificate& certificate)
{
    std::ostringstream output;

    output
        << "{"
        << "\"certificateId\":\""
        << jsonEscape(
               certificate.certificateId)
        << "\","
        << "\"runId\":\""
        << jsonEscape(
               certificate.runId)
        << "\","
        << "\"caseId\":\""
        << jsonEscape(
               certificate.caseId)
        << "\","
        << "\"workstationId\":\""
        << jsonEscape(
               certificate.workstationId)
        << "\","
        << "\"sourceIdentifier\":\""
        << jsonEscape(
               certificate.sourceIdentifier)
        << "\","
        << "\"sourceName\":\""
        << jsonEscape(
               certificate.sourceName)
        << "\","
        << "\"model\":\""
        << jsonEscape(
               certificate.model)
        << "\","
        << "\"serialNumber\":\""
        << jsonEscape(
               certificate.serialNumber)
        << "\","
        << "\"capacityBytes\":"
        << certificate.capacityBytes
        << ","
        << "\"interfaceType\":\""
        << jsonEscape(
               certificate.interfaceType)
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
        << jsonEscape(
               certificate.auditAnchorHash)
        << "\","
        << "\"generatedAt\":\""
        << jsonEscape(
               certificate.generatedAt)
        << "\","
        << "\"hashAlgorithm\":\""
        << jsonEscape(
               certificate.hashAlgorithm)
        << "\","
        << "\"artifactCount\":"
        << certificate.artifacts.size()
        << ","
        << "\"artifacts\":[";

    for (
        std::size_t index = 0;
        index <
        certificate.artifacts.size();
        ++index)
    {
        if (
            index > 0)
        {
            output
                << ',';
        }

        output
            << certificateArtifactJson(
                   certificate.artifacts[
                       index]);
    }

    output
        << "],"
        << "\"certificateHash\":\""
        << jsonEscape(
               certificate.certificateHash)
        << "\""
        << "}";

    return output.str();
}

std::string auditEventJson(
    const NativeAuditEvent& event)
{
    std::ostringstream output;

    output
        << "{"
        << "\"sequence\":"
        << event.sequence
        << ","
        << "\"eventType\":\""
        << jsonEscape(
               event.eventType)
        << "\","
        << "\"artifactId\":\""
        << jsonEscape(
               event.artifactId)
        << "\","
        << "\"timestampUtc\":\""
        << jsonEscape(
               event.timestampUtc)
        << "\","
        << "\"details\":\""
        << jsonEscape(
               event.details)
        << "\","
        << "\"previousEventHash\":\""
        << jsonEscape(
               event.previousEventHash)
        << "\","
        << "\"eventHash\":\""
        << jsonEscape(
               event.eventHash)
        << "\""
        << "}";

    return output.str();
}

std::string auditJson(
    const std::vector<NativeAuditEvent>& events)
{
    std::ostringstream output;

    output
        << "{"
        << "\"hashAlgorithm\":\"SHA-256\","
        << "\"events\":[";

    for (
        std::size_t index = 0;
        index < events.size();
        ++index)
    {
        if (
            index > 0)
        {
            output
                << ',';
        }

        output
            << auditEventJson(
                   events[index]);
    }

    output
        << "]"
        << "}";

    return output.str();
}

} // namespace

namespace SecureWipe
{
namespace ForensicIntegrity
{

std::string createRunId()
{
    return createRunIdInternal();
}

bool buildEvidencePackage(
    const std::string& caseId,
    const std::string& workstationId,
    const SourceMetadata& source,
    const std::string& runId,
    const EvidenceCollectionSummary& summary,
    const std::vector<EvidenceItem>& evidence,
    std::string& json,
    std::string& errorMessage)
{
    json.clear();
    errorMessage.clear();

    if (
        caseId.empty())
    {
        errorMessage =
            "Forensic case ID is empty.";

        return false;
    }

    if (
        workstationId.empty())
    {
        errorMessage =
            "Workstation ID is empty.";

        return false;
    }

    if (
        source.sourceIdentifier.empty())
    {
        errorMessage =
            "Forensic source identifier is empty.";

        return false;
    }

    if (
        runId.empty())
    {
        errorMessage =
            "Forensic run ID is empty.";

        return false;
    }

    std::vector<
        NativeAuditEvent>
        auditEvents;

    appendAuditEvent(
        auditEvents,
        caseId,
        runId,
        workstationId,
        source.sourceIdentifier,
        "ACQUISITION_STARTED",
        "",
        "sourceModel=" +
            source.model +
            ";sourceSerial=" +
            source.serialNumber +
            ";sourceCapacityBytes=" +
            std::to_string(
                source.capacityBytes) +
            ";sourceInterface=" +
            source.interfaceType);

    appendAuditEvent(
        auditEvents,
        caseId,
        runId,
        workstationId,
        source.sourceIdentifier,
        "SCAN_COMPLETED",
        "",
        "bytesScanned=" +
            std::to_string(
                summary.bytesScanned) +
            ";totalBytes=" +
            std::to_string(
                summary.totalBytes) +
            ";candidatesFound=" +
            std::to_string(
                summary.candidatesFound) +
            ";recoveredArtifacts=" +
            std::to_string(
                summary.recoveredArtifacts) +
            ";validatedArtifacts=" +
            std::to_string(
                summary.validatedArtifacts) +
            ";rejectedArtifacts=" +
            std::to_string(
                summary.rejectedArtifacts));

    std::ostringstream artifactsJson;

    artifactsJson
        << "[";

    std::vector<
        CertificateArtifact>
        certificateArtifacts;

    certificateArtifacts.reserve(
        evidence.size());

    for (
        std::size_t index = 0;
        index < evidence.size();
        ++index)
    {
        const EvidenceItem& item =
            evidence[index];

        if (
            item.recoveredPath.empty())
        {
            errorMessage =
                "Recovered artifact path is empty for artifact " +
                item.artifactId;

            return false;
        }

        std::vector<
            std::uint8_t>
            bytes;

        if (
            !readFileBytes(
                item.recoveredPath,
                bytes,
                errorMessage))
        {
            return false;
        }

        if (
            bytes.size() !=
            item.size)
        {
            errorMessage =
                "Recovered artifact size does not match metadata for artifact " +
                item.artifactId;

            return false;
        }

        const std::string encoded =
            base64Encode(
                bytes);

        if (
            index > 0)
        {
            artifactsJson
                << ',';
        }

        artifactsJson
            << artifactJson(
                   item,
                   encoded);

        CertificateArtifact certificateArtifact;

        certificateArtifact.artifactId =
            item.artifactId;

        certificateArtifact.fileName =
            item.fileName;

        certificateArtifact.fileType =
            item.fileType;

        certificateArtifact.offset =
            item.offset;

        certificateArtifact.size =
            item.size;

        certificateArtifact.confidenceScore =
            item.confidenceScore;

        certificateArtifact.confidenceLevel =
            item.getConfidenceString();

        certificateArtifact.sha256 =
            item.sha256;

        certificateArtifact.validated =
            item.validated;

        certificateArtifact.headerValid =
            item.headerValid;

        certificateArtifact.footerValid =
            item.footerValid;

        certificateArtifact.structureValid =
            item.structureValid;

        certificateArtifact.sizeValid =
            item.sizeValid;

        certificateArtifact.decodable =
            item.decodable;

        certificateArtifacts.push_back(
            certificateArtifact);

        appendAuditEvent(
            auditEvents,
            caseId,
            runId,
            workstationId,
            source.sourceIdentifier,
            "ARTIFACT_HASHED",
            item.artifactId,
            "fileType=" +
                item.fileType +
                ";size=" +
                std::to_string(
                    item.size) +
                ";offset=" +
                std::to_string(
                    item.offset) +
                ";sha256=" +
                item.sha256 +
                ";validated=" +
                boolString(
                    item.validated) +
                ";confidence=" +
                item.getConfidenceString());
    }

    artifactsJson
        << "]";

    appendAuditEvent(
        auditEvents,
        caseId,
        runId,
        workstationId,
        source.sourceIdentifier,
        "ACQUISITION_SUMMARY",
        "",
        "highConfidenceArtifacts=" +
            std::to_string(
                summary.highConfidenceArtifacts) +
            ";recoveredBytes=" +
            std::to_string(
                summary.recoveredBytes));

    NativeCertificate certificate;

    certificate.certificateId =
        randomCertificateId();

    certificate.runId =
        runId;

    certificate.caseId =
        caseId;

    certificate.workstationId =
        workstationId;

    certificate.sourceIdentifier =
        source.sourceIdentifier;

    certificate.sourceName =
        source.sourceName;

    certificate.model =
        source.model;

    certificate.serialNumber =
        source.serialNumber;

    certificate.capacityBytes =
        source.capacityBytes;

    certificate.interfaceType =
        source.interfaceType;

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

    certificate.artifacts =
        certificateArtifacts;

    certificate.certificateHash =
        sha256String(
            canonicalCertificate(
                certificate));

    if (
        certificate.certificateHash.empty() ||
        certificate.certificateHash.size() !=
            64)
    {
        errorMessage =
            "Unable to calculate native forensic certificate SHA-256.";

        return false;
    }

    appendAuditEvent(
        auditEvents,
        caseId,
        runId,
        workstationId,
        source.sourceIdentifier,
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
        source.sourceIdentifier,
        "ACQUISITION_COMPLETED",
        "",
        "certificateHash=" +
            certificate.certificateHash +
            ";completionMarker=1");

    std::ostringstream nativeCertificateJson;

    nativeCertificateJson
        << certificateJson(
               certificate);

    std::ostringstream nativeAuditJson;

    nativeAuditJson
        << auditJson(
               auditEvents);

    std::ostringstream package;

    package
        << "{"
        << "\"schemaVersion\":1,"
        << "\"runId\":\""
        << jsonEscape(
               runId)
        << "\","
        << "\"workstationId\":\""
        << jsonEscape(
               workstationId)
        << "\","
        << "\"sourceType\":\""
        << jsonEscape(
               source.sourceType)
        << "\","
        << "\"sourceIdentifier\":\""
        << jsonEscape(
               source.sourceIdentifier)
        << "\","
        << "\"status\":\"COMPLETED\","
        << "\"source\":{"
        << "\"deviceId\":\""
        << jsonEscape(
               source.sourceIdentifier)
        << "\","
        << "\"model\":\""
        << jsonEscape(
               source.model)
        << "\","
        << "\"serialNumber\":\""
        << jsonEscape(
               source.serialNumber)
        << "\","
        << "\"capacityBytes\":"
        << source.capacityBytes
        << ","
        << "\"interfaceType\":\""
        << jsonEscape(
               source.interfaceType)
        << "\""
        << "},"
        << "\"summary\":{"
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
        << "\"artifacts\":"
        << artifactsJson.str()
        << ","
        << "\"nativeAudit\":"
        << nativeAuditJson.str()
        << ","
        << "\"certificate\":"
        << nativeCertificateJson.str()
        << "}";

    json =
        package.str();

    /*
     * The backend limits raw artifact bytes to 20 MiB.
     * Keep a client-side body guard so an oversized package
     * is rejected before network transmission.
     */
    constexpr std::size_t
        maximumPackageBytes =
            32U * 1024U * 1024U;

    if (
        json.size() >
        maximumPackageBytes)
    {
        errorMessage =
            "Forensic evidence package exceeds the 32 MiB client upload limit.";

        json.clear();

        return false;
    }

    return true;
}

} // namespace ForensicIntegrity
} // namespace SecureWipe