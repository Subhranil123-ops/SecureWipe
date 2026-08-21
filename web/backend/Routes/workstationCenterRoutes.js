const express = require("express");
const router = express.Router();
const workstationController = require("../Controller/workstationCenter.controller");
const { Authenticate } = require("../middlewares/auth.middleware");
const { Authorize } = require("../middlewares/authorize.middleware");

router.post("/", Authenticate, Authorize("ADMIN"), workstationController.createWorkStationCenter);
router.get("/:id", Authenticate,workstationController.getWorkstationCenterById);

module.exports = router;