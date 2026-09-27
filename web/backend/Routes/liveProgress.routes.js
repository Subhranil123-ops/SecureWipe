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
 * The native execution worker / CLI reports the progress
 * of an already-authorized sanitization request.
 *
 * URL identifier:
 *
 *     SanitizationRequest.requestId
 *
 * The native operation identifier is carried separately
 * inside the JSON payload as "operationId".
 *
 * CUSTOMER is intentionally excluded from writes.
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
 * Website / authenticated users read the current
 * sanitization progress.
 *
 * Object-level authorization is still enforced inside
 * the service.
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
 * The native forensic execution worker reports progress
 * against the assigned forensic case.
 *
 * URL identifier:
 *
 *     ForensicCase.caseId
 *
 * The native execution identifier is carried separately
 * inside the JSON payload as "operationId".
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
 * Website / authenticated users read current forensic
 * progress.
 *
 * Object-level authorization is enforced by the service.
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