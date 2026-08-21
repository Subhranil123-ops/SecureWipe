const Joi = require("joi");

const registerSchema = Joi.object({
    name: Joi.string()
        .trim()
        .min(2)
        .max(50)
        .required()
        .messages({
            "any.required": "Name is required",
            "string.empty": "Name is required",
            "string.min": "Name must be at least 2 characters long",
            "string.max": "Name cannot exceed 50 characters"
        }),

    email: Joi.string()
        .trim()
        .email()
        .required()
        .messages({
            "any.required": "Email is required",
            "string.empty": "Email is required",
            "string.email": "Please enter a valid email address"
        }),


    password: Joi.string()
        .min(8)
        .max(128)
        .required()
        .messages({
            "any.required": "Password is required",
            "string.empty": "Password is required",
            "string.min": "Password must be at least 8 characters long"
        }),
});

const loginSchema = Joi.object({
    email: Joi.string()
        .trim()
        .email()
        .required()
        .messages({
            "any.required": "Email is required",
            "string.empty": "Email is required",
            "string.email": "Please enter a valid email address"
        }),

    password: Joi.string()
        .required()
        .messages({
            "any.required": "Password is required",
            "string.empty": "Password is required",
            "string.min": "Password must be at least 8 characters long"
        }),
});

module.exports = {
    registerSchema,
    loginSchema
};