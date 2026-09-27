const express =
    require("express");

const router =
    express.Router();

const {
    Authenticate
} =
    require(
        "../middlewares/auth.middleware"
    );

const {
    Authorize
} =
    require(
        "../middlewares/authorize.middleware"
    );

const controller =
    require(
        "../Controller/forensicCase.controller"
    );

const evidencePackageController =
    require(
        "../Controller/forensicEvidence.controller"
    );

const {
    validate
} =
    require(
        "../middlewares/validate.middleware"
    );

const {
    forensicCaseSchema
} =
    require(
        "../forensic.schema"
    );

router.use(
    Authenticate
);


/*
 * ============================================================
 * FORENSIC DASHBOARD
 * ============================================================
 */

router.get(
    "/dashboard",
    Authorize(
        "ADMIN",
        "CUSTOMER",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.getDashboard
);


/*
 * ============================================================
 * FORENSIC CASE LIST
 * ============================================================
 */

router.get(
    "/",
    Authorize(
        "ADMIN",
        "CUSTOMER",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.getCases
);


/*
 * ============================================================
 * CREATE CASE
 * ============================================================
 */

router.post(
    "/",
    Authorize(
        "CUSTOMER"
    ),
    validate(
        forensicCaseSchema
    ),
    controller.createCase
);


/*
 * ============================================================
 * CASE DETAILS
 * ============================================================
 */

router.get(
    "/:caseId",
    Authorize(
        "ADMIN",
        "CUSTOMER",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.getCase
);


/*
 * ============================================================
 * SERVER FORENSIC AUDIT TRAIL
 * ============================================================
 */

router.get(
    "/:caseId/audit",
    Authorize(
        "ADMIN",
        "CUSTOMER",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.getAuditTrail
);


/*
 * ============================================================
 * NATIVE FORENSIC INTEGRITY
 * ============================================================
 */

router.get(
    "/:caseId/integrity",
    Authorize(
        "ADMIN",
        "CUSTOMER",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.getNativeIntegrity
);


/*
 * ============================================================
 * VERIFY STORED NATIVE FORENSIC INTEGRITY
 * ============================================================
 */

router.post(
    "/:caseId/integrity/verify",
    Authorize(
        "ADMIN",
        "CUSTOMER",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.verifyNativeIntegrity
);


/*
 * ============================================================
 * CASE ASSIGNMENT
 * ============================================================
 */

router.patch(
    "/:caseId/assign",
    Authorize(
        "ADMIN",
        "WORKSTATION_HEAD"
    ),
    controller.assignCase
);


/*
 * ============================================================
 * STATUS
 * ============================================================
 */

router.patch(
    "/:caseId/status",
    Authorize(
        "ADMIN",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.updateStatus
);


/*
 * ============================================================
 * NATIVE EVIDENCE PACKAGE
 * ============================================================
 */

router.post(
    "/:caseId/evidence-package",
    Authorize(
        "ADMIN",
        "WORKSTATION_EMPLOYEE"
    ),
    evidencePackageController
        .ingestEvidencePackage
);


/*
 * ============================================================
 * RECOVERED ARTIFACT CONTENT
 * ============================================================
 */

router.get(
    "/:caseId/artifacts/:artifactId/content",
    Authorize(
        "ADMIN",
        "CUSTOMER",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    evidencePackageController
        .getArtifactContent
);


/*
 * ============================================================
 * LEGACY RESULT INGESTION
 * ============================================================
 */

router.post(
    "/:caseId/results",
    Authorize(
        "ADMIN",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.ingestResult
);


/*
 * ============================================================
 * FORENSIC REPORT
 * ============================================================
 */

router.post(
    "/:caseId/report",
    Authorize(
        "ADMIN",
        "CUSTOMER",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.generateReport
);

module.exports =
    router;