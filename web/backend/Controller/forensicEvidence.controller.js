const crypto = require("crypto");

const ForensicCase =
    require("../models/ForensicCase");

const AppError =
    require("../utils/AppError");

const service =
    require("../services/forensicCase.services");

const asyncHandler =
    require("../utils/asyncHandler");

const MAX_STORED_ARTIFACT_BYTES =
    5 * 1024 * 1024;

const isStrictBase64 =
    value => {
        if (
            typeof value !==
            "string"
        ) {
            return false;
        }

        const normalized =
            value.replace(
                /\s+/g,
                ""
            );

        if (
            !normalized ||
            normalized.length % 4 !== 0
        ) {
            return false;
        }

        if (
            !/^[A-Za-z0-9+/]*={0,2}$/.test(
                normalized
            )
        ) {
            return false;
        }

        const decoded =
            Buffer.from(
                normalized,
                "base64"
            );

        return (
            decoded.length > 0 &&
            decoded.toString(
                "base64"
            ) === normalized
        );
    };

const verifyArtifactContent =
    (
        artifact,
        index
    ) => {
        const contentBase64 =
            typeof artifact?.contentBase64 ===
            "string"
                ? artifact.contentBase64.replace(
                      /\s+/g,
                      ""
                  )
                : "";

        if (
            !isStrictBase64(
                contentBase64
            )
        ) {
            throw new AppError(
                `Artifact ${index + 1} does not contain valid Base64 evidence content`,
                400
            );
        }

        const bytes =
            Buffer.from(
                contentBase64,
                "base64"
            );

        const declaredSize =
            Number(
                artifact.size
            ) || 0;

        if (
            bytes.length !==
            declaredSize
        ) {
            throw new AppError(
                `Artifact ${index + 1} size does not match its uploaded content`,
                400
            );
        }

        const calculatedHash =
            crypto
                .createHash(
                    "sha256"
                )
                .update(bytes)
                .digest("hex");

        const declaredHash =
            String(
                artifact.sha256 ||
                    ""
            )
                .trim()
                .toLowerCase();

        if (
            declaredHash !==
            calculatedHash
        ) {
            throw new AppError(
                `Artifact ${index + 1} SHA-256 does not match the uploaded bytes`,
                400
            );
        }

        return {
            contentBase64,
            bytesLength:
                bytes.length,
            calculatedHash
        };
    };

const ingestEvidencePackage =
    asyncHandler(
        async (
            req,
            res
        ) => {
            const payload =
                req.body || {};

            if (
                !Array.isArray(
                    payload.artifacts
                ) ||
                payload.artifacts.length ===
                    0
            ) {
                throw new AppError(
                    "At least one recovered artifact is required",
                    400
                );
            }

            if (
                payload.status !==
                    "COMPLETED" &&
                payload.status !==
                    "FAILED"
            ) {
                throw new AppError(
                    "Evidence package status must be COMPLETED or FAILED",
                    400
                );
            }

            let totalBytes = 0;

            const verifiedArtifacts =
                [];

            for (
                let index = 0;
                index <
                payload.artifacts.length;
                ++index
            ) {
                const artifact =
                    payload.artifacts[
                        index
                    ];

                const verified =
                    verifyArtifactContent(
                        artifact,
                        index
                    );

                totalBytes +=
                    verified.bytesLength;

                if (
                    totalBytes >
                    MAX_STORED_ARTIFACT_BYTES
                ) {
                    throw new AppError(
                        "The combined recovered evidence for one case exceeds the 5 MiB web-storage demo limit",
                        413
                    );
                }

                verifiedArtifacts.push(
                    {
                        artifactId:
                            String(
                                artifact.artifactId ||
                                    ""
                            ),

                        contentBase64:
                            verified.contentBase64,

                        calculatedHash:
                            verified.calculatedHash
                    }
                );
            }

            const result =
                await service.ingestResult(
                    req.params.caseId,
                    payload,
                    req.user
                );

            const item =
                await ForensicCase.findOne(
                    {
                        caseId:
                            req.params.caseId
                    }
                );

            if (!item) {
                throw new AppError(
                    "Forensic case was not found after result ingestion",
                    404
                );
            }

            const verifiedById =
                new Map(
                    verifiedArtifacts.map(
                        artifact => [
                            artifact.artifactId,
                            artifact.contentBase64
                        ]
                    )
                );

            item.artifacts =
                item.artifacts.map(
                    artifact => {
                        const contentBase64 =
                            verifiedById.get(
                                String(
                                    artifact.artifactId
                                )
                            );

                        if (
                            contentBase64
                        ) {
                            artifact.contentBase64 =
                                contentBase64;
                        }

                        return artifact;
                    }
                );

            const nativeCertificate =
                payload.certificate &&
                typeof payload.certificate ===
                    "object"
                    ? payload.certificate
                    : {};

            const nativeAudit =
                Array.isArray(
                    payload.nativeAudit
                )
                    ? payload.nativeAudit
                    : [];

            item.nativeIntegrity =
                {
                    received:
                        true,

                    certificateId:
                        String(
                            nativeCertificate.certificateId ||
                                ""
                        ),

                    certificateHash:
                        String(
                            nativeCertificate.certificateHash ||
                                ""
                        ),

                    auditAnchorHash:
                        String(
                            nativeCertificate.auditAnchorHash ||
                                ""
                        ),

                    auditChainHeadHash:
                        String(
                            nativeAudit.length
                                ? nativeAudit[
                                      nativeAudit.length -
                                          1
                                  ].eventHash ||
                                      ""
                                : ""
                        ),

                    auditEventCount:
                        nativeAudit.length,

                    receivedAt:
                        new Date()
                };

            await item.save();

            res.json(
                {
                    success:
                        true,

                    message:
                        "Forensic evidence package accepted and verified",

                    data:
                        {
                            ...result,

                            nativeIntegrity:
                                item.nativeIntegrity
                        }
                }
            );
        }
    );

module.exports = {
    ingestEvidencePackage
};