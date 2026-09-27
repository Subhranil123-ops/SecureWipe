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
        "../Controller/certificateRegistry.controller"
    );


const roles = [
    "CUSTOMER",
    "WORKSTATION_EMPLOYEE",
    "WORKSTATION_HEAD",
    "ADMIN"
];


/*
 * ============================================================
 * ALL CERTIFICATES
 * ============================================================
 */

router.get(
    "/",
    Authenticate,
    Authorize(
        ...roles
    ),
    controller.getCertificates
);


/*
 * ============================================================
 * SANITIZATION CERTIFICATE DETAILS
 * ============================================================
 */

router.get(
    "/sanitization/:certificateId",
    Authenticate,
    Authorize(
        ...roles
    ),
    controller.getSanitizationCertificate
);


/*
 * ============================================================
 * FORENSIC CERTIFICATE DETAILS
 * ============================================================
 */

router.get(
    "/forensic/:caseId",
    Authenticate,
    Authorize(
        ...roles
    ),
    controller.getForensicCertificate
);


module.exports =
    router;