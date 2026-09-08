const asyncHandler = require("../utils/asyncHandler");
const service = require("../services/forensicCase.services");

exports.createCase = asyncHandler(async (req, res) => {
    const item = await service.createForensicCase(
        req.body,
        req.user
    );

    res.status(201).json({
        success: true,
        message: "Forensic case created successfully",
        data: item
    });
});

exports.getCases = asyncHandler(async (req, res) => {
    res.json({
        success: true,
        data: await service.getCasesForUser(
            req.user
        )
    });
});

exports.getDashboard = asyncHandler(async (req, res) => {
    res.json({
        success: true,
        data: await service.getDashboard(
            req.user
        )
    });
});

exports.getCase = asyncHandler(async (req, res) => {
    res.json({
        success: true,
        data: await service.getCaseById(
            req.params.caseId,
            req.user
        )
    });
});

exports.getAuditTrail = asyncHandler(async (req, res) => {
    res.json({
        success: true,
        data: await service.getForensicAuditTrail(
            req.params.caseId,
            req.user
        )
    });
});

exports.assignCase = asyncHandler(async (req, res) => {
    res.json({
        success: true,
        message: "Forensic case assigned successfully",
        data: await service.assignCase(
            req.params.caseId,
            req.body,
            req.user
        )
    });
});

exports.updateStatus = asyncHandler(async (req, res) => {
    res.json({
        success: true,
        message: "Forensic case status updated successfully",
        data: await service.updateCaseStatus(
            req.params.caseId,
            req.body.status,
            req.body.note,
            req.user,
            req.body.workstationId || ""
        )
    });
});

exports.ingestResult = asyncHandler(async (req, res) => {
    res.json({
        success: true,
        message: "Forensic result stored successfully",
        data: await service.ingestResult(
            req.params.caseId,
            req.body,
            req.user
        )
    });
});

exports.generateReport = asyncHandler(async (req, res) => {
    res.json({
        success: true,
        data: await service.generateReport(
            req.params.caseId,
            req.user
        )
    });
});