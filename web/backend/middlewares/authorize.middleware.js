const AppError = require("../utils/AppError");

const Authorize = (...allowedRoles) => {
    return (req, res, next) => {
        if (!allowedRoles.includes(req.user.role)) {
            return next(new AppError("Access denied", 403));
        }

        next();
    };
};

module.exports = { Authorize };