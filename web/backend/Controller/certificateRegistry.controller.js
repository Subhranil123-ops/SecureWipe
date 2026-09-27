const asyncHandler =
    require(
        "../utils/asyncHandler"
    );

const service =
    require(
        "../services/certificateRegistry.services"
    );


exports.getCertificates =
    asyncHandler(
        async (
            req,
            res
        ) => {

            res.status(
                200
            ).json({

                success:
                    true,

                data:
                    await service.getCertificateRegistry(
                        req.user
                    )
            });
        }
    );


exports.getSanitizationCertificate =
    asyncHandler(
        async (
            req,
            res
        ) => {

            res.status(
                200
            ).json({

                success:
                    true,

                data:
                    await service.getSanitizationCertificate(
                        req.params.certificateId,
                        req.user
                    )
            });
        }
    );


exports.getForensicCertificate =
    asyncHandler(
        async (
            req,
            res
        ) => {

            res.status(
                200
            ).json({

                success:
                    true,

                data:
                    await service.getForensicCertificate(
                        req.params.caseId,
                        req.user
                    )
            });
        }
    );