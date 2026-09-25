const asyncHandler =
    require("../utils/asyncHandler");

const {
    submitAuditChain,
    getAuditChainByRequest,
    verifyStoredAuditChain
} = require(
    "../services/sanitizationAuditChain.services"
);

// --------------------------------------------------
// SUBMIT
// --------------------------------------------------

module.exports.submitAuditChain =
    asyncHandler(
        async (
            req,
            res
        ) => {
            const result =
                await submitAuditChain(
                    req.params.requestId,
                    req.body,
                    req.user
                );

            res.status(201).json({
                success: true,

                message:
                    "Sanitization audit chain verified and stored successfully",

                data:
                    result
            });
        }
    );

// --------------------------------------------------
// GET
// --------------------------------------------------

module.exports.getAuditChain =
    asyncHandler(
        async (
            req,
            res
        ) => {
            const auditChain =
                await getAuditChainByRequest(
                    req.params.requestId,
                    req.user
                );

            res.status(200).json({
                success: true,

                data:
                    auditChain
            });
        }
    );

// --------------------------------------------------
// VERIFY
// --------------------------------------------------

module.exports.verifyAuditChain =
    asyncHandler(
        async (
            req,
            res
        ) => {
            const result =
                await verifyStoredAuditChain(
                    req.params.requestId,
                    req.user
                );

            res.status(200).json({
                success: true,

                data:
                    result
            });
        }
    );