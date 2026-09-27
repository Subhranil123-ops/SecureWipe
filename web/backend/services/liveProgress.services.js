const mongoose = require("mongoose");

const SanitizationRequest =
    require("../models/SanitizationRequest");

const ForensicCase =
    require("../models/ForensicCase");

const WorkstationCenter =
    require("../models/WorkstationCenter");

const AppError =
    require("../utils/AppError");


// ======================================================
// COMMON HELPERS
// ======================================================

const objectIdToString = value => {
    if (!value) {
        return "";
    }

    return String(
        value._id || value
    );
};


const clampPercentage = value => {
    const numeric =
        Number(value);

    if (!Number.isFinite(numeric)) {
        return 0;
    }

    return Math.min(
        100,
        Math.max(
            0,
            numeric
        )
    );
};


const normalizeNonNegativeNumber = value => {
    const numeric =
        Number(value);

    if (
        !Number.isFinite(numeric) ||
        numeric < 0
    ) {
        return 0;
    }

    return numeric;
};


const normalizeString = (
    value,
    fallback = ""
) => {
    if (
        value === undefined ||
        value === null
    ) {
        return fallback;
    }

    return String(value).trim();
};


// ======================================================
// WORKSTATION HEAD CENTER RESOLUTION
// ======================================================

const resolveHeadCenterId = async user => {

    if (
        !user ||
        user.role !== "WORKSTATION_HEAD"
    ) {
        return null;
    }

    /*
     * Some existing code paths hydrate
     * req.user.workstationCenter.
     *
     * Do not depend on that being present here.
     * Resolve the center directly from the database.
     */

    if (user.workstationCenter) {
        return objectIdToString(
            user.workstationCenter
        );
    }

    const center =
        await WorkstationCenter.findOne({
            head: user._id,
            status: "ACTIVE"
        })
            .select("_id")
            .lean();

    if (!center) {
        return null;
    }

    return objectIdToString(
        center._id
    );
};


// ======================================================
// SANITIZATION ACCESS CONTROL
// ======================================================

const ensureSanitizationAccess = async (
    request,
    user
) => {

    if (!request) {
        throw new AppError(
            "Sanitization request not found",
            404
        );
    }

    if (
        !user ||
        !user._id ||
        !user.role
    ) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    const userId =
        objectIdToString(
            user._id
        );

    // --------------------------------------------------
    // ADMIN
    // --------------------------------------------------

    if (
        user.role === "ADMIN"
    ) {
        return;
    }

    // --------------------------------------------------
    // CUSTOMER
    // --------------------------------------------------

    if (
        user.role === "CUSTOMER"
    ) {

        if (
            objectIdToString(
                request.customer
            ) === userId
        ) {
            return;
        }

        throw new AppError(
            "Access denied",
            403
        );
    }

    // --------------------------------------------------
    // WORKSTATION EMPLOYEE
    // --------------------------------------------------

    if (
        user.role === "WORKSTATION_EMPLOYEE"
    ) {

        if (
            objectIdToString(
                request.assignedEmployee
            ) === userId
        ) {
            return;
        }

        throw new AppError(
            "Access denied",
            403
        );
    }

    // --------------------------------------------------
    // WORKSTATION HEAD
    // --------------------------------------------------

    if (
        user.role === "WORKSTATION_HEAD"
    ) {

        const headCenterId =
            await resolveHeadCenterId(
                user
            );

        const requestCenterId =
            objectIdToString(
                request.assignedCenter ||
                request.workstationCenter
            );

        if (
            headCenterId &&
            requestCenterId &&
            headCenterId === requestCenterId
        ) {
            return;
        }

        throw new AppError(
            "Access denied",
            403
        );
    }

    throw new AppError(
        "Access denied",
        403
    );
};


// ======================================================
// FORENSIC ACCESS CONTROL
// ======================================================

const ensureForensicAccess = async (
    forensicCase,
    user
) => {

    if (!forensicCase) {
        throw new AppError(
            "Forensic case not found",
            404
        );
    }

    if (
        !user ||
        !user._id ||
        !user.role
    ) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    const userId =
        objectIdToString(
            user._id
        );

    // --------------------------------------------------
    // ADMIN
    // --------------------------------------------------

    if (
        user.role === "ADMIN"
    ) {
        return;
    }

    // --------------------------------------------------
    // CUSTOMER
    // --------------------------------------------------

    if (
        user.role === "CUSTOMER"
    ) {

        if (
            objectIdToString(
                forensicCase.customer
            ) === userId
        ) {
            return;
        }

        throw new AppError(
            "Access denied",
            403
        );
    }

    // --------------------------------------------------
    // WORKSTATION EMPLOYEE
    // --------------------------------------------------

    if (
        user.role === "WORKSTATION_EMPLOYEE"
    ) {

        if (
            objectIdToString(
                forensicCase.assignedEmployee
            ) === userId
        ) {
            return;
        }

        throw new AppError(
            "Access denied",
            403
        );
    }

    // --------------------------------------------------
    // WORKSTATION HEAD
    // --------------------------------------------------

    if (
        user.role === "WORKSTATION_HEAD"
    ) {

        const headCenterId =
            await resolveHeadCenterId(
                user
            );

        const caseCenterId =
            objectIdToString(
                forensicCase.workstationCenter
            );

        if (
            headCenterId &&
            caseCenterId &&
            headCenterId === caseCenterId
        ) {
            return;
        }

        throw new AppError(
            "Access denied",
            403
        );
    }

    throw new AppError(
        "Access denied",
        403
    );
};


// ======================================================
// UPDATE SANITIZATION LIVE PROGRESS
// ======================================================

const updateSanitizationProgress = async (
    requestId,
    payload,
    user
) => {

    const cleanRequestId =
        normalizeString(
            requestId
        );

    if (!cleanRequestId) {
        throw new AppError(
            "Sanitization request ID is required",
            400
        );
    }

    if (
        !mongoose.Types.ObjectId.isValid(
            user?._id
        )
    ) {
        throw new AppError(
            "Invalid authenticated user",
            401
        );
    }

    const request =
        await SanitizationRequest.findOne({
            requestId:
                cleanRequestId
        });

    await ensureSanitizationAccess(
        request,
        user
    );

    /*
     * --------------------------------------------------
     * IMPORTANT SECURITY RULE
     *
     * A customer must NEVER be allowed to write
     * progress into a sanitization request.
     *
     * The CLI/backend execution identity should be:
     * ADMIN, WORKSTATION_EMPLOYEE, or WORKSTATION_HEAD.
     * --------------------------------------------------
     */

    if (
        user.role === "CUSTOMER"
    ) {
        throw new AppError(
            "Customers cannot update operation progress",
            403
        );
    }


    const processedBytes =
        normalizeNonNegativeNumber(
            payload?.processedBytes
        );

    const totalBytes =
        normalizeNonNegativeNumber(
            payload?.totalBytes
        );

    /*
     * If totalBytes is known, calculate the percentage
     * from the actual byte counters.
     *
     * This prevents a CLI from reporting:
     *
     * processedBytes = 30 GB
     * totalBytes = 500 GB
     * percentage = 99
     *
     * The server derives the percentage itself.
     */

    let progressKnown =
        totalBytes > 0;

    let progress =
        progressKnown
            ? (
                processedBytes >= totalBytes
                    ? 100
                    : (
                        processedBytes /
                        totalBytes
                    ) * 100
            )
            : 0;

    /*
     * For methods such as ATA where the exact byte
     * percentage is unavailable, the client can send
     * progressKnown=false.
     *
     * In that case the website should display an
     * indeterminate progress state.
     */

    if (
        payload &&
        payload.progressKnown === false
    ) {
        progressKnown = false;
        progress = 0;
    }

    /*
     * Explicit percentage is only used when the sender
     * has no byte total.
     *
     * This is useful for method-selection,
     * verification, certificate-generation, etc.
     */

    if (
        !progressKnown &&
        payload &&
        payload.progress !== undefined
    ) {

        progress =
            clampPercentage(
                payload.progress
            );
    }


    const remainingBytes =
        progressKnown
            ? Math.max(
                0,
                totalBytes -
                processedBytes
            )
            : 0;


    const phase =
        normalizeString(
            payload?.phase
        );

    const progressMessage =
        normalizeString(
            payload?.message
        );

    const operationId =
        normalizeString(
            payload?.operationId
        );


    const update = {
        progress,
        progressKnown,

        processedBytes,
        totalBytes,
        remainingBytes,

        phase,
        progressMessage,

        lastProgressAt:
            new Date()
    };


    if (operationId) {
        update.operationId =
            operationId;
    }


    /*
     * --------------------------------------------------
     * STATUS SYNCHRONIZATION
     * --------------------------------------------------
     */

    const requestedStatus =
        normalizeString(
            payload?.status
        ).toUpperCase();

    const allowedStatuses = [
        "PENDING",
        "APPROVED",
        "REJECTED",
        "ASSIGNED",
        "IN_PROGRESS",
        "VERIFYING",
        "COMPLETED",
        "FAILED",
        "CANCELLED"
    ];

    if (
        allowedStatuses.includes(
            requestedStatus
        )
    ) {

        update.status =
            requestedStatus;

        if (
            requestedStatus ===
            "IN_PROGRESS" &&
            !request.startedAt
        ) {
            update.startedAt =
                new Date();
        }

        if (
            [
                "COMPLETED",
                "FAILED",
                "CANCELLED"
            ].includes(
                requestedStatus
            )
        ) {
            update.completedAt =
                requestedStatus ===
                "COMPLETED"
                    ? new Date()
                    : request.completedAt;
        }
    }


    /*
     * COMPLETED means actual operation reached
     * completion.
     *
     * Only set 100 automatically when the sender
     * explicitly marks it completed.
     */

    if (
        requestedStatus ===
        "COMPLETED"
    ) {

        update.progress =
            100;

        update.progressKnown =
            true;

        if (
            totalBytes > 0 &&
            processedBytes < totalBytes
        ) {
            update.processedBytes =
                totalBytes;

            update.remainingBytes =
                0;
        }
    }


    const updatedRequest =
        await SanitizationRequest.findOneAndUpdate(
            {
                requestId:
                    cleanRequestId
            },
            {
                $set:
                    update
            },
            {
                new: true,
                runValidators: true
            }
        )
            .populate(
                "customer",
                "name email role"
            )
            .populate(
                "assignedEmployee",
                "name email role"
            )
            .populate(
                "assignedWorkstation",
                "workstationId name status"
            );


    if (!updatedRequest) {
        throw new AppError(
            "Sanitization request disappeared while updating progress",
            404
        );
    }


    return updatedRequest;
};


// ======================================================
// GET SANITIZATION LIVE PROGRESS
// ======================================================

const getSanitizationProgress = async (
    requestId,
    user
) => {

    const cleanRequestId =
        normalizeString(
            requestId
        );

    if (!cleanRequestId) {
        throw new AppError(
            "Sanitization request ID is required",
            400
        );
    }

    const request =
        await SanitizationRequest.findOne({
            requestId:
                cleanRequestId
        })
            .populate(
                "customer",
                "name email role"
            )
            .populate(
                "assignedEmployee",
                "name email role"
            )
            .populate(
                "assignedWorkstation",
                "workstationId name status"
            )
            .lean();

    await ensureSanitizationAccess(
        request,
        user
    );


    return {
        requestId:
            request.requestId,

        status:
            request.status,

        progress:
            request.progress,

        progressKnown:
            request.progressKnown,

        processedBytes:
            request.processedBytes,

        totalBytes:
            request.totalBytes,

        remainingBytes:
            request.remainingBytes,

        phase:
            request.phase,

        message:
            request.progressMessage,

        operationId:
            request.operationId,

        startedAt:
            request.startedAt,

        completedAt:
            request.completedAt,

        lastProgressAt:
            request.lastProgressAt
    };
};


// ======================================================
// UPDATE FORENSIC LIVE PROGRESS
// ======================================================

const updateForensicProgress = async (
    caseId,
    payload,
    user
) => {

    const cleanCaseId =
        normalizeString(
            caseId
        );

    if (!cleanCaseId) {
        throw new AppError(
            "Forensic case ID is required",
            400
        );
    }


    const forensicCase =
        await ForensicCase.findOne({
            caseId:
                cleanCaseId
        });


    await ensureForensicAccess(
        forensicCase,
        user
    );


    /*
     * A customer can view progress but cannot
     * modify the running forensic operation.
     */

    if (
        user.role === "CUSTOMER"
    ) {
        throw new AppError(
            "Customers cannot update forensic progress",
            403
        );
    }


    const bytesScanned =
        normalizeNonNegativeNumber(
            payload?.bytesScanned
        );

    const totalBytes =
        normalizeNonNegativeNumber(
            payload?.totalBytes
        );


    let progressKnown =
        totalBytes > 0;

    let progress =
        progressKnown
            ? (
                bytesScanned >= totalBytes
                    ? 100
                    : (
                        bytesScanned /
                        totalBytes
                    ) * 100
            )
            : 0;


    if (
        payload &&
        payload.progressKnown === false
    ) {
        progressKnown = false;
        progress = 0;
    }


    if (
        !progressKnown &&
        payload &&
        payload.progress !== undefined
    ) {
        progress =
            clampPercentage(
                payload.progress
            );
    }


    const candidatesFound =
        normalizeNonNegativeNumber(
            payload?.candidatesFound
        );

    const recoveredArtifacts =
        normalizeNonNegativeNumber(
            payload?.recoveredArtifacts
        );

    const validatedArtifacts =
        normalizeNonNegativeNumber(
            payload?.validatedArtifacts
        );

    const rejectedArtifacts =
        normalizeNonNegativeNumber(
            payload?.rejectedArtifacts
        );

    const highConfidenceArtifacts =
        normalizeNonNegativeNumber(
            payload?.highConfidenceArtifacts
        );

    const recoveredBytes =
        normalizeNonNegativeNumber(
            payload?.recoveredBytes
        );


    const update = {
        progress,
        bytesScanned,
        totalBytes,

        candidatesFound,
        recoveredArtifacts,
        validatedArtifacts,
        rejectedArtifacts,
        highConfidenceArtifacts,
        recoveredBytes
    };


    /*
     * The following fields are added to ForensicCase
     * by the next model update in this implementation.
     *
     * Keeping them here means the endpoint and model
     * stay aligned.
     */

    if (
        payload?.phase !== undefined
    ) {
        update.progressPhase =
            normalizeString(
                payload.phase
            );
    }

    if (
        payload?.message !== undefined
    ) {
        update.progressMessage =
            normalizeString(
                payload.message
            );
    }

    update.progressKnown =
        progressKnown;

    update.lastProgressAt =
        new Date();


    const requestedStatus =
        normalizeString(
            payload?.status
        ).toUpperCase();


    const allowedStatuses = [
        "PENDING",
        "ASSIGNED",
        "ACQUIRING",
        "ANALYZING",
        "COMPLETED",
        "FAILED",
        "CANCELLED"
    ];


    if (
        allowedStatuses.includes(
            requestedStatus
        )
    ) {

        update.status =
            requestedStatus;

        if (
            requestedStatus ===
                "ACQUIRING" &&
            !forensicCase.startedAt
        ) {
            update.startedAt =
                new Date();
        }

        if (
            requestedStatus ===
            "COMPLETED"
        ) {

            update.progress =
                100;

            update.progressKnown =
                true;

            update.bytesScanned =
                totalBytes > 0
                    ? totalBytes
                    : bytesScanned;

            update.completedAt =
                new Date();
        }

        if (
            requestedStatus ===
            "FAILED"
        ) {
            update.failedAt =
                new Date();
        }
    }


    const updatedCase =
        await ForensicCase.findOneAndUpdate(
            {
                caseId:
                    cleanCaseId
            },
            {
                $set:
                    update
            },
            {
                new: true,
                runValidators: true
            }
        )
            .populate(
                "customer",
                "name email role"
            )
            .populate(
                "assignedEmployee",
                "name email role"
            )
            .populate(
                "workstationCenter",
                "centerId name"
            );


    if (!updatedCase) {
        throw new AppError(
            "Forensic case disappeared while updating progress",
            404
        );
    }


    return updatedCase;
};


// ======================================================
// GET FORENSIC LIVE PROGRESS
// ======================================================

const getForensicProgress = async (
    caseId,
    user
) => {

    const cleanCaseId =
        normalizeString(
            caseId
        );


    if (!cleanCaseId) {
        throw new AppError(
            "Forensic case ID is required",
            400
        );
    }


    const forensicCase =
        await ForensicCase.findOne({
            caseId:
                cleanCaseId
        })
            .populate(
                "customer",
                "name email role"
            )
            .populate(
                "assignedEmployee",
                "name email role"
            )
            .populate(
                "workstationCenter",
                "centerId name"
            )
            .lean();


    await ensureForensicAccess(
        forensicCase,
        user
    );


    return {
        caseId:
            forensicCase.caseId,

        status:
            forensicCase.status,

        progress:
            forensicCase.progress,

        progressKnown:
            forensicCase.progressKnown !== false,

        bytesScanned:
            forensicCase.bytesScanned,

        totalBytes:
            forensicCase.totalBytes,

        remainingBytes:
            Math.max(
                0,
                (
                    forensicCase.totalBytes || 0
                ) -
                (
                    forensicCase.bytesScanned || 0
                )
            ),

        candidatesFound:
            forensicCase.candidatesFound,

        recoveredArtifacts:
            forensicCase.recoveredArtifacts,

        validatedArtifacts:
            forensicCase.validatedArtifacts,

        rejectedArtifacts:
            forensicCase.rejectedArtifacts,

        highConfidenceArtifacts:
            forensicCase.highConfidenceArtifacts,

        recoveredBytes:
            forensicCase.recoveredBytes,

        phase:
            forensicCase.progressPhase ||
            "",

        message:
            forensicCase.progressMessage ||
            "",

        startedAt:
            forensicCase.startedAt,

        completedAt:
            forensicCase.completedAt,

        failedAt:
            forensicCase.failedAt,

        lastProgressAt:
            forensicCase.lastProgressAt
    };
};


module.exports = {
    updateSanitizationProgress,
    getSanitizationProgress,

    updateForensicProgress,
    getForensicProgress
};