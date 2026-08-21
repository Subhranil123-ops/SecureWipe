const express = require("express");
const router = express.Router();
const authController = require("../Controller/auth.controller");
const { Authenticate } = require("../middlewares/auth.middleware");
const { Authorize } = require("../middlewares/authorize.middleware");
const { validate } = require("../middlewares/validate.middleware");
const { registerSchema, loginSchema } = require("../schema");

router.post("/register", validate(registerSchema), authController.registerUser);
router.post("/login", validate(loginSchema), authController.loginUser);
router.get("/me", Authenticate, authController.getMe)
router.get(
    "/customer-test",
    Authenticate,
    Authorize("CUSTOMER"),
    (req, res) => {
        res.status(200).json({
            success: true,
            message: "You are authorizied as Customer",
            user: req.user
        });
    }
);

router.get("/admin-test", Authenticate, Authorize("ADMIN"), (req, res) => {
    res.status(200).json({
        success: true,
        message: "You are authorizied as Admin",
        user: req.user
    });
})
module.exports = router;