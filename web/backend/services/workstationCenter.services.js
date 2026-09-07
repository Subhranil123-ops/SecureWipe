const mongoose = require("mongoose");
const User = require("../models/User");
const Workstation = require("../models/WorkStation");
const WorkstationCenter = require("../models/WorkstationCenter");
const AppError = require("../utils/AppError");
const Counter = require("../models/Counter");

const generateCenterId = async () => {
    const counter =
        await Counter.findOneAndUpdate(
            {
                name: "workstationCenter",
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

    return `CTR-${String(
        counter.sequence
    ).padStart(4, "0")}`;
};

const createWorkstationCenter = async (
    data
) => {
    const head =
        await User.findById(
            data.head
        );

    if (!head) {
        throw new AppError(
            "Workstation head not found",
            404
        );
    }

    if (
        head.status !==
        "ACTIVE"
    ) {
        throw new AppError(
            "Selected workstation head is inactive",
            400
        );
    }

    if (
        head.role !==
        "WORKSTATION_HEAD"
    ) {
        throw new AppError(
            "Selected user is not a workstation head",
            400
        );
    }

    const existingCenter =
        await WorkstationCenter.findOne({
            head: head._id,
        });

    if (existingCenter) {
        throw new AppError(
            "This workstation head is already assigned to a center",
            409
        );
    }

    const centerId =
        await generateCenterId();

    return WorkstationCenter.create({
        ...data,
        centerId,
    });
};

const getWorkstationCenterById =
    async (
        centerId,
        user
    ) => {
        if (!user) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        const center =
            await WorkstationCenter.findOne({
                centerId,
            })
                .populate(
                    "head",
                    "name email phone role status"
                )
                .populate(
                    "employees",
                    "name email role status workstationCenter"
                );

        if (!center) {
            throw new AppError(
                "Workstation center does not exist",
                404
            );
        }

        const workstations =
            await Workstation.find({
                workstationCenter:
                    center._id,
            })
                .populate(
                    "assignedEmployee",
                    "name email role status"
                )
                .select(
                    "workstationId name status connectionStatus hostname operatingSystem assignedEmployee enrolledAt"
                )
                .sort({
                    name: 1,
                });

        const safeHead =
            center.head
                ? {
                      _id:
                          center.head._id,
                      name:
                          center.head.name,
                      email:
                          center.head.email,
                      phone:
                          center.head.phone,
                      role:
                          center.head.role,
                      status:
                          center.head.status,
                  }
                : null;

        if (
            user.role ===
            "ADMIN"
        ) {
            return {
                centerId:
                    center.centerId,

                name:
                    center.name,

                location:
                    center.location,

                status:
                    center.status,

                head:
                    safeHead,

                employees:
                    center.employees,

                workstations,

                createdAt:
                    center.createdAt,

                updatedAt:
                    center.updatedAt,
            };
        }

        if (
            user.role ===
            "WORKSTATION_HEAD"
        ) {
            if (
                !center.head ||
                String(
                    center.head._id
                ) !==
                    String(user._id)
            ) {
                throw new AppError(
                    "You are not authorized to access this workstation center",
                    403
                );
            }

            return {
                centerId:
                    center.centerId,

                name:
                    center.name,

                location:
                    center.location,

                status:
                    center.status,

                head:
                    safeHead,

                employees:
                    center.employees,

                workstations,
            };
        }

        if (
            user.role ===
            "CUSTOMER"
        ) {
            return {
                centerId:
                    center.centerId,

                name:
                    center.name,

                location:
                    center.location,

                status:
                    center.status,

                head:
                    safeHead
                        ? {
                              name:
                                  safeHead.name,

                              email:
                                  safeHead.email,
                          }
                        : null,
            };
        }

        throw new AppError(
            "You are not authorized to access this workstation center",
            403
        );
    };

const assignEmployees = async (
    centerId,
    employeesIds,
    currentUser
) => {
    if (!currentUser) {
        throw new AppError(
            "Authentication required",
            401
        );
    }

    if (
        !Array.isArray(
            employeesIds
        ) ||
        employeesIds.length === 0
    ) {
        throw new AppError(
            "At least one employee is required",
            400
        );
    }

    const center =
        await WorkstationCenter.findOne({
            centerId,
        });

    if (!center) {
        throw new AppError(
            "Workstation center not found",
            404
        );
    }

    if (
        currentUser.role ===
        "WORKSTATION_HEAD"
    ) {
        if (
            !center.head ||
            String(center.head) !==
                String(
                    currentUser._id
                )
        ) {
            throw new AppError(
                "You can only assign employees to your own center",
                403
            );
        }
    }

    const uniqueEmployeeIds =
        [
            ...new Set(
                employeesIds.map(
                    (id) =>
                        id.toString()
                )
            ),
        ];

    const invalidEmployeeId =
        uniqueEmployeeIds.find(
            (id) =>
                !mongoose.Types.ObjectId.isValid(
                    id
                )
        );

    if (invalidEmployeeId) {
        throw new AppError(
            `Invalid employee ID: ${invalidEmployeeId}`,
            400
        );
    }

    const employeeObjectIds =
        uniqueEmployeeIds.map(
            (id) =>
                new mongoose.Types.ObjectId(
                    id
                )
        );

    const employees =
        await User.find({
            _id: {
                $in:
                    employeeObjectIds,
            },
        });

    if (
        employees.length !==
        employeeObjectIds.length
    ) {
        throw new AppError(
            "One or more employees were not found",
            404
        );
    }

    const invalidEmployee =
        employees.find(
            (employee) =>
                employee.role !==
                "WORKSTATION_EMPLOYEE"
        );

    if (invalidEmployee) {
        throw new AppError(
            "Only workstation employees can be assigned",
            400
        );
    }

    const inactiveEmployee =
        employees.find(
            (employee) =>
                employee.status !==
                "ACTIVE"
        );

    if (inactiveEmployee) {
        throw new AppError(
            "One or more employees are inactive",
            400
        );
    }

    const alreadyAssigned =
        employees.filter(
            (employee) =>
                employee.workstationCenter
        );

    if (
        alreadyAssigned.length > 0
    ) {
        throw new AppError(
            "One or more employees are already assigned to a workstation center",
            400
        );
    }

    center.employees.push(
        ...employeeObjectIds
    );

    await center.save();

    await User.updateMany(
        {
            _id: {
                $in:
                    employeeObjectIds,
            },
        },
        {
            $set: {
                workstationCenter:
                    center._id,
            },
        }
    );

    return getWorkstationCenterById(
        center.centerId,
        currentUser
    );
};

const getActiveWorkstationCenters =
    async () => {
        return WorkstationCenter.find({
            status: "ACTIVE",
        })
            .select(
                "centerId name location status head"
            )
            .populate(
                "head",
                "name email phone role status"
            )
            .sort({
                name: 1,
            });
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
                "Only workstation heads can access this resource",
                403
            );
        }

        const center =
            await WorkstationCenter.findOne({
                head: user._id,
            })
                .populate(
                    "head",
                    "name email phone role status"
                )
                .populate(
                    "employees",
                    "name email role status workstationCenter"
                );

        if (!center) {
            throw new AppError(
                "No workstation center is assigned to this head",
                404
            );
        }

        const workstations =
            await Workstation.find({
                workstationCenter:
                    center._id,
            })
                .populate(
                    "assignedEmployee",
                    "name email role status"
                )
                .select(
                    "workstationId name status connectionStatus hostname operatingSystem assignedEmployee enrolledAt"
                )
                .sort({
                    name: 1,
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

            head:
                center.head
                    ? {
                          _id:
                              center.head._id,
                          name:
                              center.head.name,
                          email:
                              center.head.email,
                          phone:
                              center.head.phone,
                      }
                    : null,

            employees:
                center.employees,

            workstations,
        };
    };

const getEligibleEmployees =
    async (
        centerId,
        currentUser
    ) => {
        if (!currentUser) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        const center =
            await WorkstationCenter.findOne({
                centerId,
            });

        if (!center) {
            throw new AppError(
                "Workstation center not found",
                404
            );
        }

        if (
            currentUser.role ===
            "WORKSTATION_HEAD"
        ) {
            if (
                !center.head ||
                String(center.head) !==
                    String(
                        currentUser._id
                    )
            ) {
                throw new AppError(
                    "You can only access employees from your own center",
                    403
                );
            }
        }

        if (
            currentUser.role !==
                "ADMIN" &&
            currentUser.role !==
                "WORKSTATION_HEAD"
        ) {
            throw new AppError(
                "You are not authorized to view eligible employees",
                403
            );
        }

        return User.find({
            role:
                "WORKSTATION_EMPLOYEE",

            status:
                "ACTIVE",

            workstationCenter:
                null,
        })
            .select(
                "_id name email role status workstationCenter"
            )
            .sort({
                name: 1,
            });
    };

module.exports = {
    createWorkstationCenter,
    getWorkstationCenterById,
    assignEmployees,
    getActiveWorkstationCenters,
    getMyWorkstationCenter,
    getEligibleEmployees,
};