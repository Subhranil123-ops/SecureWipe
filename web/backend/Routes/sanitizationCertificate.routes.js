const express = require("express");

const router = express.Router();

const {
    Authenticate
} = require("../middlewares/auth.middleware");

const {
    Authorize
} = require("../middlewares/authorize.middleware");

const controller =
    require(
        "../Controller/sanitizationCertificate.controller"
    );

router.post(
    "/:requestId",
    Authenticate,
    Authorize(
        "WORKSTATION_EMPLOYEE",
        "ADMIN"
    ),
    controller.submitCertificate
);

router.get(
    "/:certificateId",
    Authenticate,
    Authorize(
        "CUSTOMER",
        "WORKSTATION_EMPLOYEE",
        "WORKSTATION_HEAD",
        "ADMIN"
    ),
    controller.getCertificate
);

router.get(
    "/:certificateId/verify",
    Authenticate,
    Authorize(
        "CUSTOMER",
        "WORKSTATION_EMPLOYEE",
        "WORKSTATION_HEAD",
        "ADMIN"
    ),
    controller.verifyCertificate
);

module.exports = router;