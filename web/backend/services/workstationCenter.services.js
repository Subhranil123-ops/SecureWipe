const User = require("../models/User");
const WorkstationCenter = require("../models/WorkstationCenter");
const AppError = require("../utils/AppError");

const createWorkstationCenter = async (data) => {
    const head = await User.findById(data.head);

    if (!head) throw new AppError("Workstation head not found", 404);

    if (head.status !== "ACTIVE") throw new AppError(
        "Selected workstation head is inactive",
        400
    );

    if (head.role !== "WORKSTATION_HEAD") throw new AppError(
        "Selected user is not a workstation head",
        400
    );

    const existingCenter = await WorkstationCenter.findOne({
        head: head._id,
    });

    if (existingCenter) {
        throw new AppError(
            "This workstation head is already assigned to a center",
            409
        );
    }

    const workstationCenter =
        await WorkstationCenter.create(data);

    return workstationCenter;
}

const getWorkstationCenterById = async (centerId, user) => {

    const center = await WorkstationCenter.findOne({ centerId: centerId }).populate(
        "head",
        "name email"
    );

    if (!center) {
        throw new AppError("Workstation center does not exist", 404);
    }

    if (user.role == "ADMIN") {
        return {
            centerId: center.centerId,
            name: center.name,
            location: center.location,
            status: center.status,
            head: center.head,
            employees: center.employees,
            createdAt: center.createdAt,
            updatedAt: center.updatedAt
        };
    }

    if (user.role == "WORKSTATION_HEAD") {
        if (!center.head._id.equals(user._id)) {
            throw new AppError(
                "You are not authorized to access this workstation center",
                403
            );
        }
        return {
            centerId: center.centerId,
            name: center.name,
            location: center.location,
            status: center.status,
            employees: center.employees
        };

    }

    if (user.role == "CUSTOMER") {
        return {
            centerId: center.centerId,
            name: center.name,
            location: center.location,
            status: center.status,

            head: {
                name: center.head.name
            }
        };
    }

    throw new AppError(
        "You are not authorized to access this workstation center",
        403
    );



}

module.exports = {
    createWorkstationCenter,
    getWorkstationCenterById,
}