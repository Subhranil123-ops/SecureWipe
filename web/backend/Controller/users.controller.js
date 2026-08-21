const asyncHandler = require("../utils/asyncHandler");

const {
    getEligibleWorkstationHeads,
    getAllUsers,
    updateUserRole,
} = require("../services/user.services");

module.exports.getAllUsers = asyncHandler(async (req, res, next) => {

    const users = await getAllUsers();

    res.status(200).json({
        success: true,
        data: users,
    });

});

module.exports.updateUserRole = asyncHandler(async (req, res, next) => {
    const { id } = req.params;
    const { role } = req.body;

    const user = await updateUserRole(id, role);

    res.status(200).json({
        success: true,
        message: "User role updated successfully",
        data: user
    });
});

module.exports.getEligibleWorkstationHeads = asyncHandler(
    async (req, res, next) => {
        const users = await getEligibleWorkstationHeads();

        res.status(200).json({
            success: true,
            data: users,
        });
    }
);