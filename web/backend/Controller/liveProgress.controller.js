const asyncHandler =
    require("../utils/asyncHandler");


const {
    updateSanitizationProgress,
    getSanitizationProgress,

    updateForensicProgress,
    getForensicProgress
} =
    require(
        "../services/liveProgress.services"
    );


// ======================================================
// SANITIZATION
// ======================================================

module.exports.updateSanitizationProgress =
    asyncHandler(
        async (
            req,
            res
        ) => {

            const result =
                await updateSanitizationProgress(
                    req.params.requestId,
                    req.body,
                    req.user
                );


            res.status(
                200
            ).json({

                success:
                    true,

                message:
                    "Sanitization progress updated successfully",

                data: {
                    requestId:
                        result.requestId,

                    status:
                        result.status,

                    progress:
                        result.progress,

                    progressKnown:
                        result.progressKnown,

                    processedBytes:
                        result.processedBytes,

                    totalBytes:
                        result.totalBytes,

                    remainingBytes:
                        result.remainingBytes,

                    phase:
                        result.phase,

                    message:
                        result.progressMessage,

                    operationId:
                        result.operationId,

                    startedAt:
                        result.startedAt,

                    completedAt:
                        result.completedAt,

                    lastProgressAt:
                        result.lastProgressAt
                }
            });
        }
    );


module.exports.getSanitizationProgress =
    asyncHandler(
        async (
            req,
            res
        ) => {

            const result =
                await getSanitizationProgress(
                    req.params.requestId,
                    req.user
                );


            res.status(
                200
            ).json({

                success:
                    true,

                data:
                    result
            });
        }
    );


// ======================================================
// FORENSICS
// ======================================================

module.exports.updateForensicProgress =
    asyncHandler(
        async (
            req,
            res
        ) => {

            const result =
                await updateForensicProgress(
                    req.params.caseId,
                    req.body,
                    req.user
                );


            res.status(
                200
            ).json({

                success:
                    true,

                message:
                    "Forensic progress updated successfully",

                data: {
                    caseId:
                        result.caseId,

                    status:
                        result.status,

                    progress:
                        result.progress,

                    progressKnown:
                        result.progressKnown,

                    bytesScanned:
                        result.bytesScanned,

                    totalBytes:
                        result.totalBytes,

                    candidatesFound:
                        result.candidatesFound,

                    recoveredArtifacts:
                        result.recoveredArtifacts,

                    validatedArtifacts:
                        result.validatedArtifacts,

                    rejectedArtifacts:
                        result.rejectedArtifacts,

                    highConfidenceArtifacts:
                        result.highConfidenceArtifacts,

                    recoveredBytes:
                        result.recoveredBytes,

                    phase:
                        result.progressPhase ||
                        "",

                    message:
                        result.progressMessage ||
                        "",

                    startedAt:
                        result.startedAt,

                    completedAt:
                        result.completedAt,

                    failedAt:
                        result.failedAt,

                    lastProgressAt:
                        result.lastProgressAt
                }
            });
        }
    );


module.exports.getForensicProgress =
    asyncHandler(
        async (
            req,
            res
        ) => {

            const result =
                await getForensicProgress(
                    req.params.caseId,
                    req.user
                );


            res.status(
                200
            ).json({

                success:
                    true,

                data:
                    result
            });
        }
    );