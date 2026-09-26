const crypto = require("crypto");
const mongoose = require("mongoose");
const { Readable } = require("stream");

const ForensicCase = require("../models/ForensicCase");
const ForensicIntegrityLog = require("../models/ForensicIntegrityLog");
const ForensicAuditLog = require("../models/ForensicAuditLog");
const Workstation = require("../models/WorkStation");
const WorkstationCenter = require("../models/WorkstationCenter");
const Counter = require("../models/Counter");
const AppError = require("../utils/AppError");

const MAX_UPLOAD_BYTES =
    20 * 1024 * 1024;

const ALLOWED_CONFIDENCE =
    new Set([
        "HIGH",
        "MEDIUM",
        "LOW",
        "REJECTED"
    ]);

const generateAuditId = async () => {
    const counter =
        await Counter.findOneAndUpdate(
            {
                name: "forensicAudit"
            },
            {
                $inc: {
                    sequence: 1
                }
            },
            {
                new: true,
                upsert: true
            }
        );

    return `FA-${String(
        counter.sequence
    ).padStart(8, "0")}`;
};

const sha256 = value =>
    crypto
        .createHash("sha256")
        .update(value)
        .digest("hex");

const normalizeCanonicalValue =
    value =>
        String(value ?? "")
            .replace(
                /[\r\n\0]/g,
                " "
            );

const getForensicBucket = () => {
    if (!mongoose.connection.db) {
        throw new AppError(
            "MongoDB is not ready for forensic evidence storage",
            503
        );
    }

    return new mongoose.mongo.GridFSBucket(
        mongoose.connection.db,
        {
            bucketName:
                "forensicEvidence"
        }
    );
};

const contentTypeFor =
    fileType => {
        switch (
            String(
                fileType || ""
            ).toUpperCase()
        ) {
        case "JPEG":
        case "JPG":
            return "image/jpeg";

        case "PNG":
            return "image/png";

        case "PDF":
            return "application/pdf";

        default:
            return "application/octet-stream";
        }
    };

const assertBase64 = value => {
    if (
        typeof value !==
            "string" ||
        !value.length
    ) {
        throw new AppError(
            "Artifact contentBase64 is required",
            400
        );
    }

    const compact =
        value.replace(
            /\s/g,
            ""
        );

    if (
        compact.length % 4 !== 0 ||
        !/^[A-Za-z0-9+/]*={0,2}$/.test(
            compact
        )
    ) {
        throw new AppError(
            "Artifact contentBase64 is invalid",
            400
        );
    }

    return compact;
};

const ensureCaseAccess = async (
    item,
    user
) => {
    if (!item) {
        throw new AppError(
            "Forensic case not found",
            404
        );
    }

    if (
        !user?._id ||
        !user?.role
    ) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    const userId =
        String(user._id);

    if (
        user.role ===
        "ADMIN"
    ) {
        return;
    }

    if (
        user.role ===
            "CUSTOMER" &&
        String(
            item.customer?._id ||
                item.customer
        ) === userId
    ) {
        return;
    }

    if (
        user.role ===
            "WORKSTATION_EMPLOYEE" &&
        String(
            item.assignedEmployee?._id ||
                item.assignedEmployee
        ) === userId
    ) {
        return;
    }

    if (
        user.role ===
        "WORKSTATION_HEAD"
    ) {
        const center =
            await WorkstationCenter.findOne(
                {
                    head: user._id,
                    status: "ACTIVE"
                }
            )
                .select("_id")
                .lean();

        if (
            center &&
            String(
                item.workstationCenter?._id ||
                    item.workstationCenter
            ) ===
                String(
                    center._id
                )
        ) {
            return;
        }
    }

    throw new AppError(
        "Access denied",
        403
    );
};

const getAssignedWorkstation =
    async (
        item,
        user,
        workstationId = ""
    ) => {
        if (
            !item.assignedWorkstation
        ) {
            throw new AppError(
                "No workstation is assigned to this forensic case",
                400
            );
        }

        if (!workstationId) {
            throw new AppError(
                "Workstation ID is required",
                400
            );
        }

        const workstation =
            await Workstation.findById(
                workstationId
            );

        if (!workstation) {
            throw new AppError(
                "Assigned workstation not found",
                404
            );
        }

        if (
            String(
                workstation._id
            ) !==
            String(
                item.assignedWorkstation?._id ||
                    item.assignedWorkstation
            )
        ) {
            throw new AppError(
                "This workstation is not assigned to the forensic case",
                403
            );
        }

        if (
            String(
                workstation.workstationCenter
            ) !==
            String(
                item.workstationCenter?._id ||
                    item.workstationCenter
            )
        ) {
            throw new AppError(
                "The workstation does not belong to the case workstation center",
                403
            );
        }

        if (
            workstation.status !==
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
            String(
                workstation.assignedEmployee ||
                    ""
            ) !==
                String(
                    user._id
                )
        ) {
            throw new AppError(
                "The assigned workstation is not bound to the authenticated employee",
                403
            );
        }

        return workstation;
    };

const uploadBufferToGridFs =
    async ({
        buffer,
        filename,
        contentType,
        metadata
    }) => {
        const bucket =
            getForensicBucket();

        return new Promise(
            (
                resolve,
                reject
            ) => {
                const upload =
                    bucket.openUploadStream(
                        filename,
                        {
                            contentType,
                            metadata
                        }
                    );

                upload.once(
                    "error",
                    reject
                );

                upload.once(
                    "finish",
                    () =>
                        resolve({
                            id: String(
                                upload.id
                            ),
                            size:
                                buffer.length
                        })
                );

                Readable
                    .from([
                        buffer
                    ])
                    .pipe(
                        upload
                    );
            }
        );
    };

const deleteGridFsFile =
    async storageId => {
        if (
            !storageId ||
            !mongoose.Types.ObjectId.isValid(
                storageId
            )
        ) {
            return;
        }

        try {
            await getForensicBucket().delete(
                new mongoose.Types.ObjectId(
                    storageId
                )
            );
        } catch {
            // Best-effort cleanup.
        }
    };

const readGridFsBuffer =
    async storageId => {
        if (
            !storageId ||
            !mongoose.Types.ObjectId.isValid(
                storageId
            )
        ) {
            throw new AppError(
                "Stored forensic artifact is unavailable",
                404
            );
        }

        return new Promise(
            (
                resolve,
                reject
            ) => {
                const chunks = [];

                const stream =
                    getForensicBucket()
                        .openDownloadStream(
                            new mongoose.Types.ObjectId(
                                storageId
                            )
                        );

                stream.on(
                    "data",
                    chunk =>
                        chunks.push(
                            chunk
                        )
                );

                stream.once(
                    "error",
                    reject
                );

                stream.once(
                    "end",
                    () =>
                        resolve(
                            Buffer.concat(
                                chunks
                            )
                        )
                );
            }
        );
    };

const canonicalAuditEvent = ({
    caseId,
    runId,
    workstationId,
    sourceIdentifier,
    event
}) =>
    [
        "schemaVersion=1",
        `caseId=${normalizeCanonicalValue(
            caseId
        )}`,
        `runId=${normalizeCanonicalValue(
            runId
        )}`,
        `workstationId=${normalizeCanonicalValue(
            workstationId
        )}`,
        `sourceIdentifier=${normalizeCanonicalValue(
            sourceIdentifier
        )}`,
        `sequence=${Number(
            event.sequence
        )}`,
        `eventType=${normalizeCanonicalValue(
            event.eventType
        )}`,
        `artifactId=${normalizeCanonicalValue(
            event.artifactId
        )}`,
        `timestampUtc=${normalizeCanonicalValue(
            event.timestampUtc
        )}`,
        `details=${normalizeCanonicalValue(
            event.details
        )}`,
        `previousEventHash=${normalizeCanonicalValue(
            event.previousEventHash
        )}`,
        ""
    ].join("\n");

const verifyNativeAuditEvents =
    ({
        caseId,
        runId,
        workstationId,
        sourceIdentifier,
        events
    }) => {
        if (
            !Array.isArray(
                events
            ) ||
            !events.length
        ) {
            return {
                valid: false,
                eventCount: 0,
                verifiedEventCount: 0,
                firstEventPreviousHash:
                    "",
                finalEventHash:
                    "",
                failedEventIndex:
                    0,
                expectedHash:
                    "",
                actualHash:
                    "",
                message:
                    "Native audit chain is empty"
            };
        }

        let previousHash =
            "";

        let verifiedEventCount =
            0;

        for (
            let index = 0;
            index <
            events.length;
            index += 1
        ) {
            const event =
                events[index];

            const sequence =
                index + 1;

            const actualSequence =
                Number(
                    event?.sequence
                );

            if (
                actualSequence !==
                sequence
            ) {
                return {
                    valid: false,
                    eventCount:
                        events.length,
                    verifiedEventCount,
                    firstEventPreviousHash:
                        String(
                            events[0]
                                ?.previousEventHash ||
                            ""
                        ),
                    finalEventHash:
                        String(
                            events[
                                events.length - 1
                            ]?.eventHash ||
                            ""
                        ),
                    failedEventIndex:
                        sequence,
                    expectedHash:
                        `SEQUENCE_${sequence}`,
                    actualHash:
                        String(
                            event?.sequence ||
                            ""
                        ),
                    message:
                        `Native audit sequence is invalid at event ${sequence}`
                };
            }

            const calculatedHash =
                sha256(
                    canonicalAuditEvent(
                        {
                            caseId,
                            runId,
                            workstationId,
                            sourceIdentifier,
                            event
                        }
                    )
                );

            const previousMatches =
                String(
                    event.previousEventHash ||
                        ""
                ) ===
                previousHash;

            const hashMatches =
                calculatedHash ===
                String(
                    event.eventHash ||
                        ""
                );

            if (
                !previousMatches ||
                !hashMatches
            ) {
                return {
                    valid: false,
                    eventCount:
                        events.length,
                    verifiedEventCount,
                    firstEventPreviousHash:
                        String(
                            events[0]
                                ?.previousEventHash ||
                            ""
                        ),
                    finalEventHash:
                        String(
                            events[
                                events.length - 1
                            ]?.eventHash ||
                            ""
                        ),
                    failedEventIndex:
                        sequence,
                    expectedHash:
                        calculatedHash,
                    actualHash:
                        String(
                            event.eventHash ||
                            ""
                        ),
                    message:
                        `Native audit integrity failed at event ${sequence}`
                };
            }

            previousHash =
                String(
                    event.eventHash
                );

            verifiedEventCount +=
                1;
        }

        return {
            valid: true,
            eventCount:
                events.length,
            verifiedEventCount,
            firstEventPreviousHash:
                String(
                    events[0]
                        ?.previousEventHash ||
                    ""
                ),
            finalEventHash:
                String(
                    events[
                        events.length - 1
                    ]?.eventHash ||
                    ""
                ),
            failedEventIndex:
                0,
            expectedHash:
                "",
            actualHash:
                "",
            message:
                "Native SHA-256 audit chain verified successfully"
        };
    };

const certificateArtifactFrom =
    artifact => ({
        artifactId: String(
            artifact.artifactId ||
            ""
        ),
        fileName: String(
            artifact.fileName ||
            ""
        ),
        fileType: String(
            artifact.fileType ||
            ""
        ),
        offset:
            Number(
                artifact.offset
            ) || 0,
        size:
            Number(
                artifact.size
            ) || 0,
        confidenceScore:
            Number(
                artifact.confidenceScore
            ) || 0,
        confidenceLevel:
            String(
                artifact.confidenceLevel ||
                "LOW"
            ),
        sha256:
            String(
                artifact.sha256 ||
                ""
            ),
        validated:
            Boolean(
                artifact.validated
            ),
        headerValid:
            Boolean(
                artifact.headerValid
            ),
        footerValid:
            Boolean(
                artifact.footerValid
            ),
        structureValid:
            Boolean(
                artifact.structureValid
            ),
        sizeValid:
            Boolean(
                artifact.sizeValid
            ),
        decodable:
            Boolean(
                artifact.decodable
            )
    });

const canonicalCertificate =
    certificate => {
        const artifacts =
            [
                ...(
                    certificate
                        ?.artifacts ||
                    []
                )
            ].sort(
                (
                    a,
                    b
                ) =>
                    String(
                        a.artifactId
                    ).localeCompare(
                        String(
                            b.artifactId
                        )
                    )
            );

        const lines = [
            "schemaVersion=1",
            `certificateId=${normalizeCanonicalValue(
                certificate.certificateId
            )}`,
            `runId=${normalizeCanonicalValue(
                certificate.runId
            )}`,
            `caseId=${normalizeCanonicalValue(
                certificate.caseId
            )}`,
            `workstationId=${normalizeCanonicalValue(
                certificate.workstationId
            )}`,
            `sourceIdentifier=${normalizeCanonicalValue(
                certificate.sourceIdentifier
            )}`,
            `sourceName=${normalizeCanonicalValue(
                certificate.sourceName
            )}`,
            `model=${normalizeCanonicalValue(
                certificate.model
            )}`,
            `serialNumber=${normalizeCanonicalValue(
                certificate.serialNumber
            )}`,
            `capacityBytes=${Number(
                certificate.capacityBytes
            ) || 0}`,
            `interfaceType=${normalizeCanonicalValue(
                certificate.interfaceType
            )}`,
            `bytesScanned=${Number(
                certificate.bytesScanned
            ) || 0}`,
            `totalBytes=${Number(
                certificate.totalBytes
            ) || 0}`,
            `candidatesFound=${Number(
                certificate.candidatesFound
            ) || 0}`,
            `recoveredArtifacts=${Number(
                certificate.recoveredArtifacts
            ) || 0}`,
            `validatedArtifacts=${Number(
                certificate.validatedArtifacts
            ) || 0}`,
            `rejectedArtifacts=${Number(
                certificate.rejectedArtifacts
            ) || 0}`,
            `highConfidenceArtifacts=${Number(
                certificate.highConfidenceArtifacts
            ) || 0}`,
            `recoveredBytes=${Number(
                certificate.recoveredBytes
            ) || 0}`,
            `auditAnchorHash=${normalizeCanonicalValue(
                certificate.auditAnchorHash
            )}`,
            `generatedAt=${normalizeCanonicalValue(
                certificate.generatedAt
            )}`,
            `hashAlgorithm=${normalizeCanonicalValue(
                certificate.hashAlgorithm
            )}`,
            `artifactCount=${artifacts.length}`
        ];

        for (
            const artifact
            of artifacts
        ) {
            const trusted =
                certificateArtifactFrom(
                    artifact
                );

            lines.push(
                `artifact.artifactId=${normalizeCanonicalValue(
                    trusted.artifactId
                )}`,
                `artifact.fileName=${normalizeCanonicalValue(
                    trusted.fileName
                )}`,
                `artifact.fileType=${normalizeCanonicalValue(
                    trusted.fileType
                )}`,
                `artifact.offset=${trusted.offset}`,
                `artifact.size=${trusted.size}`,
                `artifact.confidenceScore=${trusted.confidenceScore}`,
                `artifact.confidenceLevel=${normalizeCanonicalValue(
                    trusted.confidenceLevel
                )}`,
                `artifact.sha256=${normalizeCanonicalValue(
                    trusted.sha256
                )}`,
                `artifact.validated=${
                    trusted.validated
                        ? "true"
                        : "false"
                }`,
                `artifact.headerValid=${
                    trusted.headerValid
                        ? "true"
                        : "false"
                }`,
                `artifact.footerValid=${
                    trusted.footerValid
                        ? "true"
                        : "false"
                }`,
                `artifact.structureValid=${
                    trusted.structureValid
                        ? "true"
                        : "false"
                }`,
                `artifact.sizeValid=${
                    trusted.sizeValid
                        ? "true"
                        : "false"
                }`,
                `artifact.decodable=${
                    trusted.decodable
                        ? "true"
                        : "false"
                }`
            );
        }

        return (
            lines.join("\n") +
            "\n"
        );
    };

const verifyCertificateAgainstArtifacts =
    ({
        certificate,
        trustedArtifacts,
        caseId,
        runId,
        workstationId,
        sourceIdentifier,
        nativeEvents
    }) => {
        if (
            !certificate ||
            typeof certificate !==
                "object"
        ) {
            throw new AppError(
                "Native forensic certificate is required",
                400
            );
        }

        const actual = {
            certificateId:
                String(
                    certificate.certificateId ||
                    ""
                ),
            runId:
                String(
                    certificate.runId ||
                    ""
                ),
            caseId:
                String(
                    certificate.caseId ||
                    ""
                ),
            workstationId:
                String(
                    certificate.workstationId ||
                    ""
                ),
            sourceIdentifier:
                String(
                    certificate.sourceIdentifier ||
                    ""
                ),
            sourceName:
                String(
                    certificate.sourceName ||
                    ""
                ),
            model:
                String(
                    certificate.model ||
                    ""
                ),
            serialNumber:
                String(
                    certificate.serialNumber ||
                    ""
                ),
            capacityBytes:
                Number(
                    certificate.capacityBytes
                ) || 0,
            interfaceType:
                String(
                    certificate.interfaceType ||
                    ""
                ),
            bytesScanned:
                Number(
                    certificate.bytesScanned
                ) || 0,
            totalBytes:
                Number(
                    certificate.totalBytes
                ) || 0,
            candidatesFound:
                Number(
                    certificate.candidatesFound
                ) || 0,
            recoveredArtifacts:
                Number(
                    certificate.recoveredArtifacts
                ) || 0,
            validatedArtifacts:
                Number(
                    certificate.validatedArtifacts
                ) || 0,
            rejectedArtifacts:
                Number(
                    certificate.rejectedArtifacts
                ) || 0,
            highConfidenceArtifacts:
                Number(
                    certificate.highConfidenceArtifacts
                ) || 0,
            recoveredBytes:
                Number(
                    certificate.recoveredBytes
                ) || 0,
            auditAnchorHash:
                String(
                    certificate.auditAnchorHash ||
                    ""
                ),
            generatedAt:
                String(
                    certificate.generatedAt ||
                    ""
                ),
            hashAlgorithm:
                String(
                    certificate.hashAlgorithm ||
                    ""
                )
        };

        if (
            actual.runId !==
            runId
        ) {
            throw new AppError(
                "Certificate run ID does not match the evidence package",
                400
            );
        }

        if (
            actual.caseId !==
            caseId
        ) {
            throw new AppError(
                "Certificate case ID does not match the evidence package",
                400
            );
        }

        if (
            actual.workstationId !==
            workstationId
        ) {
            throw new AppError(
                "Certificate workstation ID does not match the evidence package",
                400
            );
        }

        if (
            actual.sourceIdentifier !==
            sourceIdentifier
        ) {
            throw new AppError(
                "Certificate source identifier does not match the physical device",
                400
            );
        }

        if (
            actual.serialNumber !==
            sourceIdentifier
        ) {
            throw new AppError(
                "Certificate serial number does not match the physical device",
                400
            );
        }

        if (
            actual.hashAlgorithm !==
            "SHA-256"
        ) {
            throw new AppError(
                "Unsupported forensic certificate hash algorithm",
                400
            );
        }

        const claimedArtifacts =
            Array.isArray(
                certificate.artifacts
            )
                ? certificate.artifacts
                : [];

        if (
            claimedArtifacts.length !==
            trustedArtifacts.length
        ) {
            throw new AppError(
                "Certificate artifact count does not match verified evidence",
                400
            );
        }
                const claimedById =
            new Map(
                claimedArtifacts.map(
                    artifact => [
                        String(
                            artifact.artifactId
                        ),
                        certificateArtifactFrom(
                            artifact
                        )
                    ]
                )
            );

        for (
            const artifact
            of trustedArtifacts
        ) {
            const claimed =
                claimedById.get(
                    String(
                        artifact.artifactId
                    )
                );

            if (!claimed) {
                throw new AppError(
                    `Certificate is missing artifact ${artifact.artifactId}`,
                    400
                );
            }

            const trusted =
                certificateArtifactFrom(
                    artifact
                );

            if (
                JSON.stringify(
                    claimed
                ) !==
                JSON.stringify(
                    trusted
                )
            ) {
                throw new AppError(
                    `Certificate artifact ${artifact.artifactId} does not match server-verified evidence metadata`,
                    400
                );
            }
        }

        const canonicalPayload =
            {
                ...actual,
                artifactCount:
                    trustedArtifacts.length,
                artifacts:
                    trustedArtifacts.map(
                        certificateArtifactFrom
                    )
            };

        const calculatedHash =
            sha256(
                canonicalCertificate(
                    canonicalPayload
                )
            );

        const certificateHash =
            String(
                certificate.certificateHash ||
                ""
            );

        if (
            calculatedHash !==
            certificateHash
        ) {
            throw new AppError(
                "Native forensic certificate SHA-256 verification failed",
                400
            );
        }

        if (
            !nativeEvents.some(
                event =>
                    String(
                        event.eventHash ||
                        ""
                    ) ===
                    actual.auditAnchorHash
            )
        ) {
            throw new AppError(
                "Certificate audit anchor hash does not exist in the native audit chain",
                400
            );
        }

        const certificateEvent =
            nativeEvents.find(
                event =>
                    event.eventType ===
                    "CERTIFICATE_GENERATED"
            );

        if (
            !certificateEvent ||
            !String(
                certificateEvent.details ||
                ""
            ).includes(
                `certificateHash=${certificateHash}`
            )
        ) {
            throw new AppError(
                "Native audit chain does not bind the generated certificate hash",
                400
            );
        }

        return {
            valid: true,
            certificateHash,
            calculatedHash,
            auditAnchorHash:
                actual.auditAnchorHash,
            certificateId:
                actual.certificateId,
            runId,
            artifactCount:
                trustedArtifacts.length
        };
    };

const createServerAuditEvent =
    async ({
        caseItem,
        action,
        user,
        fromStatus = "",
        toStatus = "",
        workstation = null,
        workstationId = "",
        workstationName = "",
        note = "",
        metadata = {}
    }) => {
        const previousEvent =
            await ForensicAuditLog.findOne(
                {
                    caseId:
                        caseItem.caseId
                }
            ).sort({
                sequence: -1
            });

        const sequence =
            previousEvent
                ? previousEvent.sequence +
                  1
                : 1;

        const previousEventHash =
            previousEvent?.eventHash ||
            "";

        const auditId =
            await generateAuditId();

        const timestamp =
            new Date();

        const center =
            await WorkstationCenter.findById(
                caseItem.workstationCenter
            )
                .select("centerId")
                .lean();

        const payload = {
            auditId,
            caseId:
                caseItem.caseId,
            sequence,
            action,
            fromStatus,
            toStatus,
            actor:
                user?._id
                    ? String(
                          user._id
                      )
                    : null,
            actorName:
                user?.name ||
                "",
            actorRole:
                user?.role ||
                "",
            workstation:
                workstation?._id
                    ? String(
                          workstation._id
                      )
                    : null,
            workstationId:
                workstation?.workstationId ||
                workstationId ||
                "",
            workstationName:
                workstation?.name ||
                workstationName ||
                "",
            workstationCenter:
                caseItem.workstationCenter
                    ? String(
                          caseItem.workstationCenter
                      )
                    : null,
            workstationCenterId:
                center?.centerId ||
                "",
            sourceIdentifier:
                caseItem.sourceIdentifier ||
                "",
            sourceName:
                caseItem.sourceName ||
                "",
            note:
                note || "",
            metadata:
                metadata || {},
            previousEventHash,
            timestamp
        };

        return ForensicAuditLog.create(
            {
                ...payload,
                eventHash:
                    sha256(
                        JSON.stringify(
                            payload
                        )
                    )
            }
        );
    };

const ingestEvidencePackage =
    async (
        caseId,
        payload,
        user
    ) => {
        const item =
            await ForensicCase.findOne(
                {
                    caseId
                }
            );

        await ensureCaseAccess(
            item,
            user
        );

        if (
            ![
                "ADMIN",
                "WORKSTATION_EMPLOYEE"
            ].includes(
                user.role
            )
        ) {
            throw new AppError(
                "Only the assigned workstation employee or admin can upload forensic evidence",
                403
            );
        }

        if (
            !payload ||
            payload.schemaVersion !==
                1
        ) {
            throw new AppError(
                "Unsupported forensic evidence package version",
                400
            );
        }

        if (
            ![
                "ACQUIRING",
                "ANALYZING"
            ].includes(
                item.status
            )
        ) {
            throw new AppError(
                `Forensic evidence cannot be uploaded while case is ${item.status}`,
                400
            );
        }

        const workstationId =
            String(
                payload.workstationId ||
                ""
            ).trim();

        const sourceIdentifier =
            String(
                payload.sourceIdentifier ||
                ""
            ).trim();

        const workstation =
            await getAssignedWorkstation(
                item,
                user,
                workstationId
            );

        if (
            payload.sourceType !==
            "PHYSICAL_DEVICE"
        ) {
            throw new AppError(
                "This endpoint currently accepts physical-device acquisition only",
                400
            );
        }

        if (
            !sourceIdentifier ||
            sourceIdentifier !==
                String(
                    item.sourceIdentifier ||
                    ""
                ).trim()
        ) {
            throw new AppError(
                "Physical source identifier does not match the forensic case",
                400
            );
        }

        const runId =
            String(
                payload.runId ||
                ""
            ).trim();

        if (!runId) {
            throw new AppError(
                "Forensic run ID is required",
                400
            );
        }

        const existingRun =
            await ForensicIntegrityLog.findOne(
                {
                    caseId,
                    runId
                }
            );

        if (existingRun) {
            throw new AppError(
                `Forensic run ${runId} has already been uploaded`,
                409
            );
        }

        const payloadArtifacts =
            Array.isArray(
                payload.artifacts
            )
                ? payload.artifacts
                : [];

        if (
            !payloadArtifacts.length
        ) {
            throw new AppError(
                "At least one recovered evidence artifact is required",
                400
            );
        }

        const nativeAudit =
            payload.nativeAudit ||
            {};

        if (
            nativeAudit.hashAlgorithm !==
            "SHA-256"
        ) {
            throw new AppError(
                "Native forensic audit must use SHA-256",
                400
            );
        }

        const nativeEvents =
            Array.isArray(
                nativeAudit.events
            )
                ? nativeAudit.events
                : [];

        const nativeVerification =
            verifyNativeAuditEvents(
                {
                    caseId,
                    runId,
                    workstationId,
                    sourceIdentifier,
                    events:
                        nativeEvents
                }
            );

        if (
            !nativeVerification.valid
        ) {
            throw new AppError(
                nativeVerification.message,
                400
            );
        }

        const declaredBytes =
            payloadArtifacts.reduce(
                (
                    sum,
                    artifact
                ) =>
                    sum +
                    Math.max(
                        0,
                        Number(
                            artifact.size
                        ) || 0
                    ),
                0
            );

        if (
            declaredBytes >
            MAX_UPLOAD_BYTES
        ) {
            throw new AppError(
                `Forensic artifact payload exceeds ${MAX_UPLOAD_BYTES} bytes`,
                413
            );
        }

        const verifiedArtifacts =
            [];

        const uploadedStorageIds =
            [];

        try {
            for (
                let index = 0;
                index <
                payloadArtifacts.length;
                index += 1
            ) {
                const artifact =
                    payloadArtifacts[
                        index
                    ];

                const artifactId =
                    String(
                        artifact.artifactId ||
                        `ART-${String(
                            index + 1
                        ).padStart(
                            4,
                            "0"
                        )}`
                    ).trim();

                const contentBase64 =
                    assertBase64(
                        artifact.contentBase64
                    );

                const buffer =
                    Buffer.from(
                        contentBase64,
                        "base64"
                    );

                const declaredSize =
                    Number(
                        artifact.size
                    ) || 0;

                if (
                    buffer.length !==
                    declaredSize
                ) {
                    throw new AppError(
                        `Artifact ${artifactId} content size does not match its declared size`,
                        400
                    );
                }

                const calculatedHash =
                    crypto
                        .createHash(
                            "sha256"
                        )
                        .update(
                            buffer
                        )
                        .digest(
                            "hex"
                        );

                const claimedHash =
                    String(
                        artifact.sha256 ||
                        ""
                    )
                        .trim()
                        .toLowerCase();

                if (
                    claimedHash.length !==
                        64 ||
                    calculatedHash !==
                        claimedHash
                ) {
                    throw new AppError(
                        `Artifact ${artifactId} SHA-256 verification failed`,
                        400
                    );
                }

                const fileName =
                    String(
                        artifact.fileName ||
                        `${artifactId.toLowerCase()}.bin`
                    );

                const fileType =
                    String(
                        artifact.fileType ||
                        "UNKNOWN"
                    );

                const contentMimeType =
                    contentTypeFor(
                        fileType
                    );

                const uploaded =
                    await uploadBufferToGridFs(
                        {
                            buffer,
                            filename:
                                fileName,
                            contentType:
                                contentMimeType,
                            metadata: {
                                caseId,
                                runId,
                                artifactId,
                                sha256:
                                    calculatedHash,
                                sourceIdentifier,
                                workstationId
                            }
                        }
                    );

                uploadedStorageIds.push(
                    uploaded.id
                );

                const confidenceLevel =
                    String(
                        artifact.confidenceLevel ||
                        "LOW"
                    );

                verifiedArtifacts.push(
                    {
                        artifactId,
                        fileName,
                        fileType,
                        offset:
                            Math.max(
                                0,
                                Number(
                                    artifact.offset
                                ) || 0
                            ),
                        size:
                            buffer.length,
                        recoveredPath:
                            String(
                                artifact.recoveredPath ||
                                ""
                            ),
                        headerValid:
                            Boolean(
                                artifact.headerValid
                            ),
                        footerValid:
                            Boolean(
                                artifact.footerValid
                            ),
                        structureValid:
                            Boolean(
                                artifact.structureValid
                            ),
                        sizeValid:
                            Boolean(
                                artifact.sizeValid
                            ),
                        decodable:
                            Boolean(
                                artifact.decodable
                            ),
                        confidenceScore:
                            Math.max(
                                0,
                                Math.min(
                                    100,
                                    Number(
                                        artifact.confidenceScore
                                    ) || 0
                                )
                            ),
                        confidenceLevel:
                            ALLOWED_CONFIDENCE.has(
                                confidenceLevel
                            )
                                ? confidenceLevel
                                : "LOW",
                        confidenceReasons:
                            Array.isArray(
                                artifact.confidenceReasons
                            )
                                ? artifact.confidenceReasons
                                      .slice(
                                          0,
                                          20
                                      )
                                      .map(
                                          String
                                      )
                                : [],
                        sha256:
                            calculatedHash,
                        recovered:
                            artifact.recovered !==
                            false,
                        validated:
                            Boolean(
                                artifact.validated
                            ),
                        contentAvailable:
                            true,
                        contentMimeType,
                        contentLength:
                            buffer.length,
                        contentSha256:
                            calculatedHash,
                        storageId:
                            uploaded.id,
                        contentUrl:
                            `/api/forensics/${encodeURIComponent(
                                caseId
                            )}/artifacts/${encodeURIComponent(
                                artifactId
                            )}/content`
                    }
                );
            }

            const certificateVerification =
                verifyCertificateAgainstArtifacts(
                    {
                        certificate:
                            payload.certificate,
                        trustedArtifacts:
                            verifiedArtifacts,
                        caseId,
                        runId,
                        workstationId,
                        sourceIdentifier,
                        nativeEvents
                    }
                );

            const source =
                payload.source ||
                {};

            const certificate =
                payload.certificate;

            const generatedAt =
                new Date(
                    String(
                        certificate.generatedAt ||
                        ""
                    )
                );

            if (
                Number.isNaN(
                    generatedAt.getTime()
                )
            ) {
                throw new AppError(
                    "Forensic certificate generatedAt is invalid",
                    400
                );
            }

            item.artifacts =
                verifiedArtifacts;

            item.bytesScanned =
                Number(
                    payload.summary
                        ?.bytesScanned
                ) || 0;

            item.totalBytes =
                Number(
                    payload.summary
                        ?.totalBytes
                ) || 0;

            item.candidatesFound =
                Number(
                    payload.summary
                        ?.candidatesFound
                ) || 0;

            item.recoveredArtifacts =
                Number(
                    payload.summary
                        ?.recoveredArtifacts
                ) ||
                verifiedArtifacts.length;

            item.validatedArtifacts =
                Number(
                    payload.summary
                        ?.validatedArtifacts
                ) ||
                verifiedArtifacts.filter(
                    artifact =>
                        artifact.validated
                ).length;

            item.rejectedArtifacts =
                Number(
                    payload.summary
                        ?.rejectedArtifacts
                ) || 0;

            item.highConfidenceArtifacts =
                Number(
                    payload.summary
                        ?.highConfidenceArtifacts
                ) ||
                verifiedArtifacts.filter(
                    artifact =>
                        artifact.confidenceLevel ===
                        "HIGH"
                ).length;

            item.recoveredBytes =
                Number(
                    payload.summary
                        ?.recoveredBytes
                ) ||
                verifiedArtifacts.reduce(
                    (
                        sum,
                        artifact
                    ) =>
                        sum +
                        artifact.size,
                    0
                );
                            item.forensicCertificate =
                {
                    generated:
                        true,

                    certificateId:
                        certificateVerification.certificateId,

                    runId,

                    caseId,

                    workstationId,

                    sourceIdentifier,

                    sourceName:
                        String(
                            certificate.sourceName ||
                            source.deviceId ||
                            ""
                        ),

                    model:
                        String(
                            certificate.model ||
                            source.model ||
                            ""
                        ),

                    serialNumber:
                        String(
                            certificate.serialNumber ||
                            source.serialNumber ||
                            sourceIdentifier
                        ),

                    capacityBytes:
                        Number(
                            certificate.capacityBytes ??
                            source.capacityBytes
                        ) || 0,

                    interfaceType:
                        String(
                            certificate.interfaceType ||
                            source.interfaceType ||
                            ""
                        ),

                    bytesScanned:
                        Number(
                            certificate.bytesScanned
                        ) ||
                        item.bytesScanned,

                    totalBytes:
                        Number(
                            certificate.totalBytes
                        ) ||
                        item.totalBytes,

                    candidatesFound:
                        Number(
                            certificate.candidatesFound
                        ) ||
                        item.candidatesFound,

                    recoveredArtifacts:
                        Number(
                            certificate.recoveredArtifacts
                        ) ||
                        item.recoveredArtifacts,

                    validatedArtifacts:
                        Number(
                            certificate.validatedArtifacts
                        ) ||
                        item.validatedArtifacts,

                    rejectedArtifacts:
                        Number(
                            certificate.rejectedArtifacts
                        ) ||
                        item.rejectedArtifacts,

                    highConfidenceArtifacts:
                        Number(
                            certificate.highConfidenceArtifacts
                        ) ||
                        item.highConfidenceArtifacts,

                    recoveredBytes:
                        Number(
                            certificate.recoveredBytes
                        ) ||
                        item.recoveredBytes,

                    auditAnchorHash:
                        certificateVerification.auditAnchorHash,

                    generatedAt,

                    hashAlgorithm:
                        "SHA-256",

                    artifactCount:
                        verifiedArtifacts.length,

                    certificateHash:
                        certificateVerification.certificateHash,

                    serverCalculatedCertificateHash:
                        certificateVerification.calculatedHash,

                    serverVerified:
                        true,

                    verifiedAt:
                        new Date()
                };

            item.forensicIntegrity =
                {
                    verified:
                        true,

                    runId,

                    eventCount:
                        nativeVerification.eventCount,

                    verifiedEventCount:
                        nativeVerification.verifiedEventCount,

                    firstEventPreviousHash:
                        nativeVerification.firstEventPreviousHash,

                    finalEventHash:
                        nativeVerification.finalEventHash,

                    verifiedAt:
                        new Date()
                };

            await ForensicIntegrityLog.insertMany(
                nativeEvents.map(
                    event => {
                        const parsedTimestamp =
                            new Date(
                                String(
                                    event.timestampUtc ||
                                    ""
                                )
                            );

                        if (
                            Number.isNaN(
                                parsedTimestamp.getTime()
                            )
                        ) {
                            throw new AppError(
                                `Native audit event ${event.sequence} has an invalid timestamp`,
                                400
                            );
                        }

                        return {
                            caseId,
                            runId,
                            sequence:
                                Number(
                                    event.sequence
                                ),
                            eventType:
                                String(
                                    event.eventType ||
                                    ""
                                ),
                            artifactId:
                                String(
                                    event.artifactId ||
                                    ""
                                ),
                            timestampUtc:
                                String(
                                    event.timestampUtc ||
                                    ""
                                ),
                            timestamp:
                                parsedTimestamp,
                            workstationId,
                            sourceIdentifier,
                            details:
                                String(
                                    event.details ||
                                    ""
                                ),
                            previousEventHash:
                                String(
                                    event.previousEventHash ||
                                    ""
                                ),
                            eventHash:
                                String(
                                    event.eventHash ||
                                    ""
                                )
                        };
                    }
                ),
                {
                    ordered:
                        true
                }
            );

            item.markModified(
                "artifacts"
            );

            item.markModified(
                "forensicCertificate"
            );

            item.markModified(
                "forensicIntegrity"
            );

            await item.save();

            await createServerAuditEvent(
                {
                    caseItem: item,
                    action:
                        "EVIDENCE_SUBMITTED",
                    user,
                    fromStatus:
                        item.status,
                    toStatus:
                        item.status,
                    workstation,
                    workstationId,
                    workstationName:
                        workstation.name,
                    note:
                        "Native forensic evidence package received and cryptographically verified",
                    metadata: {
                        runId,
                        artifactCount:
                            verifiedArtifacts.length,
                        certificateId:
                            certificateVerification.certificateId,
                        certificateHash:
                            certificateVerification.certificateHash,
                        nativeAuditEventCount:
                            nativeVerification.eventCount
                    }
                }
            );

            await createServerAuditEvent(
                {
                    caseItem: item,
                    action:
                        "INTEGRITY_VERIFIED",
                    user,
                    fromStatus:
                        item.status,
                    toStatus:
                        item.status,
                    workstation,
                    workstationId,
                    workstationName:
                        workstation.name,
                    note:
                        "Server independently verified artifact SHA-256 values, native audit-chain hashes and certificate SHA-256",
                    metadata: {
                        runId,
                        certificateHash:
                            certificateVerification.certificateHash,
                        auditAnchorHash:
                            certificateVerification.auditAnchorHash,
                        finalNativeEventHash:
                            nativeVerification.finalEventHash,
                        verifiedEvents:
                            nativeVerification.verifiedEventCount
                    }
                }
            );

            return {
                case: item,
                integrity: {
                    nativeAudit:
                        nativeVerification,

                    certificate:
                        certificateVerification,

                    artifactCount:
                        verifiedArtifacts.length
                }
            };
        } catch (error) {
            await Promise.all(
                uploadedStorageIds.map(
                    deleteGridFsFile
                )
            );

            throw error;
        }
    };

const getNativeIntegrity =
    async (
        caseId,
        user
    ) => {
        const item =
            await ForensicCase.findOne(
                {
                    caseId
                }
            );

        await ensureCaseAccess(
            item,
            user
        );

        const runId =
            String(
                item.forensicIntegrity
                    ?.runId ||
                ""
            ).trim();

        if (!runId) {
            return {
                caseId,
                runId: "",
                chainValid: false,
                message:
                    "No native forensic integrity chain has been uploaded yet.",
                eventCount: 0,
                verifiedEventCount: 0,
                events: [],
                certificate:
                    item.forensicCertificate ||
                    null
            };
        }

        const events =
            await ForensicIntegrityLog.find(
                {
                    caseId,
                    runId
                }
            )
                .sort({
                    sequence: 1
                })
                .lean();

        const workstationId =
            String(
                item.forensicCertificate
                    ?.workstationId ||
                ""
            );

        const sourceIdentifier =
            String(
                item.sourceIdentifier ||
                ""
            );

        const verification =
            verifyNativeAuditEvents(
                {
                    caseId,
                    runId,
                    workstationId,
                    sourceIdentifier,
                    events
                }
            );

        return {
            caseId,
            runId,

            chainValid:
                verification.valid,

            eventCount:
                verification.eventCount,

            verifiedEventCount:
                verification.verifiedEventCount,

            failedEventIndex:
                verification.failedEventIndex,

            expectedHash:
                verification.expectedHash,

            actualHash:
                verification.actualHash,

            firstEventPreviousHash:
                verification.firstEventPreviousHash,

            finalEventHash:
                verification.finalEventHash,

            message:
                verification.message,

            events:
                events.map(
                    event => {
                        const calculatedHash =
                            sha256(
                                canonicalAuditEvent(
                                    {
                                        caseId,
                                        runId,
                                        workstationId,
                                        sourceIdentifier,
                                        event
                                    }
                                )
                            );

                        return {
                            ...event,
                            calculatedHash,
                            hashValid:
                                calculatedHash ===
                                event.eventHash
                        };
                    }
                ),

            certificate:
                item.forensicCertificate ||
                null
        };
    };

const verifyStoredIntegrity =
    async (
        caseId,
        user
    ) => {
        const item =
            await ForensicCase.findOne(
                {
                    caseId
                }
            );

        await ensureCaseAccess(
            item,
            user
        );

        const runId =
            String(
                item.forensicIntegrity
                    ?.runId ||
                ""
            ).trim();

        if (!runId) {
            throw new AppError(
                "No native forensic integrity package is stored for this case",
                404
            );
        }

        const events =
            await ForensicIntegrityLog.find(
                {
                    caseId,
                    runId
                }
            )
                .sort({
                    sequence: 1
                })
                .lean();

        const workstationId =
            String(
                item.forensicCertificate
                    ?.workstationId ||
                ""
            );

        const sourceIdentifier =
            String(
                item.sourceIdentifier ||
                ""
            );

        const verification =
            verifyNativeAuditEvents(
                {
                    caseId,
                    runId,
                    workstationId,
                    sourceIdentifier,
                    events
                }
            );

        const trustedArtifacts =
            item.artifacts.map(
                certificateArtifactFrom
            );

        const cert =
            item.forensicCertificate;

        const certificateData =
            cert
                ? {
                      certificateId:
                          cert.certificateId,

                      runId:
                          cert.runId,

                      caseId,

                      workstationId:
                          cert.workstationId,

                      sourceIdentifier:
                          cert.sourceIdentifier,

                      sourceName:
                          cert.sourceName,

                      model:
                          cert.model,

                      serialNumber:
                          cert.serialNumber,

                      capacityBytes:
                          cert.capacityBytes,

                      interfaceType:
                          cert.interfaceType,

                      bytesScanned:
                          cert.bytesScanned,

                      totalBytes:
                          cert.totalBytes,

                      candidatesFound:
                          cert.candidatesFound,

                      recoveredArtifacts:
                          cert.recoveredArtifacts,

                      validatedArtifacts:
                          cert.validatedArtifacts,

                      rejectedArtifacts:
                          cert.rejectedArtifacts,

                      highConfidenceArtifacts:
                          cert.highConfidenceArtifacts,

                      recoveredBytes:
                          cert.recoveredBytes,

                      auditAnchorHash:
                          cert.auditAnchorHash,

                      generatedAt:
                          cert.generatedAt
                              ?.toISOString
                              ?.() ||
                          String(
                              cert.generatedAt ||
                              ""
                          ),

                      hashAlgorithm:
                          cert.hashAlgorithm,

                      artifactCount:
                          trustedArtifacts.length,

                      artifacts:
                          trustedArtifacts,

                      certificateHash:
                          cert.certificateHash
                  }
                : null;

        const calculatedCertificateHash =
            certificateData
                ? sha256(
                      canonicalCertificate(
                          certificateData
                      )
                  )
                : "";

        const certificateValid =
            Boolean(
                certificateData
            ) &&
            calculatedCertificateHash ===
                String(
                    certificateData
                        .certificateHash ||
                    ""
                );

        let artifactIntegrityValid =
            true;

        let verifiedArtifactCount =
            0;

        let artifactFailure =
            "";

        for (
            const artifact
            of item.artifacts
        ) {
            if (
                !artifact.contentAvailable ||
                !artifact.storageId
            ) {
                artifactIntegrityValid =
                    false;

                artifactFailure =
                    `Artifact ${artifact.artifactId} has no stored content`;

                break;
            }

            try {
                const buffer =
                    await readGridFsBuffer(
                        artifact.storageId
                    );

                const calculatedArtifactHash =
                    crypto
                        .createHash(
                            "sha256"
                        )
                        .update(
                            buffer
                        )
                        .digest(
                            "hex"
                        );

                if (
                    calculatedArtifactHash !==
                    artifact.sha256
                ) {
                    artifactIntegrityValid =
                        false;

                    artifactFailure =
                        `Artifact ${artifact.artifactId} SHA-256 does not match stored content`;

                    break;
                }

                verifiedArtifactCount +=
                    1;
            } catch {
                artifactIntegrityValid =
                    false;

                artifactFailure =
                    `Artifact ${artifact.artifactId} content could not be read from GridFS`;

                break;
            }
        }

        const valid =
            verification.valid &&
            certificateValid &&
            artifactIntegrityValid;

        const verifiedAt =
            new Date();

        item.forensicIntegrity =
            {
                verified:
                    valid,

                runId,

                eventCount:
                    verification.eventCount,

                verifiedEventCount:
                    verification.verifiedEventCount,

                firstEventPreviousHash:
                    verification.firstEventPreviousHash,

                finalEventHash:
                    verification.finalEventHash,

                verifiedAt
            };

        if (
            item.forensicCertificate
        ) {
            item.forensicCertificate
                .serverCalculatedCertificateHash =
                    calculatedCertificateHash;

            item.forensicCertificate
                .serverVerified =
                    valid;

            item.forensicCertificate
                .verifiedAt =
                    verifiedAt;
        }

        item.markModified(
            "forensicIntegrity"
        );

        item.markModified(
            "forensicCertificate"
        );

        await item.save();

        return {
            caseId,
            runId,

            valid,

            chainValid:
                verification.valid,

            certificateValid,

            artifactIntegrityValid,

            eventCount:
                verification.eventCount,

            verifiedEventCount:
                verification.verifiedEventCount,

            verifiedArtifactCount,

            totalArtifacts:
                item.artifacts.length,

            failedEventIndex:
                verification.failedEventIndex,

            expectedHash:
                verification.expectedHash,

            actualHash:
                verification.actualHash,

            firstEventPreviousHash:
                verification.firstEventPreviousHash,

            finalEventHash:
                verification.finalEventHash,

            calculatedCertificateHash,

            storedCertificateHash:
                cert?.certificateHash ||
                "",

            artifactFailure,

            verifiedAt,

            message:
                valid
                    ? "Server independently verified the complete native audit chain, forensic certificate and stored artifact SHA-256 values."
                    : artifactFailure ||
                      verification.message ||
                      "Forensic integrity verification failed."
        };
    };

const streamArtifactContent =
    async (
        caseId,
        artifactId,
        user,
        res
    ) => {
        const item =
            await ForensicCase.findOne(
                {
                    caseId
                }
            );

        await ensureCaseAccess(
            item,
            user
        );

        const artifact =
            item.artifacts.find(
                entry =>
                    String(
                        entry.artifactId
                    ) ===
                    String(
                        artifactId
                    )
            );

        if (!artifact) {
            throw new AppError(
                "Forensic artifact not found",
                404
            );
        }

        if (
            !artifact.contentAvailable ||
            !artifact.storageId
        ) {
            throw new AppError(
                "The recovered artifact content is not stored on the server",
                404
            );
        }

        if (
            !mongoose.Types.ObjectId.isValid(
                artifact.storageId
            )
        ) {
            throw new AppError(
                "Stored forensic artifact reference is invalid",
                500
            );
        }

        const safeName =
            String(
                artifact.fileName ||
                "recovered-evidence"
            )
                .replace(
                    /[\\/"\r\n]/g,
                    "_"
                )
                .trim() ||
            "recovered-evidence";

        res.status(200);

        res.setHeader(
            "Content-Type",
            artifact.contentMimeType ||
                contentTypeFor(
                    artifact.fileType
                )
        );

        res.setHeader(
            "Content-Length",
            String(
                artifact.contentLength ||
                artifact.size ||
                0
            )
        );

        res.setHeader(
            "Content-Disposition",
            `inline; filename="${safeName}"`
        );

        res.setHeader(
            "Cache-Control",
            "private, no-store"
        );

        res.setHeader(
            "X-Forensic-SHA256",
            artifact.sha256 ||
                ""
        );

        res.setHeader(
            "X-Forensic-Artifact-Id",
            artifact.artifactId
        );

        const download =
            getForensicBucket()
                .openDownloadStream(
                    new mongoose.Types.ObjectId(
                        artifact.storageId
                    )
                );

        download.once(
            "error",
            error => {
                if (
                    !res.headersSent
                ) {
                    res.status(
                        404
                    ).json(
                        {
                            success:
                                false,
                            message:
                                "Recovered artifact content could not be read"
                        }
                    );

                    return;
                }

                res.destroy(
                    error
                );
            }
        );

        download.pipe(res);
    };

module.exports = {
    ingestEvidencePackage,
    getNativeIntegrity,
    verifyStoredIntegrity,
    streamArtifactContent
};