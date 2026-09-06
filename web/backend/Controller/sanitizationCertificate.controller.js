const asyncHandler =
    require("../utils/asyncHandler");

const {
    submitSanitizationCertificate,
    getSanitizationCertificate,
    verifySanitizationCertificate,
} = require(
    "../services/sanitizationCertificate.services"
);

module.exports.submitCertificate =
    asyncHandler(
        async (req, res) => {

            const certificate =
                await submitSanitizationCertificate(
                    req.params.requestId,
                    req.body,
                    req.user
                );

            res.status(201).json({
                success: true,
                message:
                    "Sanitization certificate submitted successfully",
                data: certificate,
            });
        }
    );

module.exports.getCertificate =
    asyncHandler(
        async (req, res) => {

            const certificate =
                await getSanitizationCertificate(
                    req.params.certificateId,
                    req.user
                );

            res.status(200).json({
                success: true,
                data: certificate,
            });
        }
    );

module.exports.verifyCertificate =
    asyncHandler(
        async (req, res) => {

            const result =
                await verifySanitizationCertificate(
                    req.params.certificateId,
                    req.user
                );

            res.status(200).json({
                success: true,
                data: result,
            });
        }
    );