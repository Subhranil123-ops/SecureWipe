const AppError = require("../utils/AppError");

const validate = (schema) => {
    return (req, res, next) => {
        const { error, value } = schema.validate(req.body, {
            abortEarly: false,
            stripUnknown: true
        });

        if (error) {
            const message = error.details
                .map((detail) => detail.message)
                .join(", ");

            return next(new AppError(message, 400));
        }

        req.body = value;

        next();
    };
};

module.exports = { validate };