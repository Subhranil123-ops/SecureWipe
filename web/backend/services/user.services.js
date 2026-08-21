const User = require("../models/User");
const asignableRoles = [
    "WORKSTATION_HEAD",
    "WORKSTATION_EMPLOYEE",
    "CUSTOMER"
]
const getAllUsers = async () => {
    const users = await User.find()
        .select("_id name email role status emailVerified createdAt");

    return users;
};

const updateUserRole = async (id, role) => {

    const user = await User.findById(id);

    if (!user) {
        throw new AppError("User not found", 404);
    }

    if (user.role === "ADMIN") {
        throw new AppError(
            "Admin role cannot be changed",
            403
        );
    }

    if (!asignableRoles.includes(role)) {
        throw new AppError(
            "Invalid role assignment", 400
        );
    }

    user.role = role;

    await user.save();

    return user;
}

const getEligibleWorkstationHeads = async () => {
    const users = await User.find({
        role: "WORKSTATION_HEAD"
    }).select("_id name email");

    return users;
};

module.exports = {
    getEligibleWorkstationHeads,
    getAllUsers,
    updateUserRole
};