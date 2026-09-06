#include "NvmeCapability.h"

#include <Windows.h>
#include <winioctl.h>
#include <nvme.h>

#include <cstddef>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

 
// Convert fixed-width NVMe character fields into std::string
 

static std::string decodeNvmeString(
    const UCHAR *buffer,
    std::size_t length)
{
    std::string result;

    result.reserve(length);

    for (std::size_t i = 0; i < length; ++i)
    {
        char character = static_cast<char>(buffer[i]);

        if (character != '\0')
        {
            result += character;
        }
    }

    // NVMe identification strings are normally
    // space padded.
    while (!result.empty() &&
           result.back() == ' ')
    {
        result.pop_back();
    }

    return result;
}

 
// NVMe Identify Controller
//
// NON-DESTRUCTIVE.
//
// This only asks Windows/NVMe controller for information.
// No sanitize command is sent.
 

bool detectNvmeCapability(HANDLE deviceHandle,SanitizationCapability &capability)
{
    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    // --------------------------------------------------------
    // Allocate a buffer large enough for:
    //
    // STORAGE_PROPERTY_QUERY
    // +
    // STORAGE_PROTOCOL_SPECIFIC_DATA
    // +
    // NVMe Identify Controller data
    // --------------------------------------------------------

    constexpr ULONG identifyDataLength =
        sizeof(NVME_IDENTIFY_CONTROLLER_DATA);

    constexpr ULONG protocolDataOffset =
        sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);

    constexpr ULONG bufferSize =
        sizeof(STORAGE_PROPERTY_QUERY) +
        protocolDataOffset +
        identifyDataLength;

    std::vector<UCHAR> buffer(
        bufferSize,
        0);

    // --------------------------------------------------------
    // STORAGE_PROPERTY_QUERY
    // --------------------------------------------------------

    auto *query =
        reinterpret_cast<
            STORAGE_PROPERTY_QUERY *>(
            buffer.data());

    query->PropertyId =
        StorageAdapterProtocolSpecificProperty;

    query->QueryType =
        PropertyStandardQuery;

    // --------------------------------------------------------
    // STORAGE_PROTOCOL_SPECIFIC_DATA
    // --------------------------------------------------------

    auto *protocolData =
        reinterpret_cast<
            STORAGE_PROTOCOL_SPECIFIC_DATA *>(
            query->AdditionalParameters);

    protocolData->ProtocolType =
        ProtocolTypeNvme;

    protocolData->DataType =
        NVMeDataTypeIdentify;

    // Identify Controller
    protocolData->ProtocolDataRequestValue =
        NVME_IDENTIFY_CNS_CONTROLLER;

    protocolData->ProtocolDataRequestSubValue =
        0;

    protocolData->ProtocolDataOffset =
        protocolDataOffset;

    protocolData->ProtocolDataLength =
        identifyDataLength;

    // --------------------------------------------------------
    // Send query to Windows storage stack
    // --------------------------------------------------------

    DWORD bytesReturned = 0;

    BOOL success =
        DeviceIoControl(
            deviceHandle,

            IOCTL_STORAGE_QUERY_PROPERTY,

            buffer.data(),
            bufferSize,

            buffer.data(),
            bufferSize,

            &bytesReturned,

            nullptr);

    if (!success)
    {
        DWORD error =
            GetLastError();

        std::cout
            << "\nNVMe Identify Controller query FAILED.\n";

        std::cout
            << "Windows error: "
            << error
            << '\n';

        capability.nvmeIdentifyAvailable =
            false;

        return false;
    }

    // --------------------------------------------------------
    // Validate returned size
    // --------------------------------------------------------

    if (bytesReturned <
        sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))
    {
        std::cout
            << "\nNVMe Identify response is too small.\n";

        capability.nvmeIdentifyAvailable =
            false;

        return false;
    }

    // --------------------------------------------------------
    // Get protocol descriptor
    // --------------------------------------------------------

    auto *descriptor =
        reinterpret_cast<
            STORAGE_PROTOCOL_DATA_DESCRIPTOR *>(
            buffer.data());

    const auto &returnedProtocolData =
        descriptor->ProtocolSpecificData;

    // --------------------------------------------------------
    // Validate returned protocol data
    // --------------------------------------------------------

    if (returnedProtocolData.ProtocolDataLength == 0)
    {
        std::cout
            << "\nNVMe Identify returned no protocol data.\n";

        capability.nvmeIdentifyAvailable =
            false;

        return false;
    }

    if (returnedProtocolData.ProtocolDataOffset + returnedProtocolData.ProtocolDataLength > bufferSize)
    {
        std::cout
            << "\nNVMe Identify response contains "
               "invalid data range.\n";

        capability.nvmeIdentifyAvailable = false;

        return false;
    }

    // --------------------------------------------------------
    // Locate NVMe Identify Controller data
    // --------------------------------------------------------

    const UCHAR *identifyBuffer =
        reinterpret_cast<
            const UCHAR *>(
            &returnedProtocolData) +
        returnedProtocolData.ProtocolDataOffset;

    const auto *identifyData =
        reinterpret_cast<
            const NVME_IDENTIFY_CONTROLLER_DATA *>(
            identifyBuffer);

    // --------------------------------------------------------
    // Mark Identify as available
    // --------------------------------------------------------

    capability.nvmeIdentifyAvailable =
        true;

    // --------------------------------------------------------
    // Display basic controller information
    // --------------------------------------------------------

    std::cout
        << "\n========================================\n"
        << " NVMe Identify Controller\n"
        << "========================================\n";

    std::cout
        << "Vendor ID        : 0x"
        << std::hex
        << std::setw(4)
        << std::setfill('0')
        << static_cast<unsigned int>(
               identifyData->VID)
        << std::dec
        << '\n';

    std::cout
        << "Subsystem Vendor : 0x"
        << std::hex
        << std::setw(4)
        << std::setfill('0')
        << static_cast<unsigned int>(
               identifyData->SSVID)
        << std::dec
        << '\n';

    std::string serial =
        decodeNvmeString(
            identifyData->SN,
            sizeof(identifyData->SN));

    std::string model =
        decodeNvmeString(
            identifyData->MN,
            sizeof(identifyData->MN));

    std::string firmware =
        decodeNvmeString(
            identifyData->FR,
            sizeof(identifyData->FR));

    std::cout
        << "Serial           : "
        << serial
        << '\n';

    std::cout
        << "Model            : "
        << model
        << '\n';

    std::cout
        << "Firmware         : "
        << firmware
        << '\n';

    std::cout
        << "Namespaces       : "
        << identifyData->NN
        << '\n';

    // --------------------------------------------------------
    // SANICAP
    // --------------------------------------------------------

    std::cout
        << "\nNVMe Sanitize Capabilities\n";

    capability.nvmeCryptoEraseSupported =
        identifyData->SANICAP.CryptoErase != 0;

    capability.nvmeBlockEraseSupported =
        identifyData->SANICAP.BlockErase != 0;

    capability.nvmeOverwriteSupported =
        identifyData->SANICAP.Overwrite != 0;

    std::cout
        << "Crypto Erase     : "
        << (capability.nvmeCryptoEraseSupported
                ? "SUPPORTED"
                : "NOT SUPPORTED")
        << '\n';

    std::cout
        << "Block Erase      : "
        << (capability.nvmeBlockEraseSupported
                ? "SUPPORTED"
                : "NOT SUPPORTED")
        << '\n';

    std::cout
        << "Overwrite        : "
        << (capability.nvmeOverwriteSupported
                ? "SUPPORTED"
                : "NOT SUPPORTED")
        << '\n';

    // --------------------------------------------------------
    // Overall native sanitize capability
    // --------------------------------------------------------

    if (capability.nvmeCryptoEraseSupported ||
        capability.nvmeBlockEraseSupported ||
        capability.nvmeOverwriteSupported)
    {
        capability.nativeSanitizeSupported =
            NativeSanitizeSupport::SUPPORTED;
    }
    else
    {
        capability.nativeSanitizeSupported =
            NativeSanitizeSupport::NOT_SUPPORTED;
    }

    std::cout
        << "\nNative NVMe Sanitize Support : "
        << (capability.nativeSanitizeSupported ==
                    NativeSanitizeSupport::SUPPORTED
                ? "SUPPORTED"
                : "NOT SUPPORTED")
        << '\n';

    std::cout
        << "========================================\n";

    return true;
}