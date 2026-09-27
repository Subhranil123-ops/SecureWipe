find_package(libjpeg-turbo CONFIG REQUIRED)

# ============================================================
# CORE STORAGE / SANITIZATION / FORENSICS LIBRARY
# ============================================================

add_library(
    SecureWipeStorage

    # ----------------------------------------------------------
    # STORAGE
    # ----------------------------------------------------------

    storage/src/StorageDevice.cpp
    storage/src/StorageManager.cpp

    # ----------------------------------------------------------
    # DISCOVERY
    # ----------------------------------------------------------

    discovery/src/WindowsStorageDiscovery.cpp

    # ----------------------------------------------------------
    # CLASSIFICATION
    # ----------------------------------------------------------

    classification/src/DeviceClassifier.cpp

    # ----------------------------------------------------------
    # SAFETY
    # ----------------------------------------------------------

    safety/src/BootInfo.cpp
    safety/src/WindowsBootChecker.cpp
    safety/src/WindowsStorageUtils.cpp
    safety/src/SafetyEngine.cpp

    # ----------------------------------------------------------
    # SANITIZATION
    # ----------------------------------------------------------

    sanitization/src/SanitizationEngine.cpp
    sanitization/src/SanitizationCapability.cpp
    sanitization/src/ScsiCapability.cpp
    sanitization/src/StoragePropertyCapability.cpp

    sanitization/src/NvmeCapability.cpp
    sanitization/src/NvmeSanitizer.cpp

    sanitization/src/AtaCapability.cpp
    sanitization/src/AtaSanitizer.cpp

    sanitization/src/HostOverwriteSanitizer.cpp
    sanitization/src/CertificateGenerator.cpp
    sanitization/src/AuditLogger.cpp
    sanitization/src/OperationLogger.cpp
    sanitization/src/SanitizationEvent.cpp
    sanitization/src/SanitizationPipeline.cpp

    # ----------------------------------------------------------
    # VERIFICATION
    # ----------------------------------------------------------

    verification/src/AuditChainVerifier.cpp
    verification/src/CertificateVerifier.cpp

    # ----------------------------------------------------------
    # FORENSICS
    # ----------------------------------------------------------

    forensic/evidence/src/EvidenceCollector.cpp
    forensic/validation/src/EvidenceValidator.cpp
    forensic/hashing/src/HashCalculator.cpp
    forensic/scoring/src/ConfidenceScorer.cpp

    # ----------------------------------------------------------
    # LIVE BACKEND PROGRESS REPORTING
    # ----------------------------------------------------------

    progress/src/LiveProgressReporter.cpp
)

# ============================================================
# PUBLIC INCLUDE DIRECTORIES
# ============================================================

target_include_directories(
    SecureWipeStorage
    PUBLIC

        storage/include

        discovery/include

        classification/include

        safety/include

        sanitization/include

        forensic/evidence/include
        forensic/validation/include
        forensic/hashing/include
        forensic/scoring/include

        verification/include

        progress/include
)

# ============================================================
# WINDOWS DEPENDENCIES
# ============================================================

if(WIN32)

    target_link_libraries(
        SecureWipeStorage
        PUBLIC

            wbemuuid
            setupapi
            bcrypt

            # JPEG decoding used by forensic validation
            libjpeg-turbo::jpeg

            # Live backend progress reporting
            winhttp
    )

endif()

# ============================================================
# UNIT / FUNCTIONAL TEST EXECUTABLES
# ============================================================

add_executable(
    StorageDeviceTest
    tests/StorageDeviceTest.cpp
)

add_executable(
    StorageManagerTest
    tests/StorageManagerTest.cpp
)

add_executable(
    WindowsStorageDiscoveryTest
    tests/WindowsStorageDiscoveryTest.cpp
)

add_executable(
    DeviceClassifierTest
    tests/DeviceClassifierTest.cpp
)

add_executable(
    WindowsBootCheckerTest
    tests/WindowsBootCheckerTest.cpp
)

add_executable(
    SafetyEngineTest
    tests/SafetyEngineTest.cpp
)

add_executable(
    SanitizationEngineTest
    tests/SanitizationEngineTest.cpp
)

add_executable(
    SanitizationCapabilityTest
    tests/SanitizationCapabilityTest.cpp
)

add_executable(
    NvmeCapabilityTest
    tests/NvmeCapabilityTest.cpp
)

add_executable(
    AtaCapabilityTest
    tests/AtaCapabilityTest.cpp
)

add_executable(
    SanitizationMethodSelectionTest
    tests/SanitizationMethodSelectionTest.cpp
)

add_executable(
    HostOverwriteSanitizerTest
    tests/HostOverwriteSanitizerTest.cpp
)

add_executable(
    SanitizationPipelineAuditTest
    tests/SanitizationPipelineAuditTest.cpp
)

add_executable(
    AuditChainVerifierTest
    tests/AuditChainVerifierTest.cpp
)

add_executable(
    CertificateVerifierTest
    tests/CertificateVerifierTest.cpp
)

# ============================================================
# REAL DEVICE HOST OVERWRITE + CERTIFICATE E2E TEST
#
# WARNING:
# This test performs a destructive operation on a real
# physical storage device after explicit confirmation.
# ============================================================

add_executable(
    HostOverwriteE2ETest
    tests/HostOverwriteE2ETest.cpp
)

if(WIN32)

    target_link_libraries(
        HostOverwriteE2ETest
        PRIVATE

            winhttp
    )

endif()

# ============================================================
# FORENSIC TESTS
# ============================================================

add_executable(
    EvidenceCollectorTest
    tests/forensic/evidence/EvidenceCollectorTest.cpp
)

add_executable(
    HashCalculatorTest
    tests/forensic/hashing/HashCalculatorTest.cpp
)

add_executable(
    ConfidenceScorerTest
    tests/forensic/scoring/ConfidenceScorerTest.cpp
)

# ============================================================
# ALL TEST TARGETS
# ============================================================

set(
    STORAGE_TESTS

    StorageDeviceTest
    StorageManagerTest
    WindowsStorageDiscoveryTest
    DeviceClassifierTest
    WindowsBootCheckerTest
    SafetyEngineTest

    SanitizationEngineTest
    SanitizationCapabilityTest
    NvmeCapabilityTest
    AtaCapabilityTest
    SanitizationMethodSelectionTest
    HostOverwriteSanitizerTest

    # ----------------------------------------------------------
    # Real physical-device E2E test
    # ----------------------------------------------------------

    HostOverwriteE2ETest

    # ----------------------------------------------------------
    # Pipeline / verification
    # ----------------------------------------------------------

    SanitizationPipelineAuditTest
    CertificateVerifierTest
    AuditChainVerifierTest

    # ----------------------------------------------------------
    # Forensic
    # ----------------------------------------------------------

    EvidenceCollectorTest
    HashCalculatorTest
    ConfidenceScorerTest
)

# ============================================================
# COMMON TEST CONFIGURATION
# ============================================================

foreach(TEST_TARGET IN LISTS STORAGE_TESTS)

    target_include_directories(
        ${TEST_TARGET}
        PRIVATE

            storage/include

            discovery/include

            classification/include

            safety/include

            sanitization/include

            forensic/evidence/include
            forensic/validation/include
            forensic/hashing/include
            forensic/scoring/include

            verification/include

            progress/include
    )

    target_link_libraries(
        ${TEST_TARGET}
        PRIVATE

            SecureWipeStorage
    )

endforeach()