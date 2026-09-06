const SanitizationResult = require("../models/SanitizationResult");
const SanitizationRequest = require("../models/SanitizationRequest");
const AppError = require("../utils/AppError");

const submitSanitizationResult = async (requestId, payload, user) => {
    if (!user) {
        throw new AppError("Authentication required", 401);
    }

    if (!["WORKSTATION_EMPLOYEE", "ADMIN"].includes(user.role)) {
        throw new AppError(
            "Only the assigned workstation employee or admin can submit sanitization results",
            403
        );
    }

    if (!requestId) {
        throw new AppError("Request ID is required", 400);
    }

    const request = await SanitizationRequest.findOne({ requestId });

    if (!request) {
        throw new AppError("Sanitization request not found", 404);
    }

    if (user.role === "WORKSTATION_EMPLOYEE") {
        if (
            !request.assignedEmployee ||
            request.assignedEmployee.toString() !== user._id.toString()
        ) {
            throw new AppError(
                "This sanitization request is not assigned to you",
                403
            );
        }
    }

    if (!["IN_PROGRESS", "VERIFYING"].includes(request.status)) {
        throw new AppError(
            `Sanitization result cannot be submitted while request status is ${request.status}`,
            400
        );
    }

    if (!payload || typeof payload !== "object") {
        throw new AppError("Sanitization result payload is required", 400);
    }

    const requiredFields = [
        "operationId",
        "deviceId",
        "model",
        "serialNumber",
        "capacityBytes",
        "interfaceType",
        "method",
        "status",
        "verificationStatus"
    ];

    for (const field of requiredFields) {
        if (
            payload[field] === undefined ||
            payload[field] === null ||
            payload[field] === ""
        ) {
            throw new AppError(
                `Sanitization result field '${field}' is required`,
                400
            );
        }
    }

    if (payload.method !== "HOST_OVERWRITE") {
        throw new AppError(
            "The current end-to-end pipeline accepts HOST_OVERWRITE results only",
            400
        );
    }

    const verificationPassed =
        Boolean(payload.verificationPerformed) &&
        payload.verificationStatus === "PASSED";

    if (payload.status === "COMPLETED" && !verificationPassed) {
        throw new AppError(
            "A completed sanitization result requires performed and passed verification",
            400
        );
    }

    if (payload.status === "FAILED" && payload.verificationStatus === "PASSED") {
        throw new AppError(
            "A failed sanitization result cannot report PASSED verification",
            400
        );
    }

    const existingResult = await SanitizationResult.findOne({
        operationId: String(payload.operationId)
    });

    if (existingResult) {
        throw new AppError(
            "A sanitization result already exists for this operation",
            409
        );
    }

    const result = await SanitizationResult.create({
        requestId: request.requestId,
        operationId: String(payload.operationId),
        submittedBy: user._id,

        deviceId: String(payload.deviceId),
        model: String(payload.model),
        serialNumber: String(payload.serialNumber),
        capacityBytes: Number(payload.capacityBytes),
        interfaceType: String(payload.interfaceType),

        method: String(payload.method),
        status: String(payload.status),

        bytesProcessed: Number(payload.bytesProcessed) || 0,
        operationDurationMs: Number(payload.operationDurationMs) || 0,

        verificationStatus: String(payload.verificationStatus),
        verificationPerformed: Boolean(payload.verificationPerformed),
        verificationPassed,

        bytesVerified: Number(payload.bytesVerified) || 0,
        verificationSamples: Number(payload.verificationSamples) || 0,

        verificationMessage: String(
            payload.verificationMessage || ""
        ),

        nativeErrorCode: Number(payload.nativeErrorCode) || 0,

        deviceReportedSuccess: Boolean(
            payload.deviceReportedSuccess
        ),

        globalDataErased: Boolean(
            payload.globalDataErased
        )
    });

    if (request.status === "IN_PROGRESS") {
        request.status = "VERIFYING";

        request.history.push({
            status: "VERIFYING",
            changedBy: user._id,
            changedAt: new Date(),
            note: "Sanitization result submitted for verification review"
        });

        await request.save();
    }

    return result;
};

const getSanitizationResultByRequest = async (
    requestId,
    user
) => {
    if (!user) {
        throw new AppError("Authentication required", 401);
    }

    const request = await SanitizationRequest.findOne({
        requestId
    });

    if (!request) {
        throw new AppError("Sanitization request not found", 404);
    }

    if (
        user.role === "CUSTOMER" &&
        request.customer.toString() !== user._id.toString()
    ) {
        throw new AppError("Access denied", 403);
    }

    if (
        user.role === "WORKSTATION_EMPLOYEE" &&
        request.assignedEmployee?.toString() !== user._id.toString()
    ) {
        throw new AppError("Access denied", 403);
    }

    if (
        user.role === "WORKSTATION_HEAD" &&
        request.workstationCenter.toString() !==
        user.workstationCenter?.toString()
    ) {
        throw new AppError("Access denied", 403);
    }

    const result = await SanitizationResult.findOne({
        requestId
    })
        .populate("submittedBy", "name email role")
        .sort({ createdAt: -1 });

    if (!result) {
        throw new AppError(
            "Sanitization result not found",
            404
        );
    }

    return result;
};

module.exports = {
    submitSanitizationResult,
    getSanitizationResultByRequest
};