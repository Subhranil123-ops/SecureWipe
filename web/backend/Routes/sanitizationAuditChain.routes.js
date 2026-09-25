const express =
    require("express");

const router =
    express.Router();

const {
    Authenticate
} = require(
    "../middlewares/auth.middleware"
);

const {
    Authorize
} = require(
    "../middlewares/authorize.middleware"
);

const controller =
    require(
        "../Controller/sanitizationAuditChain.controller"
    );

// --------------------------------------------------
// SUBMIT AUDIT EVIDENCE
// --------------------------------------------------

router.post(
    "/:requestId",
    Authenticate,
    Authorize(
        "WORKSTATION_EMPLOYEE",
        "ADMIN"
    ),
    controller.submitAuditChain
);

// --------------------------------------------------
// GET AUDIT EVIDENCE
// --------------------------------------------------

router.get(
    "/:requestId",
    Authenticate,
    Authorize(
        "CUSTOMER",
        "WORKSTATION_EMPLOYEE",
        "WORKSTATION_HEAD",
        "ADMIN"
    ),
    controller.getAuditChain
);

// --------------------------------------------------
// VERIFY STORED AUDIT EVIDENCE
// --------------------------------------------------

router.get(
    "/:requestId/verify",
    Authenticate,
    Authorize(
        "CUSTOMER",
        "WORKSTATION_EMPLOYEE",
        "WORKSTATION_HEAD",
        "ADMIN"
    ),
    controller.verifyAuditChain
);

module.exports = router;