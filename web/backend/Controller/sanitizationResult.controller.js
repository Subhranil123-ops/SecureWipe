const asyncHandler = require("../utils/asyncHandler");

const {
    submitSanitizationResult,
    getSanitizationResultByRequest
} = require("../services/sanitizationResult.services");

exports.submitResult = asyncHandler(async (req, res) => {
    const result = await submitSanitizationResult(
        req.params.requestId,
        req.body,
        req.user
    );

    res.status(201).json({
        success: true,
        message: "Sanitization result stored successfully",
        data: result
    });
});

exports.getResultByRequest = asyncHandler(async (req, res) => {
    const result = await getSanitizationResultByRequest(
        req.params.requestId,
        req.user
    );

    res.status(200).json({
        success: true,
        data: result
    });
});