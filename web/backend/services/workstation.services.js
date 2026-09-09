const Workstation = require("../models/WorkStation");
const WorkstationCenter = require("../models/WorkstationCenter");
const SanitizationRequest = require("../models/SanitizationRequest");
const AppError = require("../utils/AppError");
const Counter = require("../models/Counter");

const generateWorkstationId = async () => {
    const counter = await Counter.findOneAndUpdate(
        { name: "workstation" },
        { $inc: { sequence: 1 } },
        { new: true, upsert: true }
    );

    return `WS-${String(counter.sequence).padStart(4, "0")}`;
};

const createWorkstation = async (data) => {
    const existingWorkstation = await Workstation.findOne({
        name: data.name
    });

    if (existingWorkstation) {
        throw new AppError(
            "A workstation with this name already exists",
            409
        );
    }

    const center = await WorkstationCenter.findOne({
        centerId: data.workstationCenterId
    });

    if (!center) {
        throw new AppError("Workstation center not found", 404);
    }

    if (center.status !== "ACTIVE") {
        throw new AppError(
            "Cannot add a workstation to an inactive center",
            400
        );
    }

    const workstationId = await generateWorkstationId();

    const workstation = await Workstation.create({
        ...data,
        workstationId,
        name: data.name,
        status: data.status || "ACTIVE",
        workstationCenter: center._id,
        assignedEmployee: null
    });

    return workstation;
};

/*
 * Safe workstation deletion.
 *
 * A workstation cannot be deleted while:
 * 1. It is assigned to an employee.
 * 2. It is referenced by an active sanitization request.
 *
 * This prevents dangling references.
 */
const deleteWorkstation = async (workstationId) => {
    const workstation = await Workstation.findOne({ workstationId });

    if (!workstation) {
        throw new AppError("Workstation not found", 404);
    }

    if (workstation.assignedEmployee) {
        throw new AppError(
            `Workstation ${workstation.workstationId} is assigned to an employee. Unassign the employee before deleting this workstation.`,
            409
        );
    }

    const activeRequest = await SanitizationRequest.findOne({
        assignedWorkstation: workstation._id,
        status: {
            $in: ["APPROVED", "ASSIGNED", "IN_PROGRESS", "VERIFYING"]
        }
    });

    if (activeRequest) {
        throw new AppError(
            `Workstation ${workstation.workstationId} is referenced by active sanitization request ${activeRequest.requestId} and cannot be deleted.`,
            409
        );
    }

    await workstation.deleteOne();

    return {
        workstationId: workstation.workstationId,
        deleted: true
    };
};

/*
 * Explicit workstation unassignment.
 *
 * This is safer than directly changing the database because it removes
 * the employee binding in one place.
 */
const unassignEmployee = async (workstationId) => {
    const workstation = await Workstation.findOne({ workstationId });

    if (!workstation) {
        throw new AppError("Workstation not found", 404);
    }

    if (!workstation.assignedEmployee) {
        return workstation;
    }

    const activeRequest = await SanitizationRequest.findOne({
        assignedWorkstation: workstation._id,
        status: {
            $in: ["APPROVED", "ASSIGNED", "IN_PROGRESS", "VERIFYING"]
        }
    });

    if (activeRequest) {
        throw new AppError(
            `Workstation ${workstation.workstationId} cannot be unassigned while sanitization request ${activeRequest.requestId} is active.`,
            409
        );
    }

    workstation.assignedEmployee = null;
    await workstation.save();

    return workstation;
};

/*
 * Bind the physical SecureWipe desktop installation to the workstation
 * record assigned to the authenticated employee.
 *
 * The desktop sends a SHA-256 fingerprint derived from the Windows
 * MachineGuid. The raw MachineGuid is never stored.
 *
 * First successful registration binds the workstation.
 * Later registrations must present the same fingerprint.
 */
const bindWorkstationIdentity = async (data, user) => {
    if (!user) {
        throw new AppError("Authentication required", 401);
    }

    if (user.role !== "WORKSTATION_EMPLOYEE") {
        throw new AppError(
            "Only workstation employees can bind a desktop workstation identity",
            403
        );
    }

    const workstationId = data?.workstationId?.trim();
    const machineFingerprint = data?.machineFingerprint?.trim();
    const legacyMachineFingerprint = data?.legacyMachineFingerprint?.trim();
    const hostname = data?.hostname?.trim() || null;
    const operatingSystem = data?.operatingSystem || {};

    if (!workstationId) {
        throw new AppError("Workstation ID is required", 400);
    }

    if (!machineFingerprint) {
        throw new AppError("Machine fingerprint is required", 400);
    }

    if (!/^[a-fA-F0-9]{64}$/.test(machineFingerprint)) {
        throw new AppError(
            "Machine fingerprint must be a SHA-256 hexadecimal value",
            400
        );
    }

    if (
        legacyMachineFingerprint &&
        !/^[a-fA-F0-9]{64}$/.test(legacyMachineFingerprint)
    ) {
        throw new AppError(
            "Legacy machine fingerprint must be a SHA-256 hexadecimal value",
            400
        );
    }

    const workstation = await Workstation.findOne({ workstationId });

    if (!workstation) {
        throw new AppError("Workstation not found", 404);
    }

    if (
        !workstation.assignedEmployee ||
        String(workstation.assignedEmployee) !== String(user._id)
    ) {
        throw new AppError(
            "This workstation is not assigned to the authenticated employee",
            403
        );
    }

    if (workstation.status !== "ACTIVE") {
        throw new AppError("The assigned workstation is not active", 409);
    }

    if (
        !workstation.workstationCenter ||
        (
            user.workstationCenter &&
            String(workstation.workstationCenter) !==
                String(user.workstationCenter)
        )
    ) {
        throw new AppError(
            "The workstation is not assigned to the employee's workstation center",
            403
        );
    }

    /*
     * First enrollment:
     * bind this workstation record to the current SecureWipe installation.
     *
     * A fingerprint cannot be bound to another workstation record.
     */
    if (!workstation.machineFingerprint) {
        const fingerprintOwner = await Workstation.findOne({
            $or: [
                { machineFingerprint },
                ...(legacyMachineFingerprint
                    ? [{ machineFingerprint: legacyMachineFingerprint }]
                    : [])
            ]
        });

        if (
            fingerprintOwner &&
            String(fingerprintOwner._id) !== String(workstation._id)
        ) {
            throw new AppError(
                `This SecureWipe machine is already bound to workstation ${fingerprintOwner.workstationId}`,
                409
            );
        }

        const claimed = await Workstation.findOneAndUpdate(
            {
                _id: workstation._id,
                assignedEmployee: user._id,
                status: "ACTIVE",
                $or: [
                    { machineFingerprint: null },
                    { machineFingerprint: { $exists: false } }
                ]
            },
            {
                $set: {
                    machineFingerprint,
                    identityBoundAt: new Date(),
                    identityLastSeenAt: new Date(),
                    hostname,
                    operatingSystem: {
                        name: operatingSystem.name || null,
                        version: operatingSystem.version || null,
                        architecture: operatingSystem.architecture || null
                    },
                    connectionStatus: "ONLINE",
                    lastSeen: new Date()
                }
            },
            { new: true }
        );

        if (!claimed) {
            const current = await Workstation.findById(workstation._id);

            if (
                current?.machineFingerprint &&
                current.machineFingerprint !== machineFingerprint
            ) {
                throw new AppError(
                    "This workstation is already bound to a different SecureWipe machine",
                    403
                );
            }

            throw new AppError(
                "The workstation became unavailable during identity binding. Try again.",
                409
            );
        }

        return claimed;
    }

    /*
     * Existing enrollment:
     * only the exact previously bound machine may continue using this workstation.
     */
    const fingerprintMatches =
        workstation.machineFingerprint === machineFingerprint;

    const legacyFingerprintMatches =
        !!legacyMachineFingerprint &&
        workstation.machineFingerprint === legacyMachineFingerprint;

    if (!fingerprintMatches && !legacyFingerprintMatches) {
        throw new AppError(
            "This physical workstation does not match the SecureWipe machine previously enrolled for this workstation ID",
            403
        );
    }

    /*
     * Compatibility:
     * If the database contains the older MachineGuid-only hash,
     * accept it once and upgrade the stored value to the new
     * SecureWipe|MachineGuid|... fingerprint.
     */
    if (legacyFingerprintMatches && !fingerprintMatches) {
        workstation.machineFingerprint = machineFingerprint;
    }

    workstation.identityLastSeenAt = new Date();
    workstation.hostname = hostname;

    workstation.operatingSystem = {
        name:
            operatingSystem.name ||
            workstation.operatingSystem?.name ||
            null,
        version:
            operatingSystem.version ||
            workstation.operatingSystem?.version ||
            null,
        architecture:
            operatingSystem.architecture ||
            workstation.operatingSystem?.architecture ||
            null
    };

    workstation.connectionStatus = "ONLINE";
    workstation.lastSeen = new Date();

    await workstation.save();

    return workstation;
};

module.exports = {
    createWorkstation,
    deleteWorkstation,
    unassignEmployee,
    bindWorkstationIdentity
};