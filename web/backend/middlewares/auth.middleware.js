const { verifyToken } = require("../utils/jwt.js");
const AppError = require("../utils/AppError");
const User = require("../models/User.js");

const Authenticate = async (req, res, next) => {

    try {
        const authHeader = req.headers.authorization;

        if (!authHeader || !authHeader.startsWith("Bearer ")) return next(new AppError("Authentication required", 401));

        const token = authHeader.split(" ")[1];

        const decoded = verifyToken(token);

        const user = await User.findById(decoded.sub);

        if (!user) {
            return next(new AppError("User no longer exists", 401));
        }

        req.user = user;

        next();

    } catch (error) {
        return next(new AppError("Invalid or expired token", 401));
    }
}

module.exports = {
    Authenticate
}