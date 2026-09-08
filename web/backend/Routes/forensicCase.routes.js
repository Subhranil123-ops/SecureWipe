const express = require("express");
const router = express.Router();

const { Authenticate } = require("../middlewares/auth.middleware");
const { Authorize } = require("../middlewares/authorize.middleware");
const controller = require("../Controller/forensicCase.controller");
const { validate } = require("../middlewares/validate.middleware");
const { forensicCaseSchema } = require("../forensic.schema");

router.use(Authenticate);

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

router.post(
    "/",
    Authorize("CUSTOMER"),
    validate(forensicCaseSchema),
    controller.createCase
);

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

router.patch(
    "/:caseId/assign",
    Authorize("ADMIN", "WORKSTATION_HEAD"),
    controller.assignCase
);

router.patch(
    "/:caseId/status",
    Authorize(
        "ADMIN",
        "WORKSTATION_HEAD",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.updateStatus
);

router.post(
    "/:caseId/results",
    Authorize(
        "ADMIN",
        "WORKSTATION_EMPLOYEE"
    ),
    controller.ingestResult
);

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

module.exports = router;