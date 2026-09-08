const crypto = require("crypto");
const ForensicCase = require("../models/ForensicCase");
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

const populateCase = query => query
    .populate("customer", "name email role")
    .populate("workstationCenter", "centerId name location status")
    .populate("assignedEmployee", "name email role")
    .populate("assignedWorkstation", "workstationId name status connectionStatus hostname");

const ensureCaseAccess = (item, user) => {
    if (!item) {
        throw new AppError("Forensic case not found", 404);
    }

    const id = String(user._id);

    if (user.role === "ADMIN") {
        return;
    }

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

/*
 * The forensic case lifecycle is deliberately controlled here.
 *
 * PENDING
 *   -> ASSIGNED
 *   -> CANCELLED
 *
 * ASSIGNED
 *   -> ACQUIRING
 *   -> CANCELLED
 *
 * ACQUIRING
 *   -> ANALYZING
 *   -> FAILED
 *   -> CANCELLED
 *
 * ANALYZING
 *   -> COMPLETED
 *   -> FAILED
 *   -> CANCELLED
 *
 * COMPLETED / FAILED / CANCELLED are terminal states.
 */
const allowedStatusTransitions = {
    PENDING: ["ASSIGNED", "CANCELLED"],
    ASSIGNED: ["ACQUIRING", "CANCELLED"],
    ACQUIRING: ["ANALYZING", "FAILED", "CANCELLED"],
    ANALYZING: ["COMPLETED", "FAILED", "CANCELLED"],
    COMPLETED: [],
    FAILED: [],
    CANCELLED: []
};

/*
 * For a workstation employee, the JWT identity alone is not enough.
 *
 * The desktop application must also identify the workstation on which
 * the forensic acquisition is being performed.
 *
 * The backend verifies:
 *
 * employee
 *    -> case assignment
 *    -> case workstation
 *    -> workstation center
 *    -> workstation employee assignment
 *    -> active workstation
 */
const ensureAssignedWorkstation = async (
    item,
    user,
    workstationId
) => {
    if (user.role !== "WORKSTATION_EMPLOYEE") {
        return null;
    }

    if (
        !item.assignedEmployee ||
        String(item.assignedEmployee) !== String(user._id)
    ) {
        throw new AppError(
            "Case is not assigned to you",
            403
        );
    }

    if (!item.assignedWorkstation) {
        throw new AppError(
            "No workstation is assigned to this forensic case",
            409
        );
    }

    if (!workstationId) {
        throw new AppError(
            "Assigned workstation identity is required",
            400
        );
    }

    const workstation = await Workstation.findOne({
        _id: workstationId,
        workstationCenter: item.workstationCenter,
        assignedEmployee: user._id,
        status: "ACTIVE"
    });

    if (!workstation) {
        throw new AppError(
            "Workstation is not assigned to this employee",
            403
        );
    }

    if (
        String(workstation._id) !==
        String(item.assignedWorkstation)
    ) {
        throw new AppError(
            "Workstation does not match the workstation assigned to this case",
            403
        );
    }

    return workstation;
};

const createForensicCase = async (data, user) => {
    if (user.role !== "CUSTOMER") {
        throw new AppError(
            "Only customers can create forensic cases",
            403
        );
    }

    if (
        data.sourceType === "PHYSICAL_DEVICE" &&
        !data.sourceIdentifier
    ) {
        throw new AppError(
            "Physical device identifier is required",
            400
        );
    }

    const center = await WorkstationCenter.findOne({
        centerId: data.workstationCenter,
        status: "ACTIVE"
    });

    if (!center) {
        throw new AppError(
            "Selected workstation center is not active or does not exist",
            400
        );
    }

    const caseId = await generateCaseId();

    return ForensicCase.create({
        caseId,
        customer: user._id,
        title: data.title,
        description: data.description || "",
        sourceType: data.sourceType,
        sourceName: data.sourceName,
        sourceIdentifier: data.sourceIdentifier || "",
        deviceType: data.deviceType || "",
        capacity: data.capacity || "",
        assetIdentifier: data.assetIdentifier || "",
        workstationCenter: center._id,
        readOnly: true,
        status: "PENDING",
        history: [
            {
                status: "PENDING",
                changedBy: user._id,
                note: "Forensic case created"
            }
        ]
    });
};

const getCasesForUser = async user => {
    const filter = {};

    if (user.role === "CUSTOMER") {
        filter.customer = user._id;
    }

    if (user.role === "WORKSTATION_EMPLOYEE") {
        filter.assignedEmployee = user._id;
    }

    if (user.role === "WORKSTATION_HEAD") {
        filter.workstationCenter = user.workstationCenter;
    }

    return populateCase(
        ForensicCase
            .find(filter)
            .sort({ createdAt: -1 })
    );
};

const getCaseById = async (caseId, user) => {
    const item = await populateCase(
        ForensicCase.findOne({ caseId })
    );

    ensureCaseAccess(item, user);

    return item;
};

const getDashboard = async user => {
    const filter = {};

    if (user.role === "CUSTOMER") {
        filter.customer = user._id;
    }

    if (user.role === "WORKSTATION_EMPLOYEE") {
        filter.assignedEmployee = user._id;
    }

    if (user.role === "WORKSTATION_HEAD") {
        filter.workstationCenter = user.workstationCenter;
    }

    const [cases, aggregate] = await Promise.all([
        populateCase(
            ForensicCase
                .find(filter)
                .sort({ updatedAt: -1 })
                .limit(8)
        ),

        ForensicCase.aggregate([
            { $match: filter },

            {
                $group: {
                    _id: null,

                    totalCases: {
                        $sum: 1
                    },

                    activeCases: {
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

                    completedCases: {
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

                    failedCases: {
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

                    artifacts: {
                        $sum: "$recoveredArtifacts"
                    },

                    validatedArtifacts: {
                        $sum: "$validatedArtifacts"
                    },

                    highConfidenceArtifacts: {
                        $sum: "$highConfidenceArtifacts"
                    },

                    recoveredBytes: {
                        $sum: "$recoveredBytes"
                    }
                }
            }
        ])
    ]);

    return {
        stats: aggregate[0] || {
            totalCases: 0,
            activeCases: 0,
            completedCases: 0,
            failedCases: 0,
            artifacts: 0,
            validatedArtifacts: 0,
            highConfidenceArtifacts: 0,
            recoveredBytes: 0
        },

        recentCases: cases
    };
};

const assignCase = async (
    caseId,
    data,
    user
) => {
    if (
        !["WORKSTATION_HEAD", "ADMIN"].includes(
            user.role
        )
    ) {
        throw new AppError(
            "Only a workstation head or admin can assign forensic cases",
            403
        );
    }

    const item = await ForensicCase.findOne({
        caseId
    });

    if (!item) {
        throw new AppError(
            "Forensic case not found",
            404
        );
    }

    if (
        user.role === "WORKSTATION_HEAD" &&
        String(item.workstationCenter) !==
            String(user.workstationCenter)
    ) {
        throw new AppError(
            "Case does not belong to your workstation center",
            403
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

    if (!item.workstationCenter) {
        item.workstationCenter = centerId;
    }

    const employee = await User.findOne({
        _id: data.employeeId,
        role: "WORKSTATION_EMPLOYEE",
        status: "ACTIVE",
        workstationCenter: centerId
    });

    if (!employee) {
        throw new AppError(
            "Eligible workstation employee not found",
            404
        );
    }

    /*
     * A forensic case should have a workstation assigned.
     * This makes the later desktop-to-case binding deterministic.
     */
    if (!data.workstationId) {
        throw new AppError(
            "A workstation must be selected for forensic acquisition",
            400
        );
    }

    const workstation = await Workstation.findOne({
        _id: data.workstationId,
        workstationCenter: centerId,
        assignedEmployee: employee._id,
        status: "ACTIVE"
    });

    if (!workstation) {
        throw new AppError(
            "Selected workstation is not assigned to this employee",
            400
        );
    }

    item.assignedWorkstation =
        workstation._id;

    item.assignedEmployee =
        employee._id;

    if (
        item.status !== "PENDING" &&
        item.status !== "ASSIGNED"
    ) {
        throw new AppError(
            `Case cannot be assigned while it is ${item.status}`,
            409
        );
    }

    addHistory(
        item,
        "ASSIGNED",
        user,
        `Assigned to ${employee.name} on workstation ${workstation.workstationId}`
    );

    await item.save();

    return populateCase(
        ForensicCase.findOne({ caseId })
    );
};

const updateCaseStatus = async (
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

    if (!allowed.includes(status)) {
        throw new AppError(
            "Invalid forensic case status",
            400
        );
    }

    const item = await ForensicCase.findOne({
        caseId
    });

    ensureCaseAccess(item, user);

    if (
        ![
            "WORKSTATION_EMPLOYEE",
            "ADMIN",
            "WORKSTATION_HEAD"
        ].includes(user.role)
    ) {
        throw new AppError(
            "You cannot update this forensic case",
            403
        );
    }

    if (
        user.role === "WORKSTATION_EMPLOYEE"
    ) {
        await ensureAssignedWorkstation(
            item,
            user,
            workstationId
        );
    }

    const nextStatuses =
        allowedStatusTransitions[item.status] ||
        [];

    if (!nextStatuses.includes(status)) {
        throw new AppError(
            `Invalid status transition: ${item.status} -> ${status}`,
            409
        );
    }

    if (
        status === "ACQUIRING" &&
        !item.startedAt
    ) {
        item.startedAt = new Date();
    }

    if (status === "COMPLETED") {
        item.completedAt = new Date();
        item.progress = 100;
    }

    if (status === "FAILED") {
        item.failedAt = new Date();
        item.failureReason =
            note || item.failureReason;
    }

    addHistory(
        item,
        status,
        user,
        note || "Status updated"
    );

    await item.save();

    return populateCase(
        ForensicCase.findOne({ caseId })
    );
};

const ingestResult = async (
    caseId,
    payload,
    user
) => {
    const item = await ForensicCase.findOne({
        caseId
    });

    ensureCaseAccess(item, user);

    if (
        !["WORKSTATION_EMPLOYEE", "ADMIN"].includes(
            user.role
        )
    ) {
        throw new AppError(
            "Only the assigned workstation employee or admin can submit forensic results",
            403
        );
    }

    if (
        user.role === "WORKSTATION_EMPLOYEE"
    ) {
        await ensureAssignedWorkstation(
            item,
            user,
            payload.workstationId
        );
    }

    if (!Array.isArray(payload.artifacts)) {
        throw new AppError(
            "Artifacts must be an array",
            400
        );
    }

    /*
     * The desktop is expected to send the source identity
     * that it actually scanned.
     *
     * If the case has a registered source identity and the
     * desktop sends a different identity, reject the result.
     */
    if (
        payload.sourceIdentifier &&
        item.sourceIdentifier &&
        String(payload.sourceIdentifier) !==
            String(item.sourceIdentifier)
    ) {
        throw new AppError(
            "Evidence source does not match the source registered for this case",
            409
        );
    }

    /*
     * Results may only be submitted after acquisition
     * has started.
     */
    if (
        !["ACQUIRING", "ANALYZING"].includes(
            item.status
        )
    ) {
        throw new AppError(
            `Results cannot be submitted while the case is ${item.status}`,
            409
        );
    }

    const requestedFinalStatus =
        payload.status === "FAILED"
            ? "FAILED"
            : "COMPLETED";

    if (
        !(
            allowedStatusTransitions[item.status] ||
            []
        ).includes(requestedFinalStatus)
    ) {
        throw new AppError(
            `Invalid result transition: ${item.status} -> ${requestedFinalStatus}`,
            409
        );
    }

    item.bytesScanned =
        Number(payload.bytesScanned) || 0;

    item.totalBytes =
        Number(payload.totalBytes) ||
        item.totalBytes ||
        0;

    item.candidatesFound =
        Number(payload.candidatesFound) || 0;

    item.recoveredArtifacts =
        Number(payload.recoveredArtifacts) ||
        payload.artifacts.length;

    item.validatedArtifacts =
        Number(payload.validatedArtifacts) ||
        payload.artifacts.filter(
            artifact => artifact.validated
        ).length;

    item.rejectedArtifacts =
        Number(payload.rejectedArtifacts) || 0;

    item.highConfidenceArtifacts =
        Number(payload.highConfidenceArtifacts) ||
        payload.artifacts.filter(
            artifact =>
                artifact.confidenceLevel === "HIGH"
        ).length;

    item.recoveredBytes =
        Number(payload.recoveredBytes) ||
        payload.artifacts.reduce(
            (sum, artifact) =>
                sum +
                (Number(artifact.size) || 0),
            0
        );

    item.progress = 100;

    item.artifacts =
        payload.artifacts.map(
            (artifact, index) => ({
                artifactId: String(
                    artifact.artifactId ||
                    `ART-${String(index + 1).padStart(4, "0")}`
                ),

                fileName: String(
                    artifact.fileName || ""
                ),

                fileType: String(
                    artifact.fileType || "JPEG"
                ),

                offset:
                    Number(artifact.offset) || 0,

                size:
                    Number(artifact.size) || 0,

                recoveredPath: String(
                    artifact.recoveredPath || ""
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

                confidenceScore: Math.max(
                    0,
                    Math.min(
                        100,
                        Number(
                            artifact.confidenceScore
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
                        artifact.confidenceLevel
                    )
                        ? artifact.confidenceLevel
                        : "LOW",

                confidenceReasons:
                    Array.isArray(
                        artifact.confidenceReasons
                    )
                        ? artifact.confidenceReasons
                              .slice(0, 20)
                              .map(String)
                        : [],

                sha256: String(
                    artifact.sha256 || ""
                ),

                recovered:
                    artifact.recovered !== false,

                validated:
                    Boolean(
                        artifact.validated
                    )
            })
        );

    item.status =
        requestedFinalStatus;

    if (item.status === "COMPLETED") {
        item.completedAt = new Date();
        item.failedAt = null;
        item.failureReason = "";
    }

    if (item.status === "FAILED") {
        item.failedAt = new Date();
        item.failureReason = String(
            payload.failureReason ||
            "Forensic acquisition failed"
        );
    }

    item.history.push({
        status: item.status,
        changedBy: user._id,
        note:
            item.status === "FAILED"
                ? item.failureReason
                : "Forensic results submitted from assigned workstation"
    });

    await item.save();

    return populateCase(
        ForensicCase.findOne({ caseId })
    );
};

const generateReport = async (
    caseId,
    user
) => {
    const item = await ForensicCase.findOne({
        caseId
    });

    ensureCaseAccess(item, user);

    if (item.status !== "COMPLETED") {
        throw new AppError(
            "A forensic report can only be generated after a completed case",
            400
        );
    }

    const reportPayload = {
        caseId: item.caseId,

        source: {
            type: item.sourceType,
            name: item.sourceName,
            identifier: item.sourceIdentifier,
            readOnly: item.readOnly
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

        artifacts: item.artifacts.map(
            artifact => ({
                artifactId:
                    artifact.artifactId,

                fileName:
                    artifact.fileName,

                fileType:
                    artifact.fileType,

                offset:
                    artifact.offset,

                size:
                    artifact.size,

                confidenceScore:
                    artifact.confidenceScore,

                confidenceLevel:
                    artifact.confidenceLevel,

                confidenceReasons:
                    artifact.confidenceReasons,

                sha256:
                    artifact.sha256,

                validated:
                    artifact.validated,

                validation: {
                    headerValid:
                        artifact.headerValid,

                    footerValid:
                        artifact.footerValid,

                    structureValid:
                        artifact.structureValid,

                    sizeValid:
                        artifact.sizeValid,

                    decodable:
                        artifact.decodable
                }
            })
        ),

        generatedAt:
            new Date().toISOString()
    };

    const reportHash =
        crypto
            .createHash("sha256")
            .update(
                JSON.stringify(
                    reportPayload
                )
            )
            .digest("hex");

    item.report = {
        generated: true,
        generatedAt: new Date(),
        reportHash
    };

    await item.save();

    return {
        report: reportPayload,
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
    generateReport
};