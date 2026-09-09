const asyncHandler = require("../utils/asyncHandler");

const {
    createWorkstation,
    bindWorkstationIdentity
} = require("../services/workstation.services");

module.exports.createWorkstation = asyncHandler(
    async (req, res, next) => {
        const workstation = await createWorkstation(req.body);

        res.status(201).json({
            success: true,
            message: "Workstation created successfully",
            data: workstation
        });
    }
);

module.exports.bindWorkstationIdentity = asyncHandler(
    async (req, res, next) => {
        const workstation = await bindWorkstationIdentity(
            req.body,
            req.user
        );

        res.status(200).json({
            success: true,
            message: "Workstation identity verified successfully",
            data: workstation
        });
    }
);