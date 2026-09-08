const SanitizationRequest = require("../models/SanitizationRequest");
const Workstation = require("../models/WorkStation");
const WorkstationCenter = require("../models/WorkstationCenter");
const AppError = require("../utils/AppError");
const Counter = require("../models/Counter");
const User = require("../models/User");

const generateRequestId = async () => {
    const counter = await Counter.findOneAndUpdate(
        {
            name: "sanitizationRequest",
        },
        {
            $inc: {
                sequence: 1,
            },
        },
        {
            new: true,
            upsert: true,
        }
    );

    return `REQ-${String(counter.sequence).padStart(4, "0")}`;
};

const createSanitizationRequest = async (data, user) => {
    if (!user) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    if (user.role !== "CUSTOMER") {
        throw new AppError(
            "Only customers can create sanitization requests",
            403
        );
    }

    const center = await WorkstationCenter.findOne({
        centerId: data.workstationCenter,
    });

    if (!center) {
        throw new AppError(
            "Workstation center not found",
            404
        );
    }

    if (center.status !== "ACTIVE") {
        throw new AppError(
            "Selected workstation center is not active",
            400
        );
    }

    const requestId = await generateRequestId();

    const request = await SanitizationRequest.create({
        requestId,
        customer: user._id,
        workstationCenter: center._id,
        name: data.name,
        email: data.email,
        phone: data.phone,
        deviceType: data.deviceType,
        capacity: data.capacity,
        deviceCount: data.deviceCount,
        assetIdentifier: data.assetIdentifier || "",
        sanitizationMethod: data.sanitizationMethod,
        additionalRequirements:
            data.additionalRequirements || "",
        preferredDate:
            data.preferredDate || null,
        notes: data.notes || "",
        consent: data.consent,
        status: "PENDING",
        history: [
            {
                status: "PENDING",
                changedBy: user._id,
                changedAt: new Date(),
                note: "Sanitization request created",
            },
        ],
    });

    return request;
};

const getAllSanitizationRequests = async () => {
    return SanitizationRequest.find()
        .populate(
            "customer",
            "name email role"
        )
        .populate({
            path: "workstationCenter",
            select:
                "centerId name location status head",
            populate: {
                path: "head",
                select: "name email phone",
            },
        })
        .populate(
            "assignedEmployee",
            "name email role status"
        )
        .populate(
            "assignedWorkstation",
            "workstationId name status connectionStatus"
        )
        .sort({
            createdAt: -1,
        });
};

const getMySanitizationRequests = async (user) => {
    if (!user) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    if (user.role !== "CUSTOMER") {
        throw new AppError(
            "Only customers can view their sanitization requests",
            403
        );
    }

    return SanitizationRequest.find({
        customer: user._id,
    })
        .populate(
            "workstationCenter",
            "centerId name location status head"
        )
        .populate(
            "assignedEmployee",
            "name email role status"
        )
        .populate(
            "assignedWorkstation",
            "workstationId name status connectionStatus"
        )
        .sort({
            createdAt: -1,
        });
};

const getHeadSanitizationRequests = async (user) => {
    if (!user) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    if (user.role !== "WORKSTATION_HEAD") {
        throw new AppError(
            "Only workstation heads can view center requests",
            403
        );
    }

    const center = await WorkstationCenter.findOne({
        head: user._id,
    });

    if (!center) {
        throw new AppError(
            "Workstation head is not assigned to a workstation center",
            400
        );
    }

    return SanitizationRequest.find({
        workstationCenter: center._id,
        status: "PENDING",
    })
        .populate(
            "customer",
            "name email"
        )
        .populate(
            "workstationCenter",
            "centerId name location status"
        )
        .sort({
            createdAt: -1,
        });
};

const updateSanitizationRequestStatus = async (
    requestId,
    data,
    user
) => {
    if (!user) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    if (user.role !== "WORKSTATION_HEAD") {
        throw new AppError(
            "Only a Workstation Head can review sanitization requests",
            403
        );
    }

    const { status, reason } = data || {};

    if (!["APPROVED", "REJECTED"].includes(status)) {
        throw new AppError(
            "Status must be APPROVED or REJECTED",
            400
        );
    }

    if (
        status === "REJECTED" &&
        !reason?.trim()
    ) {
        throw new AppError(
            "Rejection reason is required",
            400
        );
    }

    const request =
        await SanitizationRequest.findOne({
            requestId,
        });

    if (!request) {
        throw new AppError(
            "Sanitization request not found",
            404
        );
    }

    if (request.status !== "PENDING") {
        throw new AppError(
            `Request cannot be reviewed because its current status is ${request.status}`,
            400
        );
    }

    const center =
        await WorkstationCenter.findOne({
            head: user._id,
        });

    if (!center) {
        throw new AppError(
            "No workstation center is assigned to this Workstation Head",
            403
        );
    }

    if (
        !request.workstationCenter ||
        String(request.workstationCenter) !==
        String(center._id)
    ) {
        throw new AppError(
            "You are not authorized to review this request",
            403
        );
    }

    request.status = status;

    request.reviewedBy = user._id;
    request.reviewedAt = new Date();

    request.rejectionReason =
        status === "REJECTED"
            ? reason.trim()
            : "";

    request.history.push({
        status,
        changedBy: user._id,
        changedAt: new Date(),
        note:
            status === "REJECTED"
                ? reason.trim()
                : "Request approved by Workstation Head",
    });

    await request.save();

    return request;
};

const getHeadApprovedSanitizationRequests =
    async (user) => {
        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        if (user.role !== "WORKSTATION_HEAD") {
            throw new AppError(
                "Only workstation heads can view center requests",
                403
            );
        }

        const center =
            await WorkstationCenter.findOne({
                head: user._id,
            });

        if (!center) {
            throw new AppError(
                "Workstation head is not assigned to a workstation center",
                400
            );
        }

        return SanitizationRequest.find({
            workstationCenter: center._id,
            status: "APPROVED",
        })
            .populate(
                "customer",
                "name email"
            )
            .populate(
                "workstationCenter",
                "centerId name location status"
            )
            .populate(
                "assignedEmployee",
                "name email role status"
            )
            .populate(
                "assignedWorkstation",
                "workstationId name status connectionStatus"
            )
            .sort({
                createdAt: -1,
            });
    };

const getAllHeadSanitizationRequests =
    async (user) => {
        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        if (user.role !== "WORKSTATION_HEAD") {
            throw new AppError(
                "Only workstation heads can view center requests",
                403
            );
        }

        const center =
            await WorkstationCenter.findOne({
                head: user._id,
            });

        if (!center) {
            throw new AppError(
                "Workstation head is not assigned to a workstation center",
                400
            );
        }

        return SanitizationRequest.find({
            workstationCenter: center._id,
        })
            .populate(
                "customer",
                "name email"
            )
            .populate(
                "workstationCenter",
                "centerId name location status"
            )
            .populate(
                "assignedEmployee",
                "name email phone role status"
            )
            .populate(
                "assignedWorkstation",
                "workstationId name status connectionStatus"
            )
            .sort({
                createdAt: -1,
            });
    };

const assignSanitizationRequest = async (
    requestId,
    data,
    user
) => {
    if (!user) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    if (user.role !== "WORKSTATION_HEAD") {
        throw new AppError(
            "Only workstation heads can assign sanitization requests",
            403
        );
    }

    if (!requestId) {
        throw new AppError(
            "Request ID is required",
            400
        );
    }

    const assignedEmployeeId =
        data?.assignedEmployeeId;

    const requestedWorkstationId =
        data?.assignedWorkstationId;

    if (!assignedEmployeeId) {
        throw new AppError(
            "Assigned employee is required",
            400
        );
    }

    const center =
        await WorkstationCenter.findOne({
            head: user._id,
        });

    if (!center) {
        throw new AppError(
            "Workstation head is not assigned to a workstation center",
            400
        );
    }

    const request =
        await SanitizationRequest.findOne({
            requestId,
        });

    if (!request) {
        throw new AppError(
            "Sanitization request not found",
            404
        );
    }

    if (
        !request.workstationCenter ||
        String(request.workstationCenter) !==
        String(center._id)
    ) {
        throw new AppError(
            "You can only assign requests belonging to your own center",
            403
        );
    }

    if (request.status !== "APPROVED") {
        throw new AppError(
            `Request cannot be assigned because its current status is ${request.status}`,
            400
        );
    }

    const employee =
        await User.findOne({
            _id: assignedEmployeeId,
            role: "WORKSTATION_EMPLOYEE",
        });

    if (!employee) {
        throw new AppError(
            "Selected user is not a workstation employee",
            404
        );
    }

    if (employee.status !== "ACTIVE") {
        throw new AppError(
            "Selected employee is not active",
            400
        );
    }

    if (
        !employee.workstationCenter ||
        String(employee.workstationCenter) !==
        String(center._id)
    ) {
        throw new AppError(
            "Selected employee does not belong to your workstation center",
            400
        );
    }

    const employeeWorkstations =
        await Workstation.find({
            assignedEmployee: employee._id,
        }).select(
            "workstationId name workstationCenter status connectionStatus assignedEmployee"
        );

    if (employeeWorkstations.length > 1) {
        throw new AppError(
            `Employee ${employee.name} is linked to multiple workstations. Resolve the workstation data before assigning this request.`,
            409
        );
    }

    const existingEmployeeWorkstation =
        employeeWorkstations[0] || null;

    let workstation =
        existingEmployeeWorkstation;

    let newlyBoundWorkstation = false;

    /*
     * EXISTING EMPLOYEE WORKSTATION
     *
     * The employee must continue using this workstation.
     * A Head cannot move the employee to a different
     * workstation just for one request.
     */
    if (workstation) {
        if (
            !workstation.workstationCenter ||
            String(
                workstation.workstationCenter
            ) !== String(center._id)
        ) {
            throw new AppError(
                `Employee ${employee.name}'s workstation ${workstation.workstationId} does not belong to this center.`,
                409
            );
        }

        if (
            workstation.status !== "ACTIVE"
        ) {
            throw new AppError(
                `Employee ${employee.name}'s workstation ${workstation.workstationId} is not active.`,
                400
            );
        }

        if (
            requestedWorkstationId &&
            requestedWorkstationId !==
            workstation.workstationId
        ) {
            throw new AppError(
                `Employee ${employee.name} is already assigned to workstation ${workstation.workstationId}. This employee cannot be assigned to another workstation.`,
                409
            );
        }
    } else {
        /*
         * NO EXISTING WORKSTATION
         *
         * This is the only case where the Head
         * is allowed to bind a workstation.
         */
        if (!requestedWorkstationId) {
            throw new AppError(
                `Employee ${employee.name} does not have a workstation assigned. Select an unassigned active workstation to bind this employee first.`,
                400
            );
        }

        /*
         * Atomically claim the workstation.
         *
         * This prevents two requests from racing for
         * the same free workstation.
         */
        workstation =
            await Workstation.findOneAndUpdate(
                {
                    workstationId:
                        requestedWorkstationId,

                    workstationCenter:
                        center._id,

                    status: "ACTIVE",

                    $or: [
                        {
                            assignedEmployee:
                                null,
                        },
                        {
                            assignedEmployee: {
                                $exists: false,
                            },
                        },
                    ],
                },
                {
                    $set: {
                        assignedEmployee:
                            employee._id,
                    },
                },
                {
                    new: true,
                }
            );

        if (!workstation) {
            const existingTarget =
                await Workstation.findOne({
                    workstationId:
                        requestedWorkstationId,
                });

            if (!existingTarget) {
                throw new AppError(
                    "Assigned workstation not found",
                    404
                );
            }

            if (
                String(
                    existingTarget.workstationCenter
                ) !== String(center._id)
            ) {
                throw new AppError(
                    "Selected workstation does not belong to your workstation center",
                    403
                );
            }

            if (
                existingTarget.status !==
                "ACTIVE"
            ) {
                throw new AppError(
                    `Selected workstation ${existingTarget.workstationId} is not active`,
                    400
                );
            }

            if (
                existingTarget.assignedEmployee
            ) {
                throw new AppError(
                    `Selected workstation ${existingTarget.workstationId} is already assigned to another employee`,
                    409
                );
            }

            throw new AppError(
                "The workstation became unavailable while the request was being assigned. Refresh the page and try again.",
                409
            );
        }

        newlyBoundWorkstation = true;
    }
    /*
 * Final workstation consistency check.
 */
    if (
        !workstation ||
        String(
            workstation.workstationCenter
        ) !== String(center._id)
    ) {
        if (
            newlyBoundWorkstation &&
            workstation
        ) {
            await Workstation.updateOne(
                {
                    _id: workstation._id,
                    assignedEmployee:
                        employee._id,
                },
                {
                    $set: {
                        assignedEmployee: null,
                    },
                }
            );
        }

        throw new AppError(
            "Workstation assignment is inconsistent with the selected center",
            409
        );
    }

    if (
        workstation.status !== "ACTIVE"
    ) {
        if (
            newlyBoundWorkstation
        ) {
            await Workstation.updateOne(
                {
                    _id: workstation._id,
                    assignedEmployee:
                        employee._id,
                },
                {
                    $set: {
                        assignedEmployee: null,
                    },
                }
            );
        }

        throw new AppError(
            `Workstation ${workstation.workstationId} is not active`,
            400
        );
    }

    /*
     * Assign request only after the workstation
     * validation/claim has succeeded.
     */
    try {
        request.assignedCenter =
            center._id;

        request.assignedEmployee =
            employee._id;

        request.assignedWorkstation =
            workstation._id;

        request.assignedAt =
            new Date();

        request.status =
            "ASSIGNED";

        request.history.push({
            status: "ASSIGNED",
            changedBy: user._id,
            changedAt: new Date(),
            note:
                `Request assigned to employee ${employee.name} (${employee.email}) on workstation ${workstation.workstationId} (${workstation.name})`,
        });

        await request.save();
    } catch (error) {
        if (
            newlyBoundWorkstation &&
            workstation
        ) {
            await Workstation.updateOne(
                {
                    _id: workstation._id,
                    assignedEmployee:
                        employee._id,
                },
                {
                    $set: {
                        assignedEmployee: null,
                    },
                }
            );
        }

        throw error;
    }

    return request;
};

const getMyWorkstationCenter =
    async (user) => {
        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        if (
            user.role !==
            "WORKSTATION_EMPLOYEE"
        ) {
            throw new AppError(
                "Only workstation employees can access their assigned workstation center",
                403
            );
        }

        const employee =
            await User.findById(
                user._id
            ).populate(
                "workstationCenter",
                "centerId name location status head"
            );

        if (
            !employee ||
            !employee.workstationCenter
        ) {
            throw new AppError(
                "Employee is not assigned to a workstation center",
                404
            );
        }

        const workstations =
            await Workstation.find({
                workstationCenter:
                    employee.workstationCenter
                        ._id,
                assignedEmployee:
                    employee._id,
            })
                .populate(
                    "assignedEmployee",
                    "name email role status"
                )
                .select(
                    "workstationId name status connectionStatus hostname operatingSystem assignedEmployee enrolledAt"
                );

        return {
            centerId:
                employee.workstationCenter.centerId,
            name:
                employee.workstationCenter.name,
            location:
                employee.workstationCenter.location,
            status:
                employee.workstationCenter.status,
            workstations,
        };
    };

const getEmployeeSanitizationRequests =
    async (user) => {
        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        if (
            user.role !==
            "WORKSTATION_EMPLOYEE"
        ) {
            throw new AppError(
                "Only workstation employees can access assigned requests",
                403
            );
        }

        return SanitizationRequest.find({
            assignedEmployee: user._id,
        })
            .populate(
                "workstationCenter",
                "centerId name location status head"
            )
            .populate(
                "assignedEmployee",
                "name email role status"
            )
            .populate(
                "assignedWorkstation",
                "workstationId name status connectionStatus hostname operatingSystem"
            )
            .sort({
                assignedAt: -1,
            });
    };

const updateEmployeeSanitizationStatus =
    async (
        requestId,
        newStatus,
        user
    ) => {
        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        if (
            user.role !==
            "WORKSTATION_EMPLOYEE"
        ) {
            throw new AppError(
                "Only workstation employees can update request status",
                403
            );
        }

        const request =
            await SanitizationRequest.findOne({
                requestId,
            });

        if (!request) {
            throw new AppError(
                "Sanitization request not found",
                404
            );
        }

        if (
            !request.assignedEmployee ||
            String(
                request.assignedEmployee
            ) !==
            String(user._id)
        ) {
            throw new AppError(
                "This request is not assigned to you",
                403
            );
        }

        if (!request.assignedWorkstation) {
            throw new AppError(
                "This request has no assigned workstation",
                409
            );
        }

        const workstation =
            await Workstation.findById(
                request.assignedWorkstation
            );

        if (!workstation) {
            throw new AppError(
                "The workstation assigned to this request no longer exists",
                409
            );
        }

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

        if (
            String(workstation.workstationCenter) !==
            String(request.workstationCenter)
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

        const allowedTransitions = {
            ASSIGNED: [
                "IN_PROGRESS",
            ],
            IN_PROGRESS: [
                "VERIFYING",
                "FAILED",
            ],
            VERIFYING: [
                "COMPLETED",
                "FAILED",
            ],
        };

        const allowedNextStatuses =
            allowedTransitions[
            request.status
            ];

        if (
            !allowedNextStatuses ||
            !allowedNextStatuses.includes(
                newStatus
            )
        ) {
            throw new AppError(
                `Invalid status transition: ${request.status} → ${newStatus}`,
                400
            );
        }

        request.status =
            newStatus;

        if (
            newStatus ===
            "IN_PROGRESS"
        ) {
            request.startedAt =
                new Date();
        }

        if (
            newStatus ===
            "COMPLETED" ||
            newStatus === "FAILED"
        ) {
            request.completedAt =
                new Date();
        }

        request.history.push({
            status: newStatus,
            changedBy: user._id,
            changedAt: new Date(),
            note:
                `Request status updated by assigned employee to ${newStatus}`,
        });

        await request.save();

        return request;
    };

module.exports = {
    createSanitizationRequest,
    getAllSanitizationRequests,
    getMySanitizationRequests,
    getHeadSanitizationRequests,
    getHeadApprovedSanitizationRequests,
    updateSanitizationRequestStatus,
    assignSanitizationRequest,
    getAllHeadSanitizationRequests,
    getEmployeeSanitizationRequests,
    updateEmployeeSanitizationStatus,
    getMyWorkstationCenter,
};