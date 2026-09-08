const crypto = require("crypto");
const ForensicCase = require("../models/ForensicCase");
const ForensicAuditLog = require("../models/ForensicAuditLog");
const WorkstationCenter = require("../models/WorkstationCenter");
const Workstation = require("../models/WorkStation");
const User = require("../models/User");
const Counter = require("../models/Counter");
const AppError = require("../utils/AppError");

const generateCaseId = async () => {
    const counter = await Counter.findOneAndUpdate(
        { name: "forensicCase" },
        { $inc: { sequence: 1 } },
        { new: true, upsert: true }
    );

    return `FR-${String(counter.sequence).padStart(4, "0")}`;
};

const generateAuditId = async () => {
    const counter = await Counter.findOneAndUpdate(
        { name: "forensicAudit" },
        { $inc: { sequence: 1 } },
        { new: true, upsert: true }
    );

    return `FA-${String(counter.sequence).padStart(8, "0")}`;
};

const populateCase = query => query
    .populate("customer", "name email role")
    .populate("workstationCenter", "centerId name location status")
    .populate("assignedEmployee", "name email role status")
    .populate("assignedWorkstation", "workstationId name status connectionStatus hostname operatingSystem");

const ensureCaseAccess = (item, user) => {
    if (!item) {
        throw new AppError("Forensic case not found", 404);
    }

    if (!user || !user._id || !user.role) {
        throw new AppError("Authentication required", 401);
    }

    const id = String(user._id);

    if (user.role === "ADMIN") return;

    if (
        user.role === "CUSTOMER" &&
        String(item.customer?._id || item.customer) === id
    ) {
        return;
    }

    if (
        user.role === "WORKSTATION_EMPLOYEE" &&
        String(item.assignedEmployee?._id || item.assignedEmployee) === id
    ) {
        return;
    }

    if (
        user.role === "WORKSTATION_HEAD" &&
        String(item.workstationCenter?._id || item.workstationCenter) ===
        String(user.workstationCenter)
    ) {
        return;
    }

    throw new AppError("Access denied", 403);
};

const addHistory = (item, status, user, note) => {
    item.status = status;

    item.history.push({
        status,
        changedBy: user._id,
        changedAt: new Date(),
        note: note || ""
    });
};

const allowedStatusTransitions = {
    PENDING: ["ASSIGNED", "CANCELLED"],
    ASSIGNED: ["ACQUIRING", "CANCELLED"],
    ACQUIRING: ["ANALYZING", "FAILED", "CANCELLED"],
    ANALYZING: ["COMPLETED", "FAILED", "CANCELLED"],
    COMPLETED: [],
    FAILED: [],
    CANCELLED: []
};

const ensureStatusTransition = (currentStatus, nextStatus) => {
    const allowedNextStatuses =
        allowedStatusTransitions[currentStatus] || [];

    if (!allowedNextStatuses.includes(nextStatus)) {
        throw new AppError(
            `Invalid status transition: ${currentStatus} → ${nextStatus}`,
            400
        );
    }
};

const getAssignedWorkstation = async (
    item,
    user,
    workstationId = ""
) => {
    if (!item.assignedWorkstation) {
        throw new AppError(
            "No workstation is assigned to this case",
            400
        );
    }

    if (
        user.role === "WORKSTATION_EMPLOYEE" &&
        !workstationId
    ) {
        throw new AppError(
            "Workstation ID is required",
            400
        );
    }

    const targetId =
        user.role === "WORKSTATION_EMPLOYEE"
            ? workstationId
            : String(item.assignedWorkstation);

    const workstation =
        await Workstation.findById(targetId);

    if (!workstation) {
        throw new AppError(
            "Assigned workstation not found",
            404
        );
    }

    if (
        String(workstation._id) !==
        String(item.assignedWorkstation)
    ) {
        throw new AppError(
            "This workstation is not assigned to the forensic case",
            403
        );
    }

    if (
        String(workstation.workstationCenter) !==
        String(item.workstationCenter)
    ) {
        throw new AppError(
            "The workstation does not belong to the case workstation center",
            403
        );
    }

    if (workstation.status !== "ACTIVE") {
        throw new AppError(
            "The assigned workstation is not active",
            409
        );
    }

    if (user.role === "WORKSTATION_EMPLOYEE") {
        if (
            !workstation.assignedEmployee ||
            String(workstation.assignedEmployee) !==
            String(user._id)
        ) {
            throw new AppError(
                "The assigned workstation is not bound to the authenticated employee",
                403
            );
        }
    }

    return workstation;
};

const buildAuditHash = payload => {
    return crypto
        .createHash("sha256")
        .update(JSON.stringify(payload))
        .digest("hex");
};

const createAuditEvent = async ({
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
        await ForensicAuditLog.findOne({
            caseId: caseItem.caseId
        }).sort({ sequence: -1 });

    const sequence =
        previousEvent
            ? previousEvent.sequence + 1
            : 1;

    const previousEventHash =
        previousEvent?.eventHash || "";

    const auditId =
        await generateAuditId();

    const timestamp = new Date();

    const center =
        await WorkstationCenter.findById(
            caseItem.workstationCenter
        )
            .select("centerId")
            .lean();

    const normalizedWorkstation =
        workstation?._id
            ? String(workstation._id)
            : workstation
                ? String(workstation)
                : null;

    const normalizedActor =
        user?._id
            ? String(user._id)
            : null;

    const normalizedCenter =
        caseItem.workstationCenter
            ? String(
                caseItem.workstationCenter?._id ||
                caseItem.workstationCenter
            )
            : null;

    const payload = {
        auditId,
        caseId: caseItem.caseId,
        sequence,
        action,
        fromStatus,
        toStatus,

        actor:
            normalizedActor,

        actorName:
            user?.name || "",

        actorRole:
            user?.role || "",

        workstation:
            normalizedWorkstation,

        workstationId:
            workstation?.workstationId ||
            workstationId ||
            "",

        workstationName:
            workstation?.name ||
            workstationName ||
            "",

        workstationCenter:
            normalizedCenter,

        workstationCenterId:
            center?.centerId || "",

        sourceIdentifier:
            caseItem.sourceIdentifier || "",

        sourceName:
            caseItem.sourceName || "",

        note:
            note || "",

        metadata:
            metadata || {},

        previousEventHash,

        timestamp
    };

    const eventHash =
        buildAuditHash(payload);

    return ForensicAuditLog.create({
        ...payload,
        eventHash
    });
};

const getForensicAuditTrail = async (
    caseId,
    user
) => {
    const item =
        await ForensicCase.findOne({
            caseId
        });

    ensureCaseAccess(item, user);

    const events =
        await ForensicAuditLog.find({
            caseId
        })
            .populate(
                "actor",
                "name email role"
            )
            .populate(
                "workstation",
                "workstationId name status hostname operatingSystem"
            )
            .populate(
                "workstationCenter",
                "centerId name"
            )
            .sort({
                sequence: 1
            })
            .lean();

    let previousHash = "";
    let validChain = true;

    const verifiedEvents =
        events.map(event => {
            const payload = {
                auditId:
                    event.auditId,

                caseId:
                    event.caseId,

                sequence:
                    event.sequence,

                action:
                    event.action,

                fromStatus:
                    event.fromStatus || "",

                toStatus:
                    event.toStatus || "",

                actor:
                    event.actor?._id
                        ? String(event.actor._id)
                        : event.actor
                            ? String(event.actor)
                            : "",

                actorName:
                    event.actorName || "",

                actorRole:
                    event.actorRole || "",

                workstation:
                    event.workstation?._id
                        ? String(
                            event.workstation._id
                        )
                        : event.workstation
                            ? String(
                                event.workstation
                            )
                            : "",

                workstationId:
                    event.workstation?.workstationId ||
                    event.workstationId ||
                    "",

                workstationName:
                    event.workstation?.name ||
                    event.workstationName ||
                    "",

                workstationCenter:
                    event.workstationCenter?._id
                        ? String(
                            event.workstationCenter._id
                        )
                        : event.workstationCenter
                            ? String(
                                event.workstationCenter
                            )
                            : "",

                workstationCenterId:
                    event.workstationCenter?.centerId ||
                    event.workstationCenterId ||
                    "",

                sourceIdentifier:
                    event.sourceIdentifier || "",

                sourceName:
                    event.sourceName || "",

                note:
                    event.note || "",

                metadata:
                    event.metadata || {},

                previousEventHash:
                    event.previousEventHash || "",

                timestamp:
                    event.timestamp
            };

            const calculatedHash =
                buildAuditHash(payload);

            const previousMatches =
                (event.previousEventHash || "") ===
                previousHash;

            const hashMatches =
                calculatedHash ===
                event.eventHash;

            const valid =
                previousMatches &&
                hashMatches;

            if (!valid) {
                validChain = false;
            }

            previousHash =
                event.eventHash;

            return {
                ...event,
                calculatedHash,
                hashValid: hashMatches,
                chainValid: previousMatches,
                valid
            };
        });

    return {
        caseId,
        totalEvents:
            verifiedEvents.length,
        validChain,
        events:
            verifiedEvents
    };
};

const createForensicCase = async (
    data,
    user
) => {
    if (
        !user ||
        user.role !== "CUSTOMER"
    ) {
        throw new AppError(
            "Only customers can create forensic cases",
            403
        );
    }

    if (
        data.sourceType ===
        "PHYSICAL_DEVICE" &&
        !data.sourceIdentifier
    ) {
        throw new AppError(
            "Physical device identifier is required",
            400
        );
    }

    const center =
        await WorkstationCenter.findOne({
            centerId:
                data.workstationCenter,
            status: "ACTIVE"
        });

    if (!center) {
        throw new AppError(
            "Selected workstation center is not active or does not exist",
            400
        );
    }

    const caseId =
        await generateCaseId();

    const item =
        await ForensicCase.create({
            caseId,
            customer:
                user._id,
            title:
                data.title,
            description:
                data.description || "",
            sourceType:
                data.sourceType,
            sourceName:
                data.sourceName,
            sourceIdentifier:
                data.sourceIdentifier || "",
            deviceType:
                data.deviceType || "",
            capacity:
                data.capacity || "",
            assetIdentifier:
                data.assetIdentifier || "",
            workstationCenter:
                center._id,
            readOnly:
                true,
            status:
                "PENDING",
            history: [{
                status:
                    "PENDING",
                changedBy:
                    user._id,
                changedAt:
                    new Date(),
                note:
                    "Forensic case created"
            }]
        });

    await createAuditEvent({
        caseItem:
            item,
        action:
            "CASE_CREATED",
        user,
        fromStatus:
            "",
        toStatus:
            "PENDING",
        note:
            "Forensic case created",
        metadata: {
            sourceType:
                item.sourceType,
            sourceName:
                item.sourceName,
            deviceType:
                item.deviceType,
            capacity:
                item.capacity,
            assetIdentifier:
                item.assetIdentifier
        }
    });

    return populateCase(
        ForensicCase.findOne({
            caseId
        })
    );
};

const getCasesForUser =
    async user => {
        const filter = {};

        if (
            user.role ===
            "CUSTOMER"
        ) {
            filter.customer =
                user._id;
        }

        if (
            user.role ===
            "WORKSTATION_EMPLOYEE"
        ) {
            filter.assignedEmployee =
                user._id;
        }

        if (
            user.role ===
            "WORKSTATION_HEAD"
        ) {
            filter.workstationCenter =
                user.workstationCenter;
        }

        return populateCase(
            ForensicCase.find(
                filter
            ).sort({
                createdAt: -1
            })
        );
    };

const getCaseById =
    async (
        caseId,
        user
    ) => {
        const item =
            await populateCase(
                ForensicCase.findOne({
                    caseId
                })
            );

        ensureCaseAccess(
            item,
            user
        );

        return item;
    };

const getDashboard =
    async user => {
        const filter = {};

        if (
            user.role ===
            "CUSTOMER"
        ) {
            filter.customer =
                user._id;
        }

        if (
            user.role ===
            "WORKSTATION_EMPLOYEE"
        ) {
            filter.assignedEmployee =
                user._id;
        }

        if (
            user.role ===
            "WORKSTATION_HEAD"
        ) {
            filter.workstationCenter =
                user.workstationCenter;
        }

        const [
            cases,
            aggregate
        ] = await Promise.all([
            populateCase(
                ForensicCase.find(
                    filter
                )
                    .sort({
                        updatedAt: -1
                    })
                    .limit(8)
            ),

            ForensicCase.aggregate([
                {
                    $match:
                        filter
                },

                {
                    $group: {
                        _id:
                            null,

                        totalCases:
                        {
                            $sum:
                                1
                        },

                        activeCases:
                        {
                            $sum: {
                                $cond: [
                                    {
                                        $in: [
                                            "$status",
                                            [
                                                "PENDING",
                                                "ASSIGNED",
                                                "ACQUIRING",
                                                "ANALYZING"
                                            ]
                                        ]
                                    },
                                    1,
                                    0
                                ]
                            }
                        },

                        completedCases:
                        {
                            $sum: {
                                $cond: [
                                    {
                                        $eq: [
                                            "$status",
                                            "COMPLETED"
                                        ]
                                    },
                                    1,
                                    0
                                ]
                            }
                        },

                        failedCases:
                        {
                            $sum: {
                                $cond: [
                                    {
                                        $eq: [
                                            "$status",
                                            "FAILED"
                                        ]
                                    },
                                    1,
                                    0
                                ]
                            }
                        },

                        artifacts:
                        {
                            $sum:
                                "$recoveredArtifacts"
                        },

                        validatedArtifacts:
                        {
                            $sum:
                                "$validatedArtifacts"
                        },

                        highConfidenceArtifacts:
                        {
                            $sum:
                                "$highConfidenceArtifacts"
                        },

                        recoveredBytes:
                        {
                            $sum:
                                "$recoveredBytes"
                        }
                    }
                }
            ])
        ]);

        return {
            stats:
                aggregate[0] || {
                    totalCases:
                        0,

                    activeCases:
                        0,

                    completedCases:
                        0,

                    failedCases:
                        0,

                    artifacts:
                        0,

                    validatedArtifacts:
                        0,

                    highConfidenceArtifacts:
                        0,

                    recoveredBytes:
                        0
                },

            recentCases:
                cases
        };
    };

const assignCase =
    async (
        caseId,
        data,
        user
    ) => {
        if (
            !user ||
            ![
                "WORKSTATION_HEAD",
                "ADMIN"
            ].includes(
                user.role
            )
        ) {
            throw new AppError(
                "Only a workstation head or admin can assign forensic cases",
                403
            );
        }

        const item =
            await ForensicCase.findOne({
                caseId
            });

        if (!item) {
            throw new AppError(
                "Forensic case not found",
                404
            );
        }

        if (
            user.role ===
            "WORKSTATION_HEAD" &&
            String(
                item.workstationCenter
            ) !==
            String(
                user.workstationCenter
            )
        ) {
            throw new AppError(
                "Case does not belong to your workstation center",
                403
            );
        }

        if (
            ![
                "PENDING",
                "ASSIGNED"
            ].includes(
                item.status
            )
        ) {
            throw new AppError(
                `Case cannot be assigned because its current status is ${item.status}`,
                400
            );
        }

        const centerId =
            item.workstationCenter ||
            user.workstationCenter;

        if (!centerId) {
            throw new AppError(
                "A workstation center is required before assignment",
                400
            );
        }

        if (
            !item.workstationCenter
        ) {
            item.workstationCenter =
                centerId;
        }

        if (
            !data?.employeeId
        ) {
            throw new AppError(
                "Employee is required",
                400
            );
        }

        if (
            !data?.workstationId
        ) {
            throw new AppError(
                "A workstation is required before assignment",
                400
            );
        }

        const employee =
            await User.findOne({
                _id:
                    data.employeeId,
                role:
                    "WORKSTATION_EMPLOYEE",
                status:
                    "ACTIVE",
                workstationCenter:
                    centerId
            });

        if (!employee) {
            throw new AppError(
                "Eligible workstation employee not found",
                404
            );
        }

        const selectedWorkstation =
            await Workstation.findOne({
                _id:
                    data.workstationId,
                workstationCenter:
                    centerId,
                assignedEmployee:
                    employee._id,
                status:
                    "ACTIVE"
            });

        if (!selectedWorkstation) {
            throw new AppError(
                "Selected workstation is not assigned to this employee or is not active",
                400
            );
        }

        const previousStatus =
            item.status;

        item.assignedEmployee =
            employee._id;

        item.assignedWorkstation =
            selectedWorkstation._id;

        if (
            item.status !==
            "ASSIGNED"
        ) {
            ensureStatusTransition(
                item.status,
                "ASSIGNED"
            );
        }

        addHistory(
            item,
            "ASSIGNED",
            user,
            `Assigned to ${employee.name}`
        );

        await item.save();

        await createAuditEvent({
            caseItem:
                item,
            action:
                "CASE_ASSIGNED",
            user,
            fromStatus:
                previousStatus,
            toStatus:
                "ASSIGNED",
            workstation:
                selectedWorkstation,
            workstationId:
                selectedWorkstation.workstationId,
            workstationName:
                selectedWorkstation.name,
            note:
                `Assigned to ${employee.name}`,
            metadata: {
                employeeId:
                    String(
                        employee._id
                    ),
                employeeName:
                    employee.name,
                workstationId:
                    selectedWorkstation.workstationId
            }
        });

        return populateCase(
            ForensicCase.findOne({
                caseId
            })
        );
    };

const updateCaseStatus =
    async (
        caseId,
        status,
        note,
        user,
        workstationId = ""
    ) => {
        const allowed = [
            "ACQUIRING",
            "ANALYZING",
            "COMPLETED",
            "FAILED",
            "CANCELLED"
        ];

        if (
            !allowed.includes(
                status
            )
        ) {
            throw new AppError(
                "Invalid forensic case status",
                400
            );
        }

        const item =
            await ForensicCase.findOne({
                caseId
            });

        ensureCaseAccess(
            item,
            user
        );

        if (
            ![
                "WORKSTATION_EMPLOYEE",
                "ADMIN",
                "WORKSTATION_HEAD"
            ].includes(
                user.role
            )
        ) {
            throw new AppError(
                "You cannot update this forensic case",
                403
            );
        }

        ensureStatusTransition(
            item.status,
            status
        );

        let workstation =
            null;

        if (
            user.role ===
            "WORKSTATION_EMPLOYEE"
        ) {
            workstation =
                await getAssignedWorkstation(
                    item,
                    user,
                    workstationId
                );
        } else if (
            item.assignedWorkstation
        ) {
            workstation =
                await Workstation.findById(
                    item.assignedWorkstation
                );

            if (!workstation) {
                throw new AppError(
                    "Assigned workstation not found",
                    404
                );
            }
        }

        const previousStatus =
            item.status;

        if (
            status ===
            "ACQUIRING" &&
            !item.startedAt
        ) {
            item.startedAt =
                new Date();
        }

        if (
            status ===
            "COMPLETED"
        ) {
            item.completedAt =
                new Date();

            item.progress =
                100;
        }

        if (
            status ===
            "FAILED"
        ) {
            item.failedAt =
                new Date();

            item.failureReason =
                note ||
                item.failureReason;
        }

        if (
            status ===
            "CANCELLED"
        ) {
            item.failureReason =
                note ||
                item.failureReason;
        }

        addHistory(
            item,
            status,
            user,
            note ||
            "Status updated"
        );

        await item.save();

        let action =
            "CASE_STATUS_UPDATED";

        if (
            status ===
            "ACQUIRING"
        ) {
            action =
                "ACQUISITION_STARTED";
        } else if (
            status ===
            "ANALYZING"
        ) {
            action =
                "ANALYSIS_STARTED";
        } else if (
            status ===
            "COMPLETED"
        ) {
            action =
                "CASE_COMPLETED";
        } else if (
            status ===
            "FAILED"
        ) {
            action =
                "CASE_FAILED";
        } else if (
            status ===
            "CANCELLED"
        ) {
            action =
                "CASE_CANCELLED";
        }

        await createAuditEvent({
            caseItem:
                item,
            action,
            user,
            fromStatus:
                previousStatus,
            toStatus:
                status,
            workstation,
            workstationId:
                workstation?.workstationId ||
                workstationId ||
                "",
            workstationName:
                workstation?.name ||
                "",
            note:
                note ||
                "Status updated",
            metadata: {
                progress:
                    item.progress,
                failureReason:
                    item.failureReason ||
                    ""
            }
        });

        return populateCase(
            ForensicCase.findOne({
                caseId
            })
        );
    };

const ingestResult =
    async (
        caseId,
        payload,
        user
    ) => {
        const item =
            await ForensicCase.findOne({
                caseId
            });

        ensureCaseAccess(
            item,
            user
        );

        if (
            !user ||
            ![
                "WORKSTATION_EMPLOYEE",
                "ADMIN"
            ].includes(
                user.role
            )
        ) {
            throw new AppError(
                "Only the assigned workstation employee or admin can submit forensic results",
                403
            );
        }

        if (
            !payload ||
            typeof payload !==
            "object"
        ) {
            throw new AppError(
                "Forensic result payload is required",
                400
            );
        }

        if (
            user.role ===
            "WORKSTATION_EMPLOYEE" &&
            String(
                item.assignedEmployee
            ) !==
            String(
                user._id
            )
        ) {
            throw new AppError(
                "Case is not assigned to you",
                403
            );
        }

        if (
            !item.assignedWorkstation
        ) {
            throw new AppError(
                "No workstation is assigned to this case",
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
                `Forensic results cannot be submitted while case is ${item.status}`,
                400
            );
        }

        let workstation =
            null;

        if (
            user.role ===
            "WORKSTATION_EMPLOYEE"
        ) {
            workstation =
                await getAssignedWorkstation(
                    item,
                    user,
                    payload.workstationId ||
                    ""
                );
        } else {
            workstation =
                await Workstation.findById(
                    item.assignedWorkstation
                );

            if (!workstation) {
                throw new AppError(
                    "Assigned workstation not found",
                    404
                );
            }
        }

        if (
            !Array.isArray(
                payload.artifacts
            )
        ) {
            throw new AppError(
                "Artifacts must be an array",
                400
            );
        }

        if (
            payload.sourceIdentifier &&
            item.sourceIdentifier &&
            String(
                payload.sourceIdentifier
            ) !==
            String(
                item.sourceIdentifier
            )
        ) {
            throw new AppError(
                "Source device identifier does not match the forensic case",
                400
            );
        }

        if (
            ![
                "COMPLETED",
                "FAILED"
            ].includes(
                payload.status
            )
        ) {
            throw new AppError(
                "Result submission status must be COMPLETED or FAILED",
                400
            );
        }

        /*
         * The desktop flow submits its final result after acquisition.
         * If the case is still ACQUIRING, move it through ANALYZING first
         * so the persisted lifecycle remains:
         *
         * PENDING → ASSIGNED → ACQUIRING → ANALYZING → COMPLETED/FAILED
         */
        if (
            item.status ===
            "ACQUIRING"
        ) {
            const analysisStartedAt =
                new Date();

            item.status =
                "ANALYZING";

            item.history.push({
                status:
                    "ANALYZING",
                changedBy:
                    user._id,
                changedAt:
                    analysisStartedAt,
                note:
                    "Forensic analysis started while processing submitted acquisition results"
            });

            await item.save();

            await createAuditEvent({
                caseItem:
                    item,
                action:
                    "ANALYSIS_STARTED",
                user,
                fromStatus:
                    "ACQUIRING",
                toStatus:
                    "ANALYZING",
                workstation,
                workstationId:
                    workstation?.workstationId ||
                    payload.workstationId ||
                    "",
                workstationName:
                    workstation?.name ||
                    "",
                note:
                    "Forensic analysis started while processing submitted acquisition results"
            });
        }

        const previousStatus =
            item.status;

        item.bytesScanned =
            Number(
                payload.bytesScanned
            ) || 0;

        item.totalBytes =
            Number(
                payload.totalBytes
            ) ||
            item.totalBytes ||
            0;

        item.candidatesFound =
            Number(
                payload.candidatesFound
            ) || 0;

        item.recoveredArtifacts =
            Number(
                payload.recoveredArtifacts
            ) ||
            payload.artifacts.length;

        item.validatedArtifacts =
            Number(
                payload.validatedArtifacts
            ) ||
            payload.artifacts.filter(
                a => a.validated
            ).length;

        item.rejectedArtifacts =
            Number(
                payload.rejectedArtifacts
            ) || 0;

        item.highConfidenceArtifacts =
            Number(
                payload.highConfidenceArtifacts
            ) ||
            payload.artifacts.filter(
                a =>
                    a.confidenceLevel ===
                    "HIGH"
            ).length;

        item.recoveredBytes =
            Number(
                payload.recoveredBytes
            ) ||
            payload.artifacts.reduce(
                (
                    sum,
                    a
                ) =>
                    sum +
                    (
                        Number(
                            a.size
                        ) || 0
                    ),
                0
            );

        item.progress =
            100;

        item.artifacts =
            payload.artifacts.map(
                (a, index) => ({
                    artifactId:
                        String(
                            a.artifactId ||
                            `ART-${String(
                                index + 1
                            ).padStart(
                                4,
                                "0"
                            )}`
                        ),

                    fileName:
                        String(
                            a.fileName ||
                            ""
                        ),

                    fileType:
                        String(
                            a.fileType ||
                            "JPEG"
                        ),

                    offset:
                        Number(
                            a.offset
                        ) || 0,

                    size:
                        Number(
                            a.size
                        ) || 0,

                    recoveredPath:
                        String(
                            a.recoveredPath ||
                            ""
                        ),

                    headerValid:
                        Boolean(
                            a.headerValid
                        ),

                    footerValid:
                        Boolean(
                            a.footerValid
                        ),

                    structureValid:
                        Boolean(
                            a.structureValid
                        ),

                    sizeValid:
                        Boolean(
                            a.sizeValid
                        ),

                    decodable:
                        Boolean(
                            a.decodable
                        ),

                    confidenceScore:
                        Math.max(
                            0,
                            Math.min(
                                100,
                                Number(
                                    a.confidenceScore
                                ) || 0
                            )
                        ),

                    confidenceLevel:
                        [
                            "HIGH",
                            "MEDIUM",
                            "LOW",
                            "REJECTED"
                        ].includes(
                            a.confidenceLevel
                        )
                            ? a.confidenceLevel
                            : "LOW",

                    confidenceReasons:
                        Array.isArray(
                            a.confidenceReasons
                        )
                            ? a.confidenceReasons
                                .slice(
                                    0,
                                    20
                                )
                                .map(
                                    String
                                )
                            : [],

                    sha256:
                        String(
                            a.sha256 ||
                            ""
                        ),

                    recovered:
                        a.recovered !==
                        false,

                    validated:
                        Boolean(
                            a.validated
                        )
                })
            );

        const finalStatus =
            payload.status;

        ensureStatusTransition(
            item.status,
            finalStatus
        );

        item.status =
            finalStatus;

        if (
            finalStatus ===
            "COMPLETED"
        ) {
            item.completedAt =
                new Date();

            item.failedAt =
                null;

            item.failureReason =
                "";
        } else {
            item.failedAt =
                new Date();

            item.completedAt =
                null;

            item.failureReason =
                String(
                    payload.failureReason ||
                    "Forensic acquisition failed"
                );
        }

        item.history.push({
            status:
                finalStatus,

            changedBy:
                user._id,

            changedAt:
                new Date(),

            note:
                finalStatus ===
                    "FAILED"
                    ? item.failureReason
                    : "Forensic results submitted from workstation"
        });

        await item.save();

        await createAuditEvent({
            caseItem:
                item,
            action:
                "EVIDENCE_SUBMITTED",
            user,
            fromStatus:
                previousStatus,
            toStatus:
                finalStatus,
            workstation,
            workstationId:
                workstation?.workstationId ||
                payload.workstationId ||
                "",
            workstationName:
                workstation?.name ||
                "",
            note:
                finalStatus ===
                    "FAILED"
                    ? item.failureReason
                    : "Forensic results submitted from workstation",
            metadata: {
                bytesScanned:
                    item.bytesScanned,

                totalBytes:
                    item.totalBytes,

                candidatesFound:
                    item.candidatesFound,

                recoveredArtifacts:
                    item.recoveredArtifacts,

                validatedArtifacts:
                    item.validatedArtifacts,

                rejectedArtifacts:
                    item.rejectedArtifacts,

                highConfidenceArtifacts:
                    item.highConfidenceArtifacts,

                recoveredBytes:
                    item.recoveredBytes,

                artifactCount:
                    item.artifacts.length
            }
        });

        await createAuditEvent({
            caseItem:
                item,
            action:
                finalStatus ===
                    "COMPLETED"
                    ? "CASE_COMPLETED"
                    : "CASE_FAILED",
            user,
            fromStatus:
                finalStatus,
            toStatus:
                finalStatus,
            workstation,
            workstationId:
                workstation?.workstationId ||
                payload.workstationId ||
                "",
            workstationName:
                workstation?.name ||
                "",
            note:
                finalStatus ===
                    "COMPLETED"
                    ? "Forensic scan completed and results accepted"
                    : item.failureReason,
            metadata: {
                artifactCount:
                    item.artifacts.length
            }
        });

        return populateCase(
            ForensicCase.findOne({
                caseId
            })
        );
    };

const generateReport =
    async (
        caseId,
        user
    ) => {
        const item =
            await ForensicCase.findOne({
                caseId
            });

        ensureCaseAccess(
            item,
            user
        );

        if (
            item.status !==
            "COMPLETED"
        ) {
            throw new AppError(
                "A forensic report can only be generated after a completed case",
                400
            );
        }

        let workstation =
            null;

        if (
            item.assignedWorkstation
        ) {
            workstation =
                await Workstation.findById(
                    item.assignedWorkstation
                );

            if (!workstation) {
                throw new AppError(
                    "Assigned workstation not found",
                    404
                );
            }
        }

        const auditSnapshot =
            await ForensicAuditLog.find({
                caseId
            })
                .sort({
                    sequence: 1
                })
                .lean();

        const reportPayload = {
            caseId:
                item.caseId,

            source: {
                type:
                    item.sourceType,

                name:
                    item.sourceName,

                identifier:
                    item.sourceIdentifier,

                readOnly:
                    item.readOnly
            },

            assignment: {
                workstationCenter:
                    item.workstationCenter,

                assignedEmployee:
                    item.assignedEmployee,

                assignedWorkstation:
                    item.assignedWorkstation
            },

            summary: {
                bytesScanned:
                    item.bytesScanned,

                totalBytes:
                    item.totalBytes,

                candidatesFound:
                    item.candidatesFound,

                recoveredArtifacts:
                    item.recoveredArtifacts,

                validatedArtifacts:
                    item.validatedArtifacts,

                rejectedArtifacts:
                    item.rejectedArtifacts,

                highConfidenceArtifacts:
                    item.highConfidenceArtifacts,

                recoveredBytes:
                    item.recoveredBytes
            },

            artifacts:
                item.artifacts.map(
                    a => ({
                        artifactId:
                            a.artifactId,

                        fileName:
                            a.fileName,

                        fileType:
                            a.fileType,

                        offset:
                            a.offset,

                        size:
                            a.size,

                        confidenceScore:
                            a.confidenceScore,

                        confidenceLevel:
                            a.confidenceLevel,

                        confidenceReasons:
                            a.confidenceReasons,

                        sha256:
                            a.sha256,

                        validated:
                            a.validated,

                        validation: {
                            headerValid:
                                a.headerValid,

                            footerValid:
                                a.footerValid,

                            structureValid:
                                a.structureValid,

                            sizeValid:
                                a.sizeValid,

                            decodable:
                                a.decodable
                        }
                    })
                ),

            auditTrail:
                auditSnapshot.map(
                    event => ({
                        auditId:
                            event.auditId,

                        sequence:
                            event.sequence,

                        action:
                            event.action,

                        fromStatus:
                            event.fromStatus,

                        toStatus:
                            event.toStatus,

                        actorName:
                            event.actorName,

                        actorRole:
                            event.actorRole,

                        workstationId:
                            event.workstationId,

                        workstationName:
                            event.workstationName,

                        note:
                            event.note,

                        previousEventHash:
                            event.previousEventHash,

                        eventHash:
                            event.eventHash,

                        timestamp:
                            event.timestamp
                    })
                ),

            generatedAt:
                new Date().toISOString()
        };

        const reportHash =
            crypto
                .createHash(
                    "sha256"
                )
                .update(
                    JSON.stringify(
                        reportPayload
                    )
                )
                .digest(
                    "hex"
                );

        item.report = {
            generated:
                true,

            generatedAt:
                new Date(),

            reportHash
        };

        await item.save();

        await createAuditEvent({
            caseItem:
                item,

            action:
                "REPORT_GENERATED",

            user,

            fromStatus:
                item.status,

            toStatus:
                item.status,

            workstation,

            workstationId:
                workstation?.workstationId ||
                "",

            workstationName:
                workstation?.name ||
                "",

            note:
                "Forensic report generated",

            metadata: {
                reportHash,

                artifactCount:
                    item.artifacts.length
            }
        });

        return {
            report:
                reportPayload,

            reportHash
        };
    };

module.exports = {
    createForensicCase,
    getCasesForUser,
    getCaseById,
    getDashboard,
    assignCase,
    updateCaseStatus,
    ingestResult,
    generateReport,
    getForensicAuditTrail
};