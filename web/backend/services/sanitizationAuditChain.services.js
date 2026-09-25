const crypto = require("crypto");

const SanitizationAuditChain =
    require("../models/SanitizationAuditChain");

const SanitizationCertificate =
    require("../models/SanitizationCertificate");

const SanitizationRequest =
    require("../models/SanitizationRequest");

const Workstation =
    require("../models/WorkStation");

const AppError =
    require("../utils/AppError");

/*
 * ============================================================
 * HELPERS
 * ============================================================
 */

const toStringValue = value => {
    if (
        value === undefined ||
        value === null
    ) {
        return "";
    }

    return String(value);
};

const toNumberString = value => {
    if (
        value === undefined ||
        value === null
    ) {
        return "0";
    }

    return String(value);
};

const calculateSha256 = data => {
    return crypto
        .createHash("sha256")
        .update(data, "utf8")
        .digest("hex");
};

/*
 * This MUST stay compatible with:
 *
 * desktop/backend/sanitization/src/SanitizationEvent.cpp
 *
 * Specifically:
 *
 * SanitizationEvent::canonicalData()
 *
 * Current native canonical format:
 *
 * eventId
 * timestampUtc
 * eventType
 * severity
 * operationId
 * requestId
 * actorId
 * deviceId
 * model
 * serialNumber
 * interfaceType
 * capacityBytes
 * method
 * sanitizationStatus
 * verificationStatus
 * bytesProcessed
 * bytesVerified
 * verificationSamples
 * error
 * nativeErrorCode
 * safetyDecision
 * safetySummary
 * safetyCheck
 * message
 * previousEventHash
 *
 * NOTE:
 * The current C++ SanitizationEvent::canonicalData()
 * does NOT include workstationId.
 *
 * Therefore DO NOT add workstationId here.
 */
const buildCanonicalAuditData = event => {
    const lines = [];

    lines.push(
        `eventId=${toStringValue(event.eventId)}`
    );

    lines.push(
        `timestampUtc=${toStringValue(
            event.timestampUtc
        )}`
    );

    lines.push(
        `eventType=${toStringValue(
            event.eventType
        )}`
    );

    lines.push(
        `severity=${toStringValue(
            event.severity
        )}`
    );

    lines.push(
        `operationId=${toStringValue(
            event.operationId
        )}`
    );

    lines.push(
        `requestId=${toStringValue(
            event.requestId
        )}`
    );

    lines.push(
        `actorId=${toStringValue(
            event.actorId
        )}`
    );

    lines.push(
        `deviceId=${toStringValue(
            event.deviceId
        )}`
    );

    lines.push(
        `model=${toStringValue(
            event.model
        )}`
    );

    lines.push(
        `serialNumber=${toStringValue(
            event.serialNumber
        )}`
    );

    lines.push(
        `interfaceType=${toStringValue(
            event.interfaceType
        )}`
    );

    lines.push(
        `capacityBytes=${toNumberString(
            event.capacityBytes
        )}`
    );

    lines.push(
        `method=${toStringValue(
            event.method
        )}`
    );

    lines.push(
        `sanitizationStatus=${toStringValue(
            event.sanitizationStatus
        )}`
    );

    lines.push(
        `verificationStatus=${toStringValue(
            event.verificationStatus
        )}`
    );

    lines.push(
        `bytesProcessed=${toNumberString(
            event.bytesProcessed
        )}`
    );

    lines.push(
        `bytesVerified=${toNumberString(
            event.bytesVerified
        )}`
    );

    lines.push(
        `verificationSamples=${toNumberString(
            event.verificationSamples
        )}`
    );

    lines.push(
        `error=${toStringValue(
            event.error
        )}`
    );

    lines.push(
        `nativeErrorCode=${toNumberString(
            event.nativeErrorCode
        )}`
    );

    lines.push(
        `safetyDecision=${toStringValue(
            event.safetyDecision
        )}`
    );

    lines.push(
        `safetySummary=${toStringValue(
            event.safetySummary
        )}`
    );

    /*
     * C++ writes:
     *
     * safetyCheck=<name>|<true/false>|<message>
     *
     * in array order.
     */
    const safetyChecks =
        Array.isArray(event.safetyChecks)
            ? event.safetyChecks
            : [];

    for (
        const check of safetyChecks
    ) {
        lines.push(
            `safetyCheck=${toStringValue(
                check?.name
            )}|${
                check?.passed === true
                    ? "true"
                    : "false"
            }|${toStringValue(
                check?.message
            )}`
        );
    }

    lines.push(
        `message=${toStringValue(
            event.message
        )}`
    );

    lines.push(
        `previousEventHash=${toStringValue(
            event.previousEventHash
        )}`
    );

    /*
     * C++ canonicalData() returns the complete
     * string with a final newline.
     */
    return lines.join("\n") + "\n";
};

/*
 * ============================================================
 * JSONL PARSING
 * ============================================================
 */

const parseAuditJsonl = auditLog => {
    if (
        typeof auditLog !==
        "string"
    ) {
        throw new AppError(
            "auditLog must be a JSONL string",
            400
        );
    }

    if (
        auditLog.trim().length === 0
    ) {
        throw new AppError(
            "Audit log is empty",
            400
        );
    }

    const lines =
        auditLog.split(/\r?\n/);

    const events = [];

    for (
        let index = 0;
        index < lines.length;
        index++
    ) {
        const line =
            lines[index].trim();

        /*
         * C++ JSONL writer leaves normal empty lines
         * out, but ignoring an empty trailing line is
         * safe.
         */
        if (!line) {
            continue;
        }

        let event;

        try {
            event =
                JSON.parse(line);
        } catch (error) {
            throw new AppError(
                `Invalid JSON at audit log line ${index + 1}`,
                400
            );
        }

        if (
            !event ||
            typeof event !==
                "object" ||
            Array.isArray(event)
        ) {
            throw new AppError(
                `Audit event at line ${index + 1} is not a JSON object`,
                400
            );
        }

        events.push(event);
    }

    if (
        events.length === 0
    ) {
        throw new AppError(
            "Audit log contains no events",
            400
        );
    }

    return events;
};

/*
 * ============================================================
 * REQUIRED EVENT FIELDS
 * ============================================================
 *
 * These correspond to fields that the native
 * AuditChainVerifier requires before reconstructing
 * canonical data.
 */

const validateEventStructure = (
    event,
    index
) => {
    const requiredStringFields = [
        "eventId",
        "timestampUtc",
        "eventType",
        "severity",
        "operationId",
        "requestId",
        "actorId",
        "deviceId",
        "model",
        "serialNumber",
        "interfaceType",
        "method",
        "sanitizationStatus",
        "verificationStatus",
        "error",
        "safetyDecision",
        "safetySummary",
        "message",
        "previousEventHash",
        "eventHash"
    ];

    for (
        const field of
            requiredStringFields
    ) {
        if (
            typeof event[field] !==
            "string"
        ) {
            throw new AppError(
                `Audit event ${index}: field '${field}' must be a string`,
                400
            );
        }
    }

    const requiredNumericFields = [
        "capacityBytes",
        "bytesProcessed",
        "bytesVerified",
        "verificationSamples",
        "nativeErrorCode"
    ];

    for (
        const field of
            requiredNumericFields
    ) {
        const value =
            event[field];

        if (
            typeof value !==
            "number" ||
            !Number.isFinite(value)
        ) {
            throw new AppError(
                `Audit event ${index}: field '${field}' must be a finite number`,
                400
            );
        }
    }

    if (
        !Array.isArray(
            event.safetyChecks
        )
    ) {
        throw new AppError(
            `Audit event ${index}: safetyChecks must be an array`,
            400
        );
    }

    for (
        let checkIndex = 0;
        checkIndex <
            event.safetyChecks.length;
        checkIndex++
    ) {
        const check =
            event.safetyChecks[
                checkIndex
            ];

        if (
            !check ||
            typeof check !==
                "object" ||
            Array.isArray(check)
        ) {
            throw new AppError(
                `Audit event ${index}: safety check ${checkIndex + 1} is invalid`,
                400
            );
        }

        if (
            typeof check.name !==
            "string"
        ) {
            throw new AppError(
                `Audit event ${index}: safety check ${checkIndex + 1} name must be a string`,
                400
            );
        }

        if (
            typeof check.passed !==
            "boolean"
        ) {
            throw new AppError(
                `Audit event ${index}: safety check ${checkIndex + 1} passed must be boolean`,
                400
            );
        }

        if (
            typeof check.message !==
            "string"
        ) {
            throw new AppError(
                `Audit event ${index}: safety check ${checkIndex + 1} message must be a string`,
                400
            );
        }
    }
};

/*
 * ============================================================
 * VERIFY COMPLETE NATIVE AUDIT LEDGER
 * ============================================================
 *
 * The native C++ AuditLogger maintains a persistent
 * JSONL ledger. Therefore the complete uploaded file
 * is verified before request-specific events are
 * selected for display.
 */

const verifyCompleteAuditLedger =
    events => {
        let previousEventHash = "";

        let firstEvent = true;

        let firstEventPreviousHash =
            "";

        let finalEventHash = "";

        let verifiedEventCount = 0;

        for (
            let index = 0;
            index < events.length;
            index++
        ) {
            const event =
                events[index];

            const lineNumber =
                index + 1;

            validateEventStructure(
                event,
                lineNumber
            );

            const storedHash =
                event.eventHash;

            const eventPreviousHash =
                event.previousEventHash;

            if (
                firstEvent
            ) {
                firstEventPreviousHash =
                    eventPreviousHash;

                firstEvent = false;
            } else {
                /*
                 * Exact equivalent of the native
                 * AuditChainVerifier linkage check.
                 */
                if (
                    eventPreviousHash !==
                    previousEventHash
                ) {
                    return {
                        valid: false,

                        eventCount:
                            events.length,

                        verifiedEventCount,

                        failedEventIndex:
                            lineNumber,

                        firstEventPreviousHash,

                        expectedHash:
                            previousEventHash,

                        actualHash:
                            eventPreviousHash,

                        finalEventHash,

                        message:
                            "Audit chain linkage failed: previousEventHash does not match the preceding eventHash."
                    };
                }
            }

            const canonicalData =
                buildCanonicalAuditData(
                    event
                );

            const calculatedHash =
                calculateSha256(
                    canonicalData
                );

            /*
             * The native verifier compares the
             * lowercase hexadecimal hash exactly.
             */
            if (
                calculatedHash !==
                storedHash
            ) {
                return {
                    valid: false,

                    eventCount:
                        events.length,

                    verifiedEventCount,

                    failedEventIndex:
                        lineNumber,

                    firstEventPreviousHash,

                    expectedHash:
                        calculatedHash,

                    actualHash:
                        storedHash,

                    finalEventHash,

                    message:
                        "Audit event SHA-256 hash mismatch."
                };
            }

            previousEventHash =
                storedHash;

            finalEventHash =
                storedHash;

            verifiedEventCount++;
        }

        return {
            valid: true,

            eventCount:
                events.length,

            verifiedEventCount,

            failedEventIndex:
                0,

            firstEventPreviousHash,

            expectedHash:
                "",

            actualHash:
                "",

            finalEventHash,

            message:
                "Audit chain verification passed for all events."
        };
    };

/*
 * ============================================================
 * REQUEST ACCESS
 * ============================================================
 */

const getRequestForAuditAccess =
    async (
        requestId,
        user,
        requireSubmitAccess
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
            requireSubmitAccess &&
            ![
                "WORKSTATION_EMPLOYEE",
                "ADMIN"
            ].includes(
                user.role
            )
        ) {
            throw new AppError(
                "Only the assigned workstation employee or admin can submit sanitization audit evidence",
                403
            );
        }

        if (
            user.role ===
                "CUSTOMER" &&
            request.customer?.toString() !==
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
            request.workstationCenter?.toString() !==
                user.workstationCenter?.toString()
        ) {
            throw new AppError(
                "Access denied",
                403
            );
        }

        return request;
    };

/*
 * ============================================================
 * SUBMIT
 * ============================================================
 */

const submitAuditChain = async (
    requestId,
    payload,
    user
) => {
    const request =
        await getRequestForAuditAccess(
            requestId,
            user,
            true
        );

    if (
        !payload ||
        typeof payload !==
            "object"
    ) {
        throw new AppError(
            "Audit chain payload is required",
            400
        );
    }

    if (
        typeof payload.auditLog !==
        "string"
    ) {
        throw new AppError(
            "auditLog must be a JSONL string",
            400
        );
    }

    /*
     * Prevent accidentally sending a huge request.
     */
    const auditSize =
        Buffer.byteLength(
            payload.auditLog,
            "utf8"
        );

    if (
        auditSize >
        10 * 1024 * 1024
    ) {
        throw new AppError(
            "Audit log exceeds the 10 MB upload limit",
            413
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

    if (
        !assignedWorkstation
    ) {
        throw new AppError(
            "The workstation assigned to this sanitization request no longer exists",
            409
        );
    }

    if (
        assignedWorkstation.status !==
        "ACTIVE"
    ) {
        throw new AppError(
            "The assigned workstation is not active",
            409
        );
    }

    if (
        user.role ===
            "WORKSTATION_EMPLOYEE" &&
        (
            !assignedWorkstation.assignedEmployee ||
            assignedWorkstation.assignedEmployee.toString() !==
                user._id.toString()
        )
    ) {
        throw new AppError(
            "The assigned workstation is not bound to the authenticated employee",
            403
        );
    }

    /*
     * The certificate must already exist.
     *
     * This ensures:
     *
     * sanitization
     *       ↓
     * verification
     *       ↓
     * certificate
     *       ↓
     * audit publication
     */
    const certificate =
        await SanitizationCertificate.findOne({
            requestId
        }).sort({
            createdAt: -1
        });

    if (!certificate) {
        throw new AppError(
            "Sanitization certificate must exist before audit evidence can be uploaded",
            409
        );
    }

    if (
        certificate.status !==
        "COMPLETED"
    ) {
        throw new AppError(
            "Audit evidence can only be uploaded for a completed certificate",
            400
        );
    }

    if (
        certificate.verificationStatus !==
        "PASSED" ||
        !certificate.verificationPerformed ||
        !certificate.verificationPassed
    ) {
        throw new AppError(
            "The associated certificate does not contain successful verification evidence",
            400
        );
    }

    if (
        certificate.workstationId !==
        assignedWorkstation.workstationId
    ) {
        throw new AppError(
            "Certificate workstation does not match the assigned workstation",
            403
        );
    }

    /*
     * Parse the COMPLETE native JSONL ledger.
     */
    const events =
        parseAuditJsonl(
            payload.auditLog
        );

    /*
     * First verify the COMPLETE global chain.
     *
     * Do not filter events before this step because
     * the native audit logger creates one continuous
     * chain across operations.
     */
    const verification =
        verifyCompleteAuditLedger(
            events
        );

    if (
        !verification.valid
    ) {
        throw new AppError(
            verification.message,
            400
        );
    }

    /*
     * Find events belonging to THIS certificate
     * operation/request.
     */
    const requestEvents =
        events.filter(
            event =>
                event.requestId ===
                    request.requestId &&
                event.operationId ===
                    certificate.operationId
        );

    if (
        requestEvents.length ===
        0
    ) {
        throw new AppError(
            "The uploaded audit ledger does not contain events for this sanitization operation",
            400
        );
    }

    /*
     * Ensure the upload's explicit metadata, when
     * supplied, matches the certificate.
     */
    if (
        payload.operationId &&
        String(
            payload.operationId
        ) !==
            certificate.operationId
    ) {
        throw new AppError(
            "Uploaded audit operationId does not match the certificate",
            400
        );
    }

    if (
        payload.certificateId &&
        String(
            payload.certificateId
        ) !==
            certificate.certificateId
    ) {
        throw new AppError(
            "Uploaded audit certificateId does not match the certificate",
            400
        );
    }

    if (
        payload.workstationId &&
        String(
            payload.workstationId
        ) !==
            assignedWorkstation.workstationId
    ) {
        throw new AppError(
            "Uploaded workstationId does not match the assigned workstation",
            403
        );
    }

    /*
     * Prevent duplicate publication for one request.
     */
    const existing =
        await SanitizationAuditChain.findOne({
            requestId:
                request.requestId
        });

    if (existing) {
        throw new AppError(
            "Audit chain evidence already exists for this sanitization request",
            409
        );
    }

    /*
     * Store the COMPLETE verified ledger.
     *
     * This is important because the native chain can
     * continue across operations. Keeping the complete
     * verified ledger preserves the real cryptographic
     * linkage instead of creating a fake isolated chain.
     */
    const auditChain =
        await SanitizationAuditChain.create({
            requestId:
                request.requestId,

            operationId:
                certificate.operationId,

            certificateId:
                certificate.certificateId,

            workstationId:
                assignedWorkstation.workstationId,

            uploadedBy:
                user._id,

            firstEventPreviousHash:
                verification.firstEventPreviousHash,

            finalEventHash:
                verification.finalEventHash,

            eventCount:
                verification.eventCount,

            verifiedEventCount:
                verification.verifiedEventCount,

            chainValid:
                true,

            verificationMessage:
                verification.message,

            verifiedAt:
                new Date(),

            events
        });

    return {
        auditChain,

        requestEventCount:
            requestEvents.length,

        globalEventCount:
            verification.eventCount,

        globalVerifiedEventCount:
            verification.verifiedEventCount,

        globalChainValid:
            true,

        message:
            "Complete native audit ledger verified and stored successfully."
    };
};

/*
 * ============================================================
 * GET
 * ============================================================
 */

const getAuditChainByRequest =
    async (
        requestId,
        user
    ) => {
        const request =
            await getRequestForAuditAccess(
                requestId,
                user,
                false
            );

        const auditChain =
            await SanitizationAuditChain.findOne({
                requestId:
                    request.requestId
            }).populate(
                "uploadedBy",
                "name email role"
            );

        if (!auditChain) {
            throw new AppError(
                "Sanitization audit chain not found",
                404
            );
        }

        /*
         * Website should display only the events
         * belonging to this sanitization operation.
         *
         * The stored document still contains the
         * COMPLETE verified ledger.
         */
        const requestEvents =
            auditChain.events.filter(
                event =>
                    event.requestId ===
                        auditChain.requestId &&
                    event.operationId ===
                        auditChain.operationId
            );

        const firstRequestEvent =
            requestEvents[0];

        const lastRequestEvent =
            requestEvents[
                requestEvents.length - 1
            ];

        return {
            _id:
                auditChain._id,

            requestId:
                auditChain.requestId,

            operationId:
                auditChain.operationId,

            certificateId:
                auditChain.certificateId,

            workstationId:
                auditChain.workstationId,

            uploadedBy:
                auditChain.uploadedBy,

            /*
             * These hashes refer to the request-specific
             * portion displayed on the website.
             */
            firstEventPreviousHash:
                firstRequestEvent?.previousEventHash ||
                "",

            finalEventHash:
                lastRequestEvent?.eventHash ||
                "",

            eventCount:
                requestEvents.length,

            verifiedEventCount:
                requestEvents.length,

            /*
             * This reflects the cryptographic verification
             * of the COMPLETE native ledger stored in MongoDB.
             */
            ledgerEventCount:
                auditChain.eventCount,

            ledgerVerifiedEventCount:
                auditChain.verifiedEventCount,

            chainValid:
                auditChain.chainValid,

            verificationMessage:
                auditChain.verificationMessage,

            verifiedAt:
                auditChain.verifiedAt,

            events:
                requestEvents,

            createdAt:
                auditChain.createdAt,

            updatedAt:
                auditChain.updatedAt
        };
    };

/*
 * ============================================================
 * VERIFY STORED LEDGER
 * ============================================================
 */

const verifyStoredAuditChain =
    async (
        requestId,
        user
    ) => {
        const request =
            await getRequestForAuditAccess(
                requestId,
                user,
                false
            );

        const auditChain =
            await SanitizationAuditChain.findOne({
                requestId:
                    request.requestId
            });

        if (!auditChain) {
            throw new AppError(
                "Sanitization audit chain not found",
                404
            );
        }

        /*
         * Re-verify the COMPLETE ledger that was stored.
         */
        const verification =
            verifyCompleteAuditLedger(
                auditChain.events
            );

        const metadataMatches =
            verification.valid &&
            verification.eventCount ===
                auditChain.eventCount &&
            verification.verifiedEventCount ===
                auditChain.verifiedEventCount &&
            verification.firstEventPreviousHash ===
                auditChain.firstEventPreviousHash &&
            verification.finalEventHash ===
                auditChain.finalEventHash &&
            auditChain.chainValid ===
                true;

        const valid =
            verification.valid &&
            metadataMatches;

        const requestEvents =
            auditChain.events.filter(
                event =>
                    event.requestId ===
                        auditChain.requestId &&
                    event.operationId ===
                        auditChain.operationId
            );

        return {
            requestId:
                auditChain.requestId,

            operationId:
                auditChain.operationId,

            certificateId:
                auditChain.certificateId,

            workstationId:
                auditChain.workstationId,

            eventCount:
                requestEvents.length,

            verifiedEventCount:
                valid
                    ? requestEvents.length
                    : 0,

            ledgerEventCount:
                verification.eventCount,

            ledgerVerifiedEventCount:
                verification.verifiedEventCount,

            firstEventPreviousHash:
                requestEvents[0]
                    ?.previousEventHash ||
                "",

            finalEventHash:
                requestEvents[
                    requestEvents.length - 1
                ]?.eventHash ||
                "",

            storedFirstEventPreviousHash:
                auditChain.firstEventPreviousHash,

            storedFinalEventHash:
                auditChain.finalEventHash,

            storedChainValid:
                auditChain.chainValid,

            metadataMatches,

            valid,

            failedEventIndex:
                verification.failedEventIndex,

            expectedHash:
                verification.expectedHash,

            actualHash:
                verification.actualHash,

            message:
                valid
                    ? "Stored audit ledger SHA-256 hashes and hash links verified successfully."
                    : verification.message,

            verifiedAt:
                new Date()
        };
    };

module.exports = {
    submitAuditChain,
    getAuditChainByRequest,
    verifyStoredAuditChain
};