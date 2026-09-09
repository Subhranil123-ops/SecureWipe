const express = require("express");
const router = express.Router();

const { validate } = require("../middlewares/validate.middleware");
const { Authenticate } = require("../middlewares/auth.middleware");
const { Authorize } = require("../middlewares/authorize.middleware");

const { workstationSchema } = require("../schema");

const workstationController = require("../Controller/workstation.controller");

router.post(
    "/",
    Authenticate,
    Authorize("ADMIN"),
    validate(workstationSchema),
    workstationController.createWorkstation
);

router.post(
    "/identity",
    Authenticate,
    Authorize("WORKSTATION_EMPLOYEE"),
    workstationController.bindWorkstationIdentity
);

module.exports = router;