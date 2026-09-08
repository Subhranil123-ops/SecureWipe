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

const workstationCenterSchema = Joi.object({
    name: Joi.string()
        .trim()
        .min(2)
        .max(100)
        .required()
        .messages({
            "any.required": "Center name is required",
            "string.empty": "Center name is required",
            "string.min": "Center name must be at least 2 characters long",
            "string.max": "Center name cannot exceed 100 characters"
        }),

    head: Joi.string()
        .trim()
        .pattern(/^[0-9a-fA-F]{24}$/)
        .required()
        .messages({
            "any.required": "Workstation head is required",
            "string.empty": "Workstation head is required",
            "string.pattern.base": "Workstation head must be a valid user ID"
        }),

    location: Joi.object({
        address: Joi.string()
            .trim()
            .min(2)
            .max(200)
            .required()
            .messages({
                "any.required": "Address is required",
                "string.empty": "Address is required"
            }),

        city: Joi.string()
            .trim()
            .min(2)
            .max(100)
            .required()
            .messages({
                "any.required": "City is required",
                "string.empty": "City is required"
            }),

        state: Joi.string()
            .trim()
            .min(2)
            .max(100)
            .default("Delhi")
            .required()
            .messages({
                "any.required": "State is required",
                "string.empty": "State is required"
            }),

        postalCode: Joi.string()
            .trim()
            .min(3)
            .max(20)
            .required()
            .messages({
                "any.required": "Postal code is required",
                "string.empty": "Postal code is required"
            }),

        country: Joi.string()
            .trim()
            .min(2)
            .max(100)
            .default("India")
    })
        .required()
        .messages({
            "any.required": "Location is required",
            "object.base": "Location must be an object"
        }),

    status: Joi.string()
        .valid("ACTIVE", "INACTIVE")
        .optional()
        .messages({
            "any.only": "Status must be either ACTIVE or INACTIVE"
        })
});

const workstationSchema = Joi.object({
    name: Joi.string()
        .trim()
        .min(2)
        .max(100)
        .required(),

    status: Joi.string()
        .valid("ACTIVE", "INACTIVE", "MAINTENANCE")
        .optional(),

    workstationCenterId: Joi.string()
        .trim()
        .required(),
});

const sanitizationRequestSchema =
    Joi.object({
        name: Joi.string()
            .trim()
            .min(2)
            .max(100)
            .required(),

        email: Joi.string()
            .trim()
            .email()
            .max(254)
            .required(),

        phone: Joi.string()
            .trim()
            .pattern(
                /^\+?[0-9\s()-]{7,20}$/
            )
            .required(),

        deviceType: Joi.string()
            .valid(
                "SSD",
                "HDD",
                "USB Drive",
                "NVMe SSD",
                "Other"
            )
            .required(),

        capacity: Joi.string()
            .valid(
                "2 GB",
                "4 GB",
                "8 GB",
                "16 GB",
                "32 GB",
                "64 GB",
                "128 GB",
                "256 GB",
                "512 GB",
                "1 TB",
                "2 TB",
                "Other"
            )
            .required(),

        deviceCount: Joi.number()
            .integer()
            .min(1)
            .max(100)
            .required(),

        assetIdentifier: Joi.string()
            .trim()
            .max(100)
            .allow("")
            .optional(),

        serialNumber: Joi.string()
            .trim()
            .max(100)
            .required(),

        additionalRequirements:
            Joi.string()
                .trim()
                .max(1000)
                .allow("")
                .optional(),

        preferredDate:
            Joi.alternatives()
                .try(
                    Joi.date(),
                    Joi.string().valid("")
                )
                .allow(null)
                .optional(),

        notes: Joi.string()
            .trim()
            .max(2000)
            .allow("")
            .optional(),

        consent: Joi.boolean()
            .valid(true)
            .required(),

        workstationCenter: Joi.string()
            .required()
            .messages({
                "any.required": "Workstation center is required",
                "string.empty": "Workstation center is required"
            }),
    });

module.exports = {
    registerSchema,
    loginSchema,
    workstationCenterSchema,
    workstationSchema,
    sanitizationRequestSchema
};