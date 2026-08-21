const express = require("express");
const router = express.Router();
const userController = require("../Controller/users.controller");
const { Authenticate } = require("../middlewares/auth.middleware");
const { Authorize } = require("../middlewares/authorize.middleware");

router.get("/", Authenticate, Authorize("ADMIN"), userController.getAllUsers);
router.patch("/:id/role", Authenticate, Authorize("ADMIN"), userController.updateUserRole);
router.get("/eligible-center-heads", Authenticate, Authorize("ADMIN"), userController.getEligibleWorkstationHeads);

module.exports = router;