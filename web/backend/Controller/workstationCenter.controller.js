const asyncHandler = require("../utils/asyncHandler");
const { createWorkstationCenter, getWorkstationCenterById } = require("../services/workstationCenter.services");

module.exports.createWorkStationCenter = asyncHandler(async (req, res, next) => {
    const workstationCenter = await createWorkstationCenter(req.body);

    res.status(201).json({
        success: true,
        data: workstationCenter,
    });

});

module.exports.getWorkstationCenterById = asyncHandler(async (req, res, next) => {
    const { id } = req.params;

    const workstationCenter = await getWorkstationCenterById(id, req.user);

    res.status(200).json({
        success: true,
        message: "Workstation center fetched successfully",
        data: workstationCenter
    });

});