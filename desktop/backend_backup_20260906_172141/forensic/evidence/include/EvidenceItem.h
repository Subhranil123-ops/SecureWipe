#pragma once

#include <cstdint>
#include <string>
#include <vector>

enum class ForensicConfidence
{
    UNKNOWN,
    LOW,
    MEDIUM,
    HIGH
};

struct EvidenceItem
{
     
    // Artifact Identification
     

    std::string artifactId;

    // Original source from which the artifact was recovered.
    // Example: physical disk / forensic image.
    std::string source;

    // Byte offset of the recovered artifact in the source.
    std::uint64_t offset = 0;

    // Number of bytes recovered.
    std::uint64_t size = 0;


     
    // Recovered File Information
     

    std::string fileName;

    // Detected file type.
    // Example: JPEG, PNG, PDF, DOCX.
    std::string fileType;

    // Path where the recovered artifact was stored.
    std::string recoveredPath;


     
    // Validation Results
     

    bool headerValid = false;

    bool footerValid = false;

    bool structureValid = false;

    bool sizeValid = false;

    bool decodable = false;


     
    // Confidence
     

    int confidenceScore = 0;

    ForensicConfidence confidence =
        ForensicConfidence::UNKNOWN;

    // Human-readable explanation of the score.
    //
    // Example:
    // "Valid JPEG header"
    // "Valid JPEG footer"
    // "Internal JPEG structure is valid"
    std::vector<std::string> confidenceReasons;


     
    // Integrity
     

    // SHA-256 or another cryptographic hash of the
    // recovered artifact.
    std::string sha256;


     
    // Status
     

    bool recovered = false;

    bool validated = false;


     
    // Helper
     

    std::string getConfidenceString() const
    {
        switch (confidence)
        {
        case ForensicConfidence::HIGH:
            return "HIGH";

        case ForensicConfidence::MEDIUM:
            return "MEDIUM";

        case ForensicConfidence::LOW:
            return "LOW";

        case ForensicConfidence::UNKNOWN:
        default:
            return "UNKNOWN";
        }
    }
};