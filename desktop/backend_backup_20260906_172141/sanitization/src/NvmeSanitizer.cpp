#include "NvmeSanitizer.h"

#include <Windows.h>
#include <winioctl.h>
#include <nvme.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <vector>

namespace
{
    constexpr bool ENABLE_DESTRUCTIVE_SANITIZE = false;

    constexpr std::uint32_t NVME_SANITIZE_OPCODE = 0x84;

    constexpr std::uint32_t SANACT_MASK = 0x7;
    constexpr std::uint32_t SANACT_BLOCK_ERASE = 2;
    constexpr std::uint32_t SANACT_OVERWRITE = 3;
    constexpr std::uint32_t SANACT_CRYPTO_ERASE = 4;

    constexpr ULONG NVME_LOG_QUERY_SIZE = 512;

    constexpr DWORD SANITIZE_STATUS_POLL_INTERVAL_MS = 1000;

    std::uint32_t getSanitizeAction(
        NvmeSanitizeMethod method)
    {
        switch (method)
        {
        case NvmeSanitizeMethod::BlockErase:
            return SANACT_BLOCK_ERASE;

        case NvmeSanitizeMethod::Overwrite:
            return SANACT_OVERWRITE;

        case NvmeSanitizeMethod::CryptoErase:
            return SANACT_CRYPTO_ERASE;

        default:
            return 0;
        }
    }

    enum class NvmeSanitizeStatus
    {
        None,
        Succeeded,
        InProgress,
        Failed,
        SucceededWithForcedDeallocation,
        QueryFailed
    };

    NvmeSanitizeStatus queryNvmeSanitizeStatus(
        HANDLE deviceHandle,
        NVME_SANITIZE_STATUS_LOG& statusLog)
    {
        if (deviceHandle == INVALID_HANDLE_VALUE)
        {
            return NvmeSanitizeStatus::QueryFailed;
        }

        constexpr ULONG protocolDataOffset =
            sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA);

        constexpr ULONG logDataLength =
            NVME_LOG_QUERY_SIZE;

        constexpr ULONG bufferSize =
            sizeof(STORAGE_PROPERTY_QUERY) +
            protocolDataOffset +
            logDataLength;

        std::vector<std::uint8_t> buffer(
            bufferSize,
            0);

        auto* query =
            reinterpret_cast<STORAGE_PROPERTY_QUERY*>(
                buffer.data());

        query->PropertyId =
            StorageAdapterProtocolSpecificProperty;

        query->QueryType =
            PropertyStandardQuery;

        auto* protocolData =
            reinterpret_cast<STORAGE_PROTOCOL_SPECIFIC_DATA*>(
                query->AdditionalParameters);

        protocolData->ProtocolType =
            ProtocolTypeNvme;

        protocolData->DataType =
            NVMeDataTypeLogPage;

        protocolData->ProtocolDataRequestValue =
            NVME_LOG_PAGE_SANITIZE_STATUS;

        protocolData->ProtocolDataRequestSubValue =
            0;

        protocolData->ProtocolDataOffset =
            protocolDataOffset;

        protocolData->ProtocolDataLength =
            logDataLength;

        DWORD bytesReturned = 0;

        const BOOL success =
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
            std::cout
                << "\nSanitize Status query FAILED.\n";

            std::cout
                << "Windows error: "
                << GetLastError()
                << '\n';

            return NvmeSanitizeStatus::QueryFailed;
        }

        if (bytesReturned <
            sizeof(STORAGE_PROTOCOL_DATA_DESCRIPTOR))
        {
            std::cout
                << "\nSanitize Status response is too small.\n";

            return NvmeSanitizeStatus::QueryFailed;
        }

        auto* descriptor =
            reinterpret_cast<
                STORAGE_PROTOCOL_DATA_DESCRIPTOR*>(
                buffer.data());

        const auto& returnedProtocolData =
            descriptor->ProtocolSpecificData;

        if (returnedProtocolData.ProtocolDataOffset <
            sizeof(STORAGE_PROTOCOL_SPECIFIC_DATA))
        {
            std::cout
                << "\nInvalid sanitize status data offset.\n";

            return NvmeSanitizeStatus::QueryFailed;
        }

        if (returnedProtocolData.ProtocolDataLength <
            sizeof(NVME_SANITIZE_STATUS_LOG))
        {
            std::cout
                << "\nSanitize Status log data is incomplete.\n";

            return NvmeSanitizeStatus::QueryFailed;
        }

        if (returnedProtocolData.ProtocolDataOffset +
                returnedProtocolData.ProtocolDataLength >
            bufferSize)
        {
            std::cout
                << "\nSanitize Status response contains "
                   "invalid data range.\n";

            return NvmeSanitizeStatus::QueryFailed;
        }

        const auto* statusData =
            reinterpret_cast<
                const NVME_SANITIZE_STATUS_LOG*>(
                reinterpret_cast<
                    const std::uint8_t*>(
                    &returnedProtocolData) +
                returnedProtocolData.ProtocolDataOffset);

        statusLog =
            *statusData;

        const auto operationStatus =
            static_cast<
                NVME_SANITIZE_OPERATION_STATUS>(
                statusLog.SSTAT
                    .MostRecentSanitizeOperationStatus);

        std::cout
            << "\nSanitize Status\n"
            << "---------------\n";

        std::cout
            << "Progress        : "
            << statusLog.SPROG
            << '\n';

        std::cout
            << "Status Code     : "
            << static_cast<unsigned int>(
                   statusLog.SSTAT
                       .MostRecentSanitizeOperationStatus)
            << '\n';

        std::cout
            << "Completed Passes: "
            << static_cast<unsigned int>(
                   statusLog.SSTAT
                       .NumberCompletedPassesOfOverwrite)
            << '\n';

        switch (operationStatus)
        {
        case NVME_SANITIZE_OPERATION_NONE:

            std::cout
                << "Operation Status: NONE\n";

            return NvmeSanitizeStatus::None;

        case NVME_SANITIZE_OPERATION_SUCCEEDED:

            std::cout
                << "Operation Status: SUCCEEDED\n";

            return NvmeSanitizeStatus::Succeeded;

        case NVME_SANITIZE_OPERATION_IN_PROGRESS:

            std::cout
                << "Operation Status: IN PROGRESS\n";

            return NvmeSanitizeStatus::InProgress;

        case NVME_SANITIZE_OPERATION_FAILED:

            std::cout
                << "Operation Status: FAILED\n";

            return NvmeSanitizeStatus::Failed;

        case NVME_SANITIZE_OPERATION_SUCCEEDED_WITH_FORCED_DEALLOCATION:

            std::cout
                << "Operation Status: "
                   "SUCCEEDED WITH FORCED DEALLOCATION\n";

            return NvmeSanitizeStatus::
                SucceededWithForcedDeallocation;

        default:

            std::cout
                << "Operation Status: UNKNOWN\n";

            return NvmeSanitizeStatus::QueryFailed;
        }
    }

    bool sendNvmeSanitizeCommand(
        HANDLE deviceHandle,
        NvmeSanitizeMethod method)
    {
        if (deviceHandle == INVALID_HANDLE_VALUE)
        {
            std::cout
                << "Invalid NVMe device handle.\n";

            return false;
        }

        const std::uint32_t sanitizeAction =
            getSanitizeAction(method);

        if ((sanitizeAction & SANACT_MASK) == 0)
        {
            std::cout
                << "Invalid NVMe sanitize method.\n";

            return false;
        }

        std::cout
            << "\nNVMe Sanitize Command\n"
            << "--------------------\n";

        switch (method)
        {
        case NvmeSanitizeMethod::BlockErase:

            std::cout
                << "Selected method : Block Erase\n";

            break;

        case NvmeSanitizeMethod::CryptoErase:

            std::cout
                << "Selected method : Crypto Erase\n";

            break;

        case NvmeSanitizeMethod::Overwrite:

            std::cout
                << "Selected method : Overwrite\n";

            break;
        }

        std::cout
            << "NVMe Opcode      : 0x84\n";

        std::cout
            << "SANACT           : "
            << sanitizeAction
            << '\n';

        if (!ENABLE_DESTRUCTIVE_SANITIZE)
        {
            std::cout
                << "\nSAFE TEST MODE\n";

            std::cout
                << "Sanitize command was NOT sent.\n";

            std::cout
                << "Enable "
                   "ENABLE_DESTRUCTIVE_SANITIZE "
                   "only for a dedicated test NVMe.\n";

            return true;
        }

        const std::size_t commandOffset =
            offsetof(
                STORAGE_PROTOCOL_COMMAND,
                Command);

        const std::size_t commandLength =
            STORAGE_PROTOCOL_COMMAND_LENGTH_NVME;

        const std::size_t bufferSize =
            commandOffset +
            commandLength;

        std::vector<std::uint8_t> buffer(
            bufferSize,
            0);

        auto* protocolCommand =
            reinterpret_cast<
                STORAGE_PROTOCOL_COMMAND*>(
                buffer.data());

        protocolCommand->Version =
            STORAGE_PROTOCOL_STRUCTURE_VERSION;

        protocolCommand->Length =
            sizeof(STORAGE_PROTOCOL_COMMAND);

        protocolCommand->ProtocolType =
            ProtocolTypeNvme;

        protocolCommand->Flags =
            0;

        protocolCommand->CommandLength =
            static_cast<ULONG>(
                commandLength);

        protocolCommand->ErrorInfoLength =
            0;

        protocolCommand->DataToDeviceTransferLength =
            0;

        protocolCommand->DataFromDeviceTransferLength =
            0;

        protocolCommand->TimeOutValue =
            30;

        protocolCommand->CommandSpecific =
            STORAGE_PROTOCOL_SPECIFIC_NVME_ADMIN_COMMAND;

        std::uint8_t* command =
            protocolCommand->Command;

        // CDW0 byte 0 = OPC

        command[0] =
            static_cast<std::uint8_t>(
                NVME_SANITIZE_OPCODE);

        // NSID = 0

        std::memset(
            command + 4,
            0,
            sizeof(std::uint32_t));

        // CDW10

        const std::uint32_t cdw10 =
            sanitizeAction &
            SANACT_MASK;

        std::memcpy(
            command + 40,
            &cdw10,
            sizeof(cdw10));

        DWORD returnedLength = 0;

        const BOOL success =
            DeviceIoControl(
                deviceHandle,
                IOCTL_STORAGE_PROTOCOL_COMMAND,
                buffer.data(),
                static_cast<DWORD>(
                    buffer.size()),
                buffer.data(),
                static_cast<DWORD>(
                    buffer.size()),
                &returnedLength,
                nullptr);

        if (!success)
        {
            const DWORD error =
                GetLastError();

            std::cout
                << "\nNVMe Sanitize command FAILED.\n";

            std::cout
                << "Windows error: "
                << error
                << '\n';

            return false;
        }

        std::cout
            << "\nDeviceIoControl completed.\n";

        std::cout
            << "Return status : 0x"
            << std::hex
            << protocolCommand->ReturnStatus
            << std::dec
            << '\n';

        std::cout
            << "Error code    : 0x"
            << std::hex
            << protocolCommand->ErrorCode
            << std::dec
            << '\n';

        if (protocolCommand->ReturnStatus !=
            STORAGE_PROTOCOL_STATUS_SUCCESS)
        {
            std::cout
                << "NVMe controller did not "
                   "report command success.\n";

            return false;
        }

        std::cout
            << "NVMe Sanitize command "
               "accepted successfully.\n";

        return true;
    }
}

bool executeNvmeSanitize(
    HANDLE deviceHandle,
    NvmeSanitizeMethod method)
{
    if (deviceHandle == INVALID_HANDLE_VALUE)
    {
        std::cout
            << "Cannot sanitize: "
               "Invalid device handle.\n";

        return false;
    }

    if (!sendNvmeSanitizeCommand(
            deviceHandle,
            method))
    {
        std::cout
            << "NVMe Sanitize command "
               "was not accepted.\n";

        return false;
    }

    if (!ENABLE_DESTRUCTIVE_SANITIZE)
    {
        std::cout
            << "\nSanitization was not executed "
               "because SAFE TEST MODE is enabled.\n";

        return true;
    }

    std::cout
        << "\nWaiting for NVMe sanitize "
           "operation to complete...\n";

    while (true)
    {
        NVME_SANITIZE_STATUS_LOG statusLog{};

        const NvmeSanitizeStatus status =
            queryNvmeSanitizeStatus(
                deviceHandle,
                statusLog);

        switch (status)
        {
        case NvmeSanitizeStatus::Succeeded:

            std::cout
                << "\nNVMe sanitization "
                   "COMPLETED successfully.\n";

            return true;

        case NvmeSanitizeStatus::
            SucceededWithForcedDeallocation:

            std::cout
                << "\nNVMe sanitization "
                   "COMPLETED with forced "
                   "deallocation.\n";

            return true;

        case NvmeSanitizeStatus::InProgress:

            Sleep(
                SANITIZE_STATUS_POLL_INTERVAL_MS);

            break;

        case NvmeSanitizeStatus::Failed:

            std::cout
                << "\nNVMe sanitization FAILED.\n";

            return false;

        case NvmeSanitizeStatus::None:

            std::cout
                << "\nNo sanitize operation "
                   "is currently reported.\n";

            return false;

        case NvmeSanitizeStatus::QueryFailed:

        default:

            std::cout
                << "\nUnable to determine "
                   "NVMe sanitize status.\n";

            return false;
        }
    }
}