const SanitizationResult = require("../models/SanitizationResult");
const SanitizationRequest = require("../models/SanitizationRequest");
const SanitizationCertificate = require("../models/SanitizationCertificate");
const Workstation = require("../models/WorkStation");
const AppError = require("../utils/AppError");

const submitSanitizationResult = async (
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
        ![
            "WORKSTATION_EMPLOYEE",
            "ADMIN"
        ].includes(user.role)
    ) {
        throw new AppError(
            "Only the assigned workstation employee or admin can submit sanitization results",
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
        user.role ===
        "WORKSTATION_EMPLOYEE"
    ) {
        if (
            !request.assignedEmployee ||
            request.assignedEmployee.toString() !==
                user._id.toString()
        ) {
            throw new AppError(
                "This sanitization request is not assigned to you",
                403
            );
        }

        if (!request.assignedWorkstation) {
            throw new AppError(
                "This sanitization request has no assigned workstation",
                409
            );
        }

        const workstation =
            await Workstation.findById(
                request.assignedWorkstation
            );

        if (!workstation) {
            throw new AppError(
                "The workstation assigned to this sanitization request no longer exists",
                409
            );
        }

        if (
            !workstation.assignedEmployee ||
            workstation.assignedEmployee.toString() !==
                user._id.toString()
        ) {
            throw new AppError(
                "The assigned workstation is not bound to the authenticated employee",
                403
            );
        }

        if (
            workstation.workstationCenter.toString() !==
            request.workstationCenter.toString()
        ) {
            throw new AppError(
                "The assigned workstation does not belong to the request workstation center",
                403
            );
        }

        if (workstation.status !== "ACTIVE") {
            throw new AppError(
                "The assigned workstation is not active",
                409
            );
        }
    }

    if (
        ![
            "IN_PROGRESS",
            "VERIFYING"
        ].includes(request.status)
    ) {
        throw new AppError(
            `Sanitization result cannot be submitted while request status is ${request.status}`,
            400
        );
    }

    if (
        !payload ||
        typeof payload !== "object"
    ) {
        throw new AppError(
            "Sanitization result payload is required",
            400
        );
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
        "verificationStatus",
        "workstationId"
    ];

    for (
        const field of requiredFields
    ) {
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

    const submittedWorkstationId =
        String(payload.workstationId || "").trim();

    if (!submittedWorkstationId) {
        throw new AppError(
            "Workstation ID is required",
            400
        );
    }

    if (
        !request.assignedWorkstation
    ) {
        throw new AppError(
            "This sanitization request has no assigned workstation",
            409
        );
    }

    const assignedWorkstation =
        await Workstation.findById(
            request.assignedWorkstation
        );

    if (!assignedWorkstation) {
        throw new AppError(
            "The workstation assigned to this sanitization request no longer exists",
            409
        );
    }

    if (
        assignedWorkstation.workstationId !==
        submittedWorkstationId
    ) {
        throw new AppError(
            "The submitted workstation does not match the workstation assigned to this request",
            403
        );
    }

    if (
        user.role === "WORKSTATION_EMPLOYEE" &&
        (
            !assignedWorkstation.assignedEmployee ||
            assignedWorkstation.assignedEmployee.toString() !==
                user._id.toString()
        )
    ) {
        throw new AppError(
            "The submitted workstation is not assigned to the authenticated employee",
            403
        );
    }

    if (
        payload.method !==
        "HOST_OVERWRITE"
    ) {
        throw new AppError(
            "The current end-to-end pipeline accepts HOST_OVERWRITE results only",
            400
        );
    }

    if (
        ![
            "COMPLETED",
            "FAILED"
        ].includes(
            String(payload.status)
        )
    ) {
        throw new AppError(
            "Sanitization result status must be COMPLETED or FAILED",
            400
        );
    }

    const verificationPassed =
        Boolean(
            payload.verificationPerformed
        ) &&
        payload.verificationStatus ===
            "PASSED";

    if (
        payload.status ===
            "COMPLETED" &&
        !verificationPassed
    ) {
        throw new AppError(
            "A completed sanitization result requires performed and passed verification",
            400
        );
    }

    if (
        payload.status === "FAILED" &&
        payload.verificationStatus ===
            "PASSED"
    ) {
        throw new AppError(
            "A failed sanitization result cannot report PASSED verification",
            400
        );
    }

    const existingResult =
        await SanitizationResult.findOne({
            operationId:
                String(
                    payload.operationId
                )
        });

    if (existingResult) {
        throw new AppError(
            "A sanitization result already exists for this operation",
            409
        );
    }

    const result =
        await SanitizationResult.create({
            requestId:
                request.requestId,

            operationId:
                String(
                    payload.operationId
                ),

            submittedBy:
                user._id,

            workstationId:
                submittedWorkstationId,

            deviceId:
                String(
                    payload.deviceId
                ),

            model:
                String(
                    payload.model
                ),

            serialNumber:
                String(
                    payload.serialNumber
                ),

            capacityBytes:
                Number(
                    payload.capacityBytes
                ),

            interfaceType:
                String(
                    payload.interfaceType
                ),

            method:
                String(
                    payload.method
                ),

            status:
                String(
                    payload.status
                ),

            bytesProcessed:
                Number(
                    payload.bytesProcessed
                ) || 0,

            operationDurationMs:
                Number(
                    payload.operationDurationMs
                ) || 0,

            verificationStatus:
                String(
                    payload.verificationStatus
                ),

            verificationPerformed:
                Boolean(
                    payload.verificationPerformed
                ),

            verificationPassed,

            bytesVerified:
                Number(
                    payload.bytesVerified
                ) || 0,

            verificationSamples:
                Number(
                    payload.verificationSamples
                ) || 0,

            verificationMessage:
                String(
                    payload.verificationMessage ||
                        ""
                ),

            nativeErrorCode:
                Number(
                    payload.nativeErrorCode
                ) || 0,

            deviceReportedSuccess:
                Boolean(
                    payload.deviceReportedSuccess
                ),

            globalDataErased:
                Boolean(
                    payload.globalDataErased
                )
        });

    /*
     * A successful physical sanitization result
     * enters the verification/certificate phase.
     */
    if (
        result.status ===
        "COMPLETED"
    ) {
        request.status =
            "VERIFYING";

        request.history.push({
            status: "VERIFYING",
            changedBy: user._id,
            changedAt: new Date(),
            note:
                "Sanitization result submitted for verification and certificate review"
        });

        await request.save();
    }

    /*
     * A failed physical sanitization operation
     * terminates the request as FAILED.
     *
     * It must not enter VERIFYING because there
     * is no successful sanitization to certify.
     */
    if (
        result.status ===
        "FAILED"
    ) {
        request.status =
            "FAILED";

        request.completedAt =
            new Date();

        request.history.push({
            status: "FAILED",
            changedBy: user._id,
            changedAt: new Date(),
            note:
                result.verificationMessage ||
                "Sanitization operation failed on the workstation"
        });

        await request.save();
    }

    return result;
};

const getSanitizationResultByRequest =
    async (
        requestId,
        user
    ) => {
        if (!user) {
            throw new AppError(
                "Authentication required",
                401
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
            user.role ===
                "CUSTOMER" &&
            request.customer.toString() !==
                user._id.toString()
        ) {
            throw new AppError(
                "Access denied",
                403
            );
        }

        if (
            user.role ===
                "WORKSTATION_EMPLOYEE" &&
            request.assignedEmployee?.toString() !==
                user._id.toString()
        ) {
            throw new AppError(
                "Access denied",
                403
            );
        }

        if (
            user.role ===
                "WORKSTATION_HEAD" &&
            request.workstationCenter.toString() !==
                user.workstationCenter?.toString()
        ) {
            throw new AppError(
                "Access denied",
                403
            );
        }

        const result =
            await SanitizationResult.findOne({
                requestId
            })
                .populate(
                    "submittedBy",
                    "name email role"
                )
                .sort({
                    createdAt: -1
                });

        if (!result) {
            throw new AppError(
                "Sanitization result not found",
                404
            );
        }

        const certificate =
            await SanitizationCertificate.findOne({
                operationId:
                    result.operationId
            })
                .populate(
                    "generatedBy",
                    "name email role"
                );

        const resultData =
            result.toObject();

        resultData.certificate =
            certificate
                ? certificate.toObject()
                : null;

        return resultData;
    };

module.exports = {
    submitSanitizationResult,
    getSanitizationResultByRequest
};