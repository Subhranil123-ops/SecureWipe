const crypto = require("crypto");

const SanitizationCertificate = require(
    "../models/SanitizationCertificate"
);

const SanitizationResult = require(
    "../models/SanitizationResult"
);

const SanitizationRequest = require(
    "../models/SanitizationRequest"
);

const AppError = require("../utils/AppError");

const buildCanonicalData = certificate => {
    const boolToString = value => value ? "true" : "false";

    return [
        `certificateId=${certificate.certificateId}`,
        `operationId=${certificate.operationId}`,
        `requestId=${certificate.requestId}`,

        `deviceId=${certificate.deviceId}`,
        `model=${certificate.model}`,
        `serialNumber=${certificate.serialNumber}`,
        `capacityBytes=${certificate.capacityBytes}`,
        `interfaceType=${certificate.interfaceType}`,

        `method=${certificate.method}`,
        `status=${certificate.status}`,
        `bytesProcessed=${certificate.bytesProcessed}`,
        `operationDurationMs=${certificate.operationDurationMs}`,

        `verificationStatus=${certificate.verificationStatus}`,
        `verificationPerformed=${boolToString(certificate.verificationPerformed)}`,
        `verificationPassed=${boolToString(certificate.verificationPassed)}`,

        `bytesVerified=${certificate.bytesVerified}`,
        `verificationSamples=${certificate.verificationSamples}`,

        `deviceReportedSuccess=${boolToString(
            certificate.deviceReportedSuccess
        )}`,

        `globalDataErased=${boolToString(
            certificate.globalDataErased
        )}`,

        `nativeErrorCode=${certificate.nativeErrorCode}`,
        `verificationMessage=${certificate.verificationMessage}`,
        `generatedAt=${certificate.generatedAt}`,
        `hashAlgorithm=${certificate.hashAlgorithm}`,
        `message=${certificate.message}`
    ].join("\n") + "\n";
};

const calculateSha256 = data => {
    return crypto
        .createHash("sha256")
        .update(data, "utf8")
        .digest("hex");
};

const submitCertificate = async (
    requestId,
    payload,
    user
) => {
    if (!user) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    if (
        !["WORKSTATION_EMPLOYEE", "ADMIN"].includes(
            user.role
        )
    ) {
        throw new AppError(
            "Only the assigned workstation employee or admin can submit sanitization certificates",
            403
        );
    }

    if (!requestId) {
        throw new AppError(
            "Request ID is required",
            400
        );
    }

    const request =
        await SanitizationRequest.findOne({
            requestId
        });

    if (!request) {
        throw new AppError(
            "Sanitization request not found",
            404
        );
    }

    if (
        user.role === "WORKSTATION_EMPLOYEE" &&
        (
            !request.assignedEmployee ||
            request.assignedEmployee.toString() !==
            user._id.toString()
        )
    ) {
        throw new AppError(
            "This sanitization request is not assigned to you",
            403
        );
    }

    if (request.status !== "VERIFYING") {
        throw new AppError(
            `Certificate can only be submitted while request is VERIFYING, current status is ${request.status}`,
            400
        );
    }

    const result =
        await SanitizationResult.findOne({
            requestId,
            operationId: payload.operationId
        });

    if (!result) {
        throw new AppError(
            "Matching sanitization result was not found",
            404
        );
    }

    if (
        result.method !== "HOST_OVERWRITE"
    ) {
        throw new AppError(
            "The current end-to-end certificate pipeline accepts HOST_OVERWRITE only",
            400
        );
    }

    if (
        result.status !== "COMPLETED" ||
        result.verificationStatus !== "PASSED" ||
        !result.verificationPerformed ||
        !result.verificationPassed
    ) {
        throw new AppError(
            "Certificate cannot be accepted because sanitization verification did not pass",
            400
        );
    }

    const existing =
        await SanitizationCertificate.findOne({
            operationId: result.operationId
        });

    if (existing) {
        throw new AppError(
            "A certificate already exists for this operation",
            409
        );
    }

    const requiredFields = [
        "certificateId",
        "operationId",
        "deviceId",
        "model",
        "serialNumber",
        "capacityBytes",
        "interfaceType",
        "method",
        "status",
        "verificationStatus",
        "verificationPerformed",
        "verificationPassed",
        "bytesVerified",
        "verificationSamples",
        "generatedAt",
        "hashAlgorithm",
        "certificateHash"
    ];

    for (const field of requiredFields) {
        if (
            payload[field] === undefined ||
            payload[field] === null
        ) {
            throw new AppError(
                `Certificate field '${field}' is required`,
                400
            );
        }
    }

    const certificateData = {
        certificateId: String(
            payload.certificateId
        ),

        operationId: String(
            payload.operationId
        ),

        requestId: request.requestId,

        deviceId: String(
            payload.deviceId
        ),

        model: String(
            payload.model
        ),

        serialNumber: String(
            payload.serialNumber
        ),

        capacityBytes: Number(
            payload.capacityBytes
        ),

        interfaceType: String(
            payload.interfaceType
        ),

        method: String(
            payload.method
        ),

        status: String(
            payload.status
        ),

        bytesProcessed: Number(
            payload.bytesProcessed
        ) || 0,

        operationDurationMs: Number(
            payload.operationDurationMs
        ) || 0,

        verificationStatus: String(
            payload.verificationStatus
        ),

        verificationPerformed: Boolean(
            payload.verificationPerformed
        ),

        verificationPassed: Boolean(
            payload.verificationPassed
        ),

        bytesVerified: Number(
            payload.bytesVerified
        ) || 0,

        verificationSamples: Number(
            payload.verificationSamples
        ) || 0,

        deviceReportedSuccess: Boolean(
            payload.deviceReportedSuccess
        ),

        globalDataErased: Boolean(
            payload.globalDataErased
        ),

        nativeErrorCode: Number(
            payload.nativeErrorCode
        ) || 0,

        verificationMessage: String(
            payload.verificationMessage || ""
        ),

        generatedAt: new Date(
            payload.generatedAt
        ).toISOString(),

        hashAlgorithm: String(
            payload.hashAlgorithm
        ),

        message: String(
            payload.message || ""
        )
    };

    if (
        certificateData.operationId !==
        result.operationId
    ) {
        throw new AppError(
            "Certificate operation ID does not match sanitization result",
            400
        );
    }

    if (
        certificateData.deviceId !==
        result.deviceId ||
        certificateData.serialNumber !==
        result.serialNumber ||
        certificateData.capacityBytes !==
        result.capacityBytes
    ) {
        throw new AppError(
            "Certificate device identity does not match sanitization result",
            400
        );
    }

    if (
        certificateData.verificationStatus !==
        result.verificationStatus ||
        certificateData.verificationPerformed !==
        result.verificationPerformed ||
        certificateData.verificationPassed !==
        result.verificationPassed
    ) {
        throw new AppError(
            "Certificate verification evidence does not match sanitization result",
            400
        );
    }

    if (
        certificateData.hashAlgorithm !==
        "SHA-256"
    ) {
        throw new AppError(
            "Only SHA-256 certificates are supported",
            400
        );
    }

    const canonicalData =
        buildCanonicalData(
            certificateData
        );

    const calculatedHash =
        calculateSha256(
            canonicalData
        );

    if (
        calculatedHash.toLowerCase() !==
        String(payload.certificateHash).toLowerCase()
    ) {
        throw new AppError(
            "Certificate SHA-256 integrity verification failed",
            400
        );
    }

    const certificate =
        await SanitizationCertificate.create({
            ...certificateData,
            result: result._id,
            generatedBy: user._id,
            certificateHash: calculatedHash,
            integrityVerified: true
        });

    request.status = "COMPLETED";
    request.completedAt = new Date();

    request.history.push({
        status: "COMPLETED",
        changedBy: user._id,
        changedAt: new Date(),
        note:
            `Sanitization completed. Certificate ${certificate.certificateId} generated and SHA-256 integrity verified.`
    });

    await request.save();

    return certificate;
};

const getCertificateById = async (
    certificateId,
    user
) => {
    if (!user) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    const certificate =
        await SanitizationCertificate
            .findOne({ certificateId })
            .populate(
                "generatedBy",
                "name email role"
            )
            .populate(
                "result"
            );

    if (!certificate) {
        throw new AppError(
            "Sanitization certificate not found",
            404
        );
    }

    const request =
        await SanitizationRequest.findOne({
            requestId: certificate.requestId
        });

    if (!request) {
        throw new AppError(
            "Associated sanitization request not found",
            404
        );
    }

    if (
        user.role === "CUSTOMER" &&
        request.customer.toString() !==
        user._id.toString()
    ) {
        throw new AppError(
            "Access denied",
            403
        );
    }

    if (
        user.role === "WORKSTATION_EMPLOYEE" &&
        request.assignedEmployee?.toString() !==
        user._id.toString()
    ) {
        throw new AppError(
            "Access denied",
            403
        );
    }

    if (
        user.role === "WORKSTATION_HEAD" &&
        request.workstationCenter.toString() !==
        user.workstationCenter?.toString()
    ) {
        throw new AppError(
            "Access denied",
            403
        );
    }

    return certificate;
};

const verifyCertificateIntegrity = async (
    certificateId,
    user
) => {
    const certificate =
        await getCertificateById(
            certificateId,
            user
        );

    const certificateData = {
        certificateId: certificate.certificateId,
        operationId: certificate.operationId,
        requestId: certificate.requestId,

        deviceId: certificate.deviceId,
        model: certificate.model,
        serialNumber: certificate.serialNumber,
        capacityBytes: certificate.capacityBytes,
        interfaceType: certificate.interfaceType,

        method: certificate.method,
        status: certificate.status,
        bytesProcessed: certificate.bytesProcessed,
        operationDurationMs:
            certificate.operationDurationMs,

        verificationStatus:
            certificate.verificationStatus,

        verificationPerformed:
            certificate.verificationPerformed,

        verificationPassed:
            certificate.verificationPassed,

        bytesVerified:
            certificate.bytesVerified,

        verificationSamples:
            certificate.verificationSamples,

        deviceReportedSuccess:
            certificate.deviceReportedSuccess,

        globalDataErased:
            certificate.globalDataErased,

        nativeErrorCode:
            certificate.nativeErrorCode,

        verificationMessage:
            certificate.verificationMessage,

        generatedAt:
            new Date(
                certificate.generatedAt
            ).toISOString(),

        hashAlgorithm:
            certificate.hashAlgorithm,

        message:
            certificate.message
    };

    const canonicalData =
        buildCanonicalData(
            certificateData
        );

    const calculatedHash =
        calculateSha256(
            canonicalData
        );

    const valid =
        calculatedHash.toLowerCase() ===
        certificate.certificateHash.toLowerCase();

    return {
        certificateId:
            certificate.certificateId,

        storedHash:
            certificate.certificateHash,

        calculatedHash,

        hashAlgorithm:
            certificate.hashAlgorithm,

        valid
    };
};

module.exports = {
    submitCertificate,
    getCertificateById,
    verifyCertificateIntegrity
};