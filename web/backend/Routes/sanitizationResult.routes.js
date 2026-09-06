const express = require("express");

const router = express.Router();

const {
    Authenticate
} = require("../middlewares/auth.middleware");

const {
    Authorize
} = require("../middlewares/authorize.middleware");

const controller = require(
    "../Controller/sanitizationResult.controller"
);

router.post(
    "/:requestId",
    Authenticate,
    Authorize("WORKSTATION_EMPLOYEE", "ADMIN"),
    controller.submitResult
);

router.get(
    "/:requestId",
    Authenticate,
    Authorize(
        "CUSTOMER",
        "WORKSTATION_EMPLOYEE",
        "WORKSTATION_HEAD",
        "ADMIN"
    ),
    controller.getResultByRequest
);

module.exports = router;