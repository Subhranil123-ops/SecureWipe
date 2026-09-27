const asyncHandler = require(
    "../utils/asyncHandler"
);

const evidenceService = require(
    "../services/forensicEvidence.services"
);

const ingestEvidencePackage =
    asyncHandler(
        async (req, res) => {
            const result =
                await evidenceService.ingestEvidencePackage(
                    req.params.caseId,
                    req.body,
                    req.user
                );

            res.status(200).json({
                success: true,

                message:
                    "Forensic evidence package accepted and cryptographically verified",

                data:
                    result
            });
        }
    );

const getArtifactContent =
    asyncHandler(
        async (req, res) => {
            await evidenceService.streamArtifactContent(
                req.params.caseId,
                req.params.artifactId,
                req.user,
                res
            );
        }
    );

module.exports = {
    ingestEvidencePackage,
    getArtifactContent
};