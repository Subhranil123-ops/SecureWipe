// const { randomUUID } = require("crypto");
const SanitizationRequest = require("../models/SanitizationRequest");
const WorkstationCenter = require("../models/WorkstationCenter");
const AppError = require("../utils/AppError");
const Counter = require("../models/Counter");


const generateRequestId = async () => {

    const counter =
        await Counter.findOneAndUpdate(
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

    return `REQ-${String(
        counter.sequence
    ).padStart(4, "0")}`;
};

const createSanitizationRequest =
    async (data, user) => {

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


        // --------------------------------------------------
        // VERIFY WORKSTATION CENTER
        // --------------------------------------------------

        const center =
            await WorkstationCenter.findOne({
                centerId:
                    data.workstationCenter
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


        // --------------------------------------------------
        // CREATE SANITIZATION REQUEST
        // --------------------------------------------------

        const requestId = await generateRequestId();

        const request =
            await SanitizationRequest.create({

                requestId,

                customer:
                    user._id,

                workstationCenter:
                    center._id,

                name:
                    data.name,

                email:
                    data.email,

                phone:
                    data.phone,

                deviceType:
                    data.deviceType,

                capacity:
                    data.capacity,

                deviceCount:
                    data.deviceCount,

                assetIdentifier:
                    data.assetIdentifier || "",

                sanitizationMethod:
                    data.sanitizationMethod,

                additionalRequirements:
                    data.additionalRequirements ||
                    "",

                preferredDate:
                    data.preferredDate || null,

                notes:
                    data.notes || "",

                consent:
                    data.consent,

                status:
                    "PENDING",

                history: [
                    {
                        status:
                            "PENDING",

                        changedBy:
                            user._id,

                        changedAt:
                            new Date(),

                        note:
                            "Sanitization request created"
                    }
                ]
            });

        return request;
    };


const getAllSanitizationRequests =
    async () => {

        const requests =
            await SanitizationRequest
                .find()
                .populate(
                    "customer",
                    "name email role"
                )
                .populate({
                    path: "workstationCenter",
                    select: "centerId name location status head",
                    populate: {
                        path: "head",
                        select: "name email phone"
                    }
                })
                .sort({
                    createdAt: -1,
                });

        return requests;
    };

const getMySanitizationRequests =
    async (user) => {

        // --------------------------------------------------
        // AUTHENTICATION CHECK
        // --------------------------------------------------

        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }


        // --------------------------------------------------
        // ROLE CHECK
        // --------------------------------------------------

        if (user.role !== "CUSTOMER") {
            throw new AppError(
                "Only customers can view their sanitization requests",
                403
            );
        }


        // --------------------------------------------------
        // FETCH ONLY LOGGED-IN CUSTOMER'S REQUESTS
        // --------------------------------------------------

        const requests =
            await SanitizationRequest
                .find({
                    customer: user._id
                })
                .populate(
                    "workstationCenter",
                    "centerId name location status"
                )
                .sort({
                    createdAt: -1
                });

        return requests;
    };

const getHeadSanitizationRequests =
    async (user) => {

        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        if (
            user.role !== "WORKSTATION_HEAD"
        ) {
            throw new AppError(
                "Only workstation heads can view center requests",
                403
            );
        }


        const center =
            await WorkstationCenter.findOne({
                head: user._id
            });

        if (!center) {
            throw new AppError(
                "Workstation head is not assigned to a workstation center",
                400
            );
        }



        console.log(
            "Logged-in user ID:",
            user._id
        );

        console.log(
            "Logged-in user role:",
            user.role
        );

        console.log(
            "Found center ID:",
            center._id
        );

        console.log(
            "Center head ID:",
            center.head
        );

        console.log(
            "========================================\n"
        );



        console.log(
            "Searching workstationCenter:",
            center._id
        );

        console.log(
            "Searching status:",
            "PENDING"
        );

        console.log(
            "=========================================\n"
        );


        const requests =
            await SanitizationRequest
                .find({
                    workstationCenter:
                        center._id,

                    status:
                        "PENDING"
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
                    createdAt: -1
                });


        return requests;
    };



// ==================================================
// ADMIN — APPROVE / REJECT SANITIZATION REQUEST
// ==================================================

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

    const { status, reason } = data;

    if (!["APPROVED", "REJECTED"].includes(status)) {
        throw new AppError(
            "Status must be APPROVED or REJECTED",
            400
        );
    }

    if (status === "REJECTED" && !reason?.trim()) {
        throw new AppError(
            "Rejection reason is required",
            400
        );
    }

    const request = await SanitizationRequest.findOne({
        requestId
    });

    if (!request) {
        throw new AppError(
            "Sanitization request not found",
            404
        );
    }

    /*
     * A request can only be reviewed while it is PENDING.
     */
    if (request.status !== "PENDING") {
        throw new AppError(
            `Request cannot be reviewed because its current status is ${request.status}`,
            400
        );
    }

    /*
     * Find the workstation center controlled
     * by the currently logged-in Workstation Head.
     */
    const center = await WorkstationCenter.findOne({
        head: user._id
    });

    if (!center) {
        throw new AppError(
            "No workstation center is assigned to this Workstation Head",
            403
        );
    }

    /*
     * Security check:
     *
     * The Workstation Head can only review requests
     * belonging to their own workstation center.
     */
    if (
        request.workstationCenter.toString() !==
        center._id.toString()
    ) {
        throw new AppError(
            "You are not authorized to review this request",
            403
        );
    }

    /*
     * Update request status.
     */
    request.status = status;

    request.reviewedBy = user._id;
    request.reviewedAt = new Date();

    if (status === "REJECTED") {
        request.rejectionReason = reason.trim();
    } else {
        request.rejectionReason = "";
    }

    /*
     * Add lifecycle history.
     */
    request.history.push({
        status,
        changedBy: user._id,
        changedAt: new Date(),
        note:
            status === "REJECTED"
                ? reason.trim()
                : "Request approved by Workstation Head"
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

        if (
            user.role !==
            "WORKSTATION_HEAD"
        ) {
            throw new AppError(
                "Only workstation heads can view center requests",
                403
            );
        }

        const center =
            await WorkstationCenter.findOne({
                head: user._id
            });

        if (!center) {
            throw new AppError(
                "Workstation head is not assigned to a workstation center",
                400
            );
        }

        const requests =
            await SanitizationRequest
                .find({
                    workstationCenter: center._id,
                    status: "APPROVED"
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
                    createdAt: -1
                });

        return requests;
    };


const getAllHeadSanitizationRequests =
    async (user) => {

        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        if (
            user.role !==
            "WORKSTATION_HEAD"
        ) {
            throw new AppError(
                "Only workstation heads can view center requests",
                403
            );
        }

        const center =
            await WorkstationCenter.findOne({
                head: user._id
            });

        if (!center) {
            throw new AppError(
                "Workstation head is not assigned to a workstation center",
                400
            );
        }

        const requests =
            await SanitizationRequest
                .find({
                    workstationCenter:
                        center._id
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
                    createdAt: -1
                });

        return requests;
    };

const assignSanitizationRequest =
    async (
        requestId,
        data,
        user
    ) => {

        // --------------------------------------------------
        // AUTHENTICATION
        // --------------------------------------------------

        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        // --------------------------------------------------
        // ROLE CHECK
        // --------------------------------------------------

        if (
            user.role !==
            "WORKSTATION_HEAD"
        ) {
            throw new AppError(
                "Only workstation heads can assign sanitization requests",
                403
            );
        }

        // --------------------------------------------------
        // VALIDATE REQUEST ID
        // --------------------------------------------------

        if (!requestId) {
            throw new AppError(
                "Request ID is required",
                400
            );
        }

        // --------------------------------------------------
        // VALIDATE ASSIGNMENT DATA
        // --------------------------------------------------

        const assignedEmployeeId =
            data?.assignedEmployeeId;

        const assignedWorkstationId =
            data?.assignedWorkstationId;

        if (!assignedEmployeeId) {
            throw new AppError(
                "Assigned employee is required",
                400
            );
        }

        if (!assignedWorkstationId) {
            throw new AppError(
                "Assigned workstation is required",
                400
            );
        }

        // --------------------------------------------------
        // FIND HEAD'S CENTER
        // --------------------------------------------------

        const center =
            await WorkstationCenter.findOne({
                head: user._id
            });

        if (!center) {
            throw new AppError(
                "Workstation head is not assigned to a workstation center",
                400
            );
        }

        // --------------------------------------------------
        // FIND REQUEST
        // --------------------------------------------------

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

        // --------------------------------------------------
        // REQUEST MUST BELONG TO THIS CENTER
        // --------------------------------------------------

        if (
            request.workstationCenter.toString() !==
            center._id.toString()
        ) {
            throw new AppError(
                "You can only assign requests belonging to your own center",
                403
            );
        }

        // --------------------------------------------------
        // ONLY APPROVED REQUESTS CAN BE ASSIGNED
        // --------------------------------------------------

        if (
            request.status !==
            "APPROVED"
        ) {
            throw new AppError(
                `Request cannot be assigned because its current status is ${request.status}`,
                400
            );
        }

        // --------------------------------------------------
        // FIND EMPLOYEE
        // --------------------------------------------------

        const User =
            require("../models/User");

        const employee =
            await User.findById(
                assignedEmployeeId
            );

        if (!employee) {
            throw new AppError(
                "Assigned employee not found",
                404
            );
        }

        // --------------------------------------------------
        // EMPLOYEE MUST BE WORKSTATION EMPLOYEE
        // --------------------------------------------------

        if (
            employee.role !==
            "WORKSTATION_EMPLOYEE"
        ) {
            throw new AppError(
                "Selected user is not a workstation employee",
                400
            );
        }

        // --------------------------------------------------
        // EMPLOYEE MUST BE ACTIVE
        // --------------------------------------------------

        if (
            employee.status !==
            "ACTIVE"
        ) {
            throw new AppError(
                "Selected employee is not active",
                400
            );
        }

        // --------------------------------------------------
        // EMPLOYEE MUST BELONG TO THIS CENTER
        // --------------------------------------------------

        if (
            !employee.workstationCenter ||
            employee.workstationCenter.toString() !==
            center._id.toString()
        ) {
            throw new AppError(
                "Selected employee does not belong to your workstation center",
                400
            );
        }

        // --------------------------------------------------
        // FIND WORKSTATION
        // --------------------------------------------------

        const Workstation =
            require("../models/WorkStation");

        const workstation =
            await Workstation.findOne({
                workstationId:
                    assignedWorkstationId
            });

        if (!workstation) {
            throw new AppError(
                "Assigned workstation not found",
                404
            );
        }

        // --------------------------------------------------
        // WORKSTATION MUST BELONG TO THIS CENTER
        // --------------------------------------------------

        if (
            workstation.workstationCenter.toString() !==
            center._id.toString()
        ) {
            throw new AppError(
                "Selected workstation does not belong to your workstation center",
                403
            );
        }

        // --------------------------------------------------
        // WORKSTATION MUST BE ACTIVE
        // --------------------------------------------------

        if (
            workstation.status !==
            "ACTIVE"
        ) {
            throw new AppError(
                "Selected workstation is not active",
                400
            );
        }

        // --------------------------------------------------
        // WORKSTATION CANNOT BE OWNED BY ANOTHER EMPLOYEE
        // --------------------------------------------------

        if (
            workstation.assignedEmployee &&
            workstation.assignedEmployee.toString() !==
            employee._id.toString()
        ) {
            throw new AppError(
                "Selected workstation is already assigned to another employee",
                409
            );
        }

        // --------------------------------------------------
        // EMPLOYEE CANNOT BE ASSIGNED TO ANOTHER
        // WORKSTATION
        // --------------------------------------------------

        const employeeWorkstation =
            await Workstation.findOne({
                assignedEmployee:
                    employee._id,
                _id: {
                    $ne: workstation._id
                }
            });

        if (employeeWorkstation) {
            throw new AppError(
                "Selected employee is already assigned to another workstation",
                409
            );
        }

        // --------------------------------------------------
        // ASSIGN REQUEST
        // --------------------------------------------------

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

        // --------------------------------------------------
        // ADD HISTORY
        // --------------------------------------------------

        request.history.push({
            status: "ASSIGNED",

            changedBy:
                user._id,

            changedAt:
                new Date(),

            note:
                `Request assigned to workstation ${workstation.workstationId} and employee ${employee.name}`
        });

        // --------------------------------------------------
        // SAVE REQUEST
        // --------------------------------------------------

        await request.save();

        // --------------------------------------------------
        // UPDATE WORKSTATION
        // --------------------------------------------------

        workstation.assignedEmployee =
            employee._id;

        await workstation.save();

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
            "WORKSTATION_HEAD"
        ) {
            throw new AppError(
                "Only workstation heads can access their center",
                403
            );
        }

        const center =
            await WorkstationCenter
                .findOne({
                    head: user._id
                })
                .populate(
                    "head",
                    "name email"
                )
                .populate(
                    "employees",
                    "name email role status workstationCenter"
                );

        if (!center) {
            throw new AppError(
                "Workstation head is not assigned to a workstation center",
                404
            );
        }

        const workstations =
            await Workstation
                .find({
                    workstationCenter:
                        center._id
                })
                .populate(
                    "assignedEmployee",
                    "name email role status"
                )
                .select(
                    "workstationId name status connectionStatus hostname operatingSystem assignedEmployee enrolledAt"
                )
                .sort({
                    name: 1
                });

        return {
            centerId:
                center.centerId,

            name:
                center.name,

            location:
                center.location,

            status:
                center.status,

            employees:
                center.employees,

            workstations
        };
    };

const getEmployeeSanitizationRequests =
    async (user) => {

        // ---------------------------------------------
        // 1. Authentication
        // ---------------------------------------------

        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }


        // ---------------------------------------------
        // 2. Employee role
        // ---------------------------------------------

        if (
            user.role !==
            "WORKSTATION_EMPLOYEE"
        ) {
            throw new AppError(
                "Only workstation employees can access assigned requests",
                403
            );
        }


        // ---------------------------------------------
        // 3. Find requests assigned to this employee
        // ---------------------------------------------

        const requests =
            await SanitizationRequest
                .find({
                    assignedEmployee: user._id
                })
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
                    "workstationId name status connectionStatus hostname operatingSystem"
                )
                .sort({
                    assignedAt: -1
                });


        return requests;
    };

const updateEmployeeSanitizationStatus =
    async (requestId, newStatus, user) => {

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
                requestId
            });

        if (!request) {
            throw new AppError(
                "Sanitization request not found",
                404
            );
        }

        // Employee can only update
        // requests assigned to them.
        if (
            !request.assignedEmployee ||
            request.assignedEmployee.toString() !==
            user._id.toString()
        ) {
            throw new AppError(
                "This request is not assigned to you",
                403
            );
        }

        const allowedTransitions = {
            ASSIGNED: ["IN_PROGRESS"],
            IN_PROGRESS: ["VERIFYING"],
            VERIFYING: ["FAILED"],
        };

        const allowedNextStatuses =
            allowedTransitions[request.status];

        if (
            !allowedNextStatuses ||
            !allowedNextStatuses.includes(newStatus)
        ) {
            throw new AppError(
                `Invalid status transition: ${request.status} → ${newStatus}`,
                400
            );
        }

        request.status = newStatus;

        if (newStatus === "IN_PROGRESS") {
            request.startedAt = new Date();
        }

        if (
            newStatus === "COMPLETED" ||
            newStatus === "FAILED"
        ) {
            request.completedAt = new Date();
        }

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
    getMyWorkstationCenter,
    getAllHeadSanitizationRequests,
    getEmployeeSanitizationRequests,
    updateEmployeeSanitizationStatus,
};