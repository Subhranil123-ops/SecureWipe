const asyncHandler = require("../utils/asyncHandler");

const {
    submitCertificate,
    getCertificateById,
    verifyCertificateIntegrity,
} = require("../services/sanitizationCertificate.services");

module.exports.submitCertificate = asyncHandler(async (req, res) => {
    const certificate = await submitCertificate(
        req.params.requestId,
        req.body,
        req.user
    );

    res.status(201).json({
        success: true,
        message: "Sanitization certificate stored successfully",
        data: certificate,
    });
});

module.exports.getCertificate = asyncHandler(async (req, res) => {
    const certificate = await getCertificateById(
        req.params.certificateId,
        req.user
    );

    res.status(200).json({
        success: true,
        data: certificate,
    });
});

module.exports.verifyCertificate = asyncHandler(async (req, res) => {
    const result = await verifyCertificateIntegrity(
        req.params.certificateId,
        req.user
    );

    res.status(200).json({
        success: true,
        data: result,
    });
});