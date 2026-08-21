const asyncHandler = require("../utils/asyncHandler");
const { registerUser, loginUser } = require("../services/auth.services");

module.exports.registerUser = asyncHandler(async (req, res, next) => {
    console.log("hello");
    const user = await registerUser(req.body);

    res.status(201).json({
        success: true,
        message: "User registered successfully",
    });
});

module.exports.loginUser = asyncHandler(async (req, res, next) => {
    console.log("login");
    const { email, password } = req.body;

    const token = await loginUser({
        email,
        password
    });

    res.status(200).json({
        success: true,
        message: "User logged in successfully",
        token
    });
});

module.exports.getMe = (req,res) => {
    res.status(200).json({
        success: true,
        user: req.user
    });
}



