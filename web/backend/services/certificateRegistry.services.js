const SanitizationCertificate =
    require(
        "../models/SanitizationCertificate"
    );

const SanitizationRequest =
    require(
        "../models/SanitizationRequest"
    );

const ForensicCase =
    require(
        "../models/ForensicCase"
    );

const WorkstationCenter =
    require(
        "../models/WorkstationCenter"
    );

const AppError =
    require(
        "../utils/AppError"
    );


const ROLES = [
    "ADMIN",
    "CUSTOMER",
    "WORKSTATION_HEAD",
    "WORKSTATION_EMPLOYEE"
];


const requireValidRole =
    user => {

        if (
            !user?._id ||
            !user?.role
        ) {
            throw new AppError(
                "Authentication required",
                401
            );
        }

        if (
            !ROLES.includes(
                user.role
            )
        ) {
            throw new AppError(
                "Access denied",
                403
            );
        }
    };


const getHeadCenterId =
    async user => {

        if (
            user.role !==
            "WORKSTATION_HEAD"
        ) {
            return null;
        }

        const center =
            await WorkstationCenter
                .findOne({
                    head:
                        user._id,

                    status:
                        "ACTIVE"
                })
                .select(
                    "_id centerId name"
                )
                .lean();

        return (
            center?._id ||
            null
        );
    };


const buildScope =
    async user => {

        requireValidRole(
            user
        );

        /*
         * ADMIN
         */

        if (
            user.role ===
            "ADMIN"
        ) {
            return {
                sanitizationRequestIds:
                    null,

                forensicFilter:
                    {}
            };
        }


        /*
         * CUSTOMER
         */

        if (
            user.role ===
            "CUSTOMER"
        ) {

            const requests =
                await SanitizationRequest
                    .find({
                        customer:
                            user._id
                    })
                    .select(
                        "requestId"
                    )
                    .lean();

            return {
                sanitizationRequestIds:
                    requests.map(
                        request =>
                            request.requestId
                    ),

                forensicFilter: {
                    customer:
                        user._id
                }
            };
        }


        /*
         * WORKSTATION EMPLOYEE
         */

        if (
            user.role ===
            "WORKSTATION_EMPLOYEE"
        ) {

            const requests =
                await SanitizationRequest
                    .find({
                        assignedEmployee:
                            user._id
                    })
                    .select(
                        "requestId"
                    )
                    .lean();

            return {
                sanitizationRequestIds:
                    requests.map(
                        request =>
                            request.requestId
                    ),

                forensicFilter: {
                    assignedEmployee:
                        user._id
                }
            };
        }


        /*
         * WORKSTATION HEAD
         */

        const centerId =
            await getHeadCenterId(
                user
            );

        if (
            !centerId
        ) {
            return {
                sanitizationRequestIds:
                    [],

                forensicFilter: {
                    _id: {
                        $in: []
                    }
                }
            };
        }

        const requests =
            await SanitizationRequest
                .find({
                    workstationCenter:
                        centerId
                })
                .select(
                    "requestId"
                )
                .lean();

        return {
            sanitizationRequestIds:
                requests.map(
                    request =>
                        request.requestId
                ),

            forensicFilter: {
                workstationCenter:
                    centerId
            }
        };
    };


const emptyRequestFilter =
    () => ({
        _id: {
            $in: []
        }
    });


const mapSanitization =
    certificate => ({

        type:
            "SANITIZATION",

        certificateId:
            certificate.certificateId,

        requestId:
            certificate.requestId,

        operationId:
            certificate.operationId,

        workstationId:
            certificate.workstationId,

        deviceId:
            certificate.deviceId,

        model:
            certificate.model,

        serialNumber:
            certificate.serialNumber,

        capacityBytes:
            certificate.capacityBytes,

        interfaceType:
            certificate.interfaceType,

        method:
            certificate.method,

        status:
            certificate.status,

        verificationStatus:
            certificate.verificationStatus,

        verificationPerformed:
            certificate.verificationPerformed,

        verificationPassed:
            certificate.verificationPassed,

        certificateHash:
            certificate.certificateHash,

        integrityVerified:
            certificate.integrityVerified,

        generatedAt:
            certificate.generatedAt,

        legacy:
            false
    });


const mapForensic =
    item => {

        const certificate =
            item.forensicCertificate;


        /*
         * New properly persisted forensic certificate
         */

        if (
            certificate?.generated &&
            certificate?.certificateId
        ) {

            return {

                type:
                    "FORENSIC",

                certificateId:
                    certificate.certificateId,

                caseId:
                    item.caseId,

                runId:
                    certificate.runId ||
                    item.forensicIntegrity?.runId ||
                    "",

                workstationId:
                    certificate.workstationId ||
                    "",

                sourceIdentifier:
                    certificate.sourceIdentifier ||
                    item.sourceIdentifier ||
                    "",

                sourceName:
                    certificate.sourceName ||
                    item.sourceName ||
                    "",

                model:
                    certificate.model ||
                    item.deviceType ||
                    "",

                serialNumber:
                    certificate.serialNumber ||
                    item.sourceIdentifier ||
                    "",

                capacityBytes:
                    certificate.capacityBytes ||
                    0,

                interfaceType:
                    certificate.interfaceType ||
                    "",

                method:
                    "FORENSIC_FILE_CARVING",

                status:
                    item.status,

                verificationStatus:
                    item.forensicIntegrity?.verified
                        ? "PASSED"
                        : "NOT_PERFORMED",

                verificationPerformed:
                    Boolean(
                        item.forensicIntegrity?.verifiedAt
                    ),

                verificationPassed:
                    Boolean(
                        item.forensicIntegrity?.verified
                    ),

                certificateHash:
                    certificate.certificateHash ||
                    "",

                integrityVerified:
                    Boolean(
                        certificate.serverVerified
                    ),

                generatedAt:
                    certificate.generatedAt,

                legacy:
                    false
            };
        }


        /*
         * Legacy forensic certificate records.
         *
         * These may exist from the old schema/controller,
         * where only nativeIntegrity was persisted.
         */

        if (
            item.nativeIntegrity?.certificateId
        ) {

            return {

                type:
                    "FORENSIC",

                certificateId:
                    item.nativeIntegrity.certificateId,

                caseId:
                    item.caseId,

                runId:
                    "",

                workstationId:
                    "",

                sourceIdentifier:
                    item.sourceIdentifier ||
                    "",

                sourceName:
                    item.sourceName ||
                    "",

                model:
                    item.deviceType ||
                    "",

                serialNumber:
                    item.sourceIdentifier ||
                    "",

                capacityBytes:
                    0,

                interfaceType:
                    "",

                method:
                    "FORENSIC_FILE_CARVING",

                status:
                    item.status,

                verificationStatus:
                    item.nativeIntegrity.received
                        ? "LEGACY"
                        : "NOT_PERFORMED",

                verificationPerformed:
                    Boolean(
                        item.nativeIntegrity.received
                    ),

                verificationPassed:
                    false,

                certificateHash:
                    item.nativeIntegrity.certificateHash ||
                    "",

                integrityVerified:
                    false,

                generatedAt:
                    item.nativeIntegrity.receivedAt,

                legacy:
                    true
            };
        }

        return null;
    };


const getCertificateRegistry =
    async user => {

        const scope =
            await buildScope(
                user
            );


        /*
         * SANITIZATION CERTIFICATES
         */

        const sanitizationFilter =
            scope.sanitizationRequestIds ===
            null

                ? {}

                : scope.sanitizationRequestIds.length

                    ? {
                        requestId: {
                            $in:
                                scope.sanitizationRequestIds
                        }
                    }

                    : emptyRequestFilter();


        const sanitizationDocuments =
            await SanitizationCertificate
                .find(
                    sanitizationFilter
                )
                .select(
                    [
                        "certificateId",
                        "requestId",
                        "operationId",
                        "workstationId",
                        "deviceId",
                        "model",
                        "serialNumber",
                        "capacityBytes",
                        "interfaceType",
                        "method",
                        "status",
                        "verificationStatus",
                        "verificationPerformed",
                        "verificationPassed",
                        "certificateHash",
                        "integrityVerified",
                        "generatedAt"
                    ].join(" ")
                )
                .sort({
                    generatedAt:
                        -1
                })
                .lean();


        /*
         * FORENSIC CERTIFICATES
         */

        const forensicDocuments =
            await ForensicCase
                .find({

                    ...scope.forensicFilter,

                    $or: [

                        {
                            "forensicCertificate.generated":
                                true
                        },

                        {
                            "forensicCertificate.certificateId":
                            {
                                $exists:
                                    true,

                                $ne:
                                    ""
                            }
                        },

                        {
                            "nativeIntegrity.certificateId":
                            {
                                $exists:
                                    true,

                                $ne:
                                    ""
                            }
                        }

                    ]

                })
                .select(
                    [
                        "caseId",
                        "status",
                        "sourceIdentifier",
                        "sourceName",
                        "deviceType",
                        "forensicCertificate",
                        "forensicIntegrity",
                        "nativeIntegrity",
                        "updatedAt"
                    ].join(" ")
                )
                .sort({
                    updatedAt:
                        -1
                })
                .lean();


        return {

            sanitization:
                sanitizationDocuments.map(
                    mapSanitization
                ),

            forensic:
                forensicDocuments
                    .map(
                        mapForensic
                    )
                    .filter(
                        Boolean
                    )
                    .sort(
                        (
                            a,
                            b
                        ) =>
                            new Date(
                                b.generatedAt ||
                                0
                            ) -
                            new Date(
                                a.generatedAt ||
                                0
                            )
                    )
        };
    };


const getSanitizationCertificate =
    async (
        certificateId,
        user
    ) => {

        const scope =
            await buildScope(
                user
            );

        const certificate =
            await SanitizationCertificate
                .findOne({
                    certificateId
                })
                .lean();

        if (
            !certificate
        ) {
            throw new AppError(
                "Sanitization certificate not found",
                404
            );
        }

        if (
            scope.sanitizationRequestIds !==
                null &&
            !scope.sanitizationRequestIds.includes(
                certificate.requestId
            )
        ) {
            throw new AppError(
                "Access denied",
                403
            );
        }

        return {

            ...certificate,

            certificateType:
                "SANITIZATION"
        };
    };


const getForensicCertificate =
    async (
        caseId,
        user
    ) => {

        const scope =
            await buildScope(
                user
            );

        const item =
            await ForensicCase
                .findOne({
                    caseId
                })
                .populate(
                    "customer",
                    "name email role"
                )
                .populate(
                    "workstationCenter",
                    "centerId name location status"
                )
                .populate(
                    "assignedEmployee",
                    "name email role status"
                )
                .populate(
                    "assignedWorkstation",
                    "workstationId name status connectionStatus hostname operatingSystem"
                )
                .lean();


        if (
            !item
        ) {
            throw new AppError(
                "Forensic case not found",
                404
            );
        }


        if (
            user.role !==
            "ADMIN"
        ) {

            const itemCustomer =
                String(
                    item.customer?._id ||
                    item.customer ||
                    ""
                );

            const itemEmployee =
                String(
                    item.assignedEmployee?._id ||
                    item.assignedEmployee ||
                    ""
                );

            const itemCenter =
                String(
                    item.workstationCenter?._id ||
                    item.workstationCenter ||
                    ""
                );

            const allowed =

                (
                    user.role ===
                    "CUSTOMER" &&

                    itemCustomer ===
                    String(
                        user._id
                    )
                )

                ||

                (
                    user.role ===
                    "WORKSTATION_EMPLOYEE" &&

                    itemEmployee ===
                    String(
                        user._id
                    )
                )

                ||

                (
                    user.role ===
                    "WORKSTATION_HEAD" &&

                    itemCenter ===
                    String(
                        scope
                            .forensicFilter
                            .workstationCenter ||
                        ""
                    )
                );


            if (
                !allowed
            ) {
                throw new AppError(
                    "Access denied",
                    403
                );
            }
        }


        const certificate =
            mapForensic(
                item
            );


        if (
            !certificate
        ) {
            throw new AppError(
                "Forensic certificate not found for this case",
                404
            );
        }


        return {

            certificateType:
                "FORENSIC",

            certificate,

            forensicCertificate:
                item.forensicCertificate ||
                null,

            forensicIntegrity:
                item.forensicIntegrity ||
                null,

            nativeIntegrity:
                item.nativeIntegrity ||
                null,

            case: {

                caseId:
                    item.caseId,

                title:
                    item.title,

                description:
                    item.description,

                status:
                    item.status,

                sourceType:
                    item.sourceType,

                sourceName:
                    item.sourceName,

                sourceIdentifier:
                    item.sourceIdentifier,

                deviceType:
                    item.deviceType,

                capacity:
                    item.capacity,

                customer:
                    item.customer,

                workstationCenter:
                    item.workstationCenter,

                assignedEmployee:
                    item.assignedEmployee,

                assignedWorkstation:
                    item.assignedWorkstation
            },

            artifacts:
                (
                    item.artifacts ||
                    []
                ).map(
                    artifact => ({

                        artifactId:
                            artifact.artifactId,

                        fileName:
                            artifact.fileName,

                        fileType:
                            artifact.fileType,

                        offset:
                            artifact.offset,

                        size:
                            artifact.size,

                        confidenceScore:
                            artifact.confidenceScore,

                        confidenceLevel:
                            artifact.confidenceLevel,

                        confidenceReasons:
                            artifact.confidenceReasons,

                        sha256:
                            artifact.sha256,

                        recovered:
                            artifact.recovered,

                        validated:
                            artifact.validated,

                        headerValid:
                            artifact.headerValid,

                        footerValid:
                            artifact.footerValid,

                        structureValid:
                            artifact.structureValid,

                        sizeValid:
                            artifact.sizeValid,

                        decodable:
                            artifact.decodable,

                        contentAvailable:
                            artifact.contentAvailable,

                        contentMimeType:
                            artifact.contentMimeType,

                        contentLength:
                            artifact.contentLength,

                        contentSha256:
                            artifact.contentSha256,

                        contentUrl:
                            artifact.contentUrl
                    })
                ),

            report:
                item.report ||
                null
        };
    };


module.exports = {
    getCertificateRegistry,
    getSanitizationCertificate,
    getForensicCertificate
};