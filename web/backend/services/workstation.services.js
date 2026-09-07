const Workstation =
    require("../models/WorkStation");

const WorkstationCenter =
    require("../models/WorkstationCenter");

const SanitizationRequest =
    require("../models/SanitizationRequest");

const AppError =
    require("../utils/AppError");

const Counter =
    require("../models/Counter");


const generateWorkstationId =
    async () => {

        const counter =
            await Counter.findOneAndUpdate(
                {
                    name:
                        "workstation"
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

        return `WS-${String(
            counter.sequence
        ).padStart(4, "0")}`;
    };


const createWorkstation =
    async (data) => {

        const existingWorkstation =
            await Workstation.findOne({
                name:
                    data.name
            });

        if (
            existingWorkstation
        ) {
            throw new AppError(
                "A workstation with this name already exists",
                409
            );
        }

        const center =
            await WorkstationCenter.findOne({
                centerId:
                    data.workstationCenterId
            });

        if (!center) {
            throw new AppError(
                "Workstation center not found",
                404
            );
        }

        if (
            center.status !==
            "ACTIVE"
        ) {
            throw new AppError(
                "Cannot add a workstation to an inactive center",
                400
            );
        }

        const workstationId =
            await generateWorkstationId();

        const workstation =
            await Workstation.create({
                ...data,

                workstationId,

                name:
                    data.name,

                status:
                    data.status ||
                    "ACTIVE",

                workstationCenter:
                    center._id,

                assignedEmployee:
                    null
            });

        return workstation;
    };


/*
 * Safe workstation deletion.
 *
 * A workstation cannot be deleted while:
 *
 * 1. It is assigned to an employee.
 * 2. It is referenced by an active sanitization request.
 *
 * This prevents dangling references.
 */
const deleteWorkstation =
    async (
        workstationId
    ) => {

        const workstation =
            await Workstation.findOne({
                workstationId
            });

        if (!workstation) {
            throw new AppError(
                "Workstation not found",
                404
            );
        }

        if (
            workstation.assignedEmployee
        ) {
            throw new AppError(
                `Workstation ${workstation.workstationId} is assigned to an employee. Unassign the employee before deleting this workstation.`,
                409
            );
        }

        const activeRequest =
            await SanitizationRequest.findOne(
                {
                    assignedWorkstation:
                        workstation._id,

                    status: {
                        $in: [
                            "APPROVED",
                            "ASSIGNED",
                            "IN_PROGRESS",
                            "VERIFYING"
                        ]
                    }
                }
            );

        if (activeRequest) {
            throw new AppError(
                `Workstation ${workstation.workstationId} is referenced by active sanitization request ${activeRequest.requestId} and cannot be deleted.`,
                409
            );
        }

        await workstation.deleteOne();

        return {
            workstationId:
                workstation.workstationId,

            deleted:
                true
        };
    };


/*
 * Explicit workstation unassignment.
 *
 * This is safer than directly changing the database
 * because it removes the employee binding in one place.
 */
const unassignEmployee =
    async (
        workstationId
    ) => {

        const workstation =
            await Workstation.findOne({
                workstationId
            });

        if (!workstation) {
            throw new AppError(
                "Workstation not found",
                404
            );
        }

        if (
            !workstation.assignedEmployee
        ) {
            return workstation;
        }

        const activeRequest =
            await SanitizationRequest.findOne(
                {
                    assignedWorkstation:
                        workstation._id,

                    status: {
                        $in: [
                            "APPROVED",
                            "ASSIGNED",
                            "IN_PROGRESS",
                            "VERIFYING"
                        ]
                    }
                }
            );

        if (activeRequest) {
            throw new AppError(
                `Workstation ${workstation.workstationId} cannot be unassigned while sanitization request ${activeRequest.requestId} is active.`,
                409
            );
        }

        workstation.assignedEmployee =
            null;

        await workstation.save();

        return workstation;
    };


module.exports = {
    createWorkstation,

    deleteWorkstation,

    unassignEmployee
};