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
        "../Controller/liveProgress.controller"
    );


// ======================================================
// SANITIZATION LIVE PROGRESS
// ======================================================

/*
 * CLI / authorized execution worker updates the
 * currently running sanitization operation.
 *
 * CUSTOMER is intentionally excluded.
 */

router.patch(
    "/sanitization/:requestId",
    Authenticate,
    Authorize(
        "ADMIN",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.updateSanitizationProgress
);


/*
 * Website reads current sanitization progress.
 *
 * All four roles may READ, but the service performs
 * object-level authorization as well.
 */

router.get(
    "/sanitization/:requestId",
    Authenticate,
    Authorize(
        "ADMIN",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE",
        "CUSTOMER"
    ),
    controller.getSanitizationProgress
);


// ======================================================
// FORENSIC LIVE PROGRESS
// ======================================================

/*
 * CLI / authorized execution worker updates the
 * currently running forensic operation.
 */

router.patch(
    "/forensics/:caseId",
    Authenticate,
    Authorize(
        "ADMIN",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.updateForensicProgress
);


/*
 * Website reads current forensic progress.
 */

router.get(
    "/forensics/:caseId",
    Authenticate,
    Authorize(
        "ADMIN",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE",
        "CUSTOMER"
    ),
    controller.getForensicProgress
);


module.exports =
    router;