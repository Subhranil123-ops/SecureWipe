const mongoose = require("mongoose");

const userSchema = new mongoose.Schema(
    {
        name: {
            type: String,
            required: [true, "Name is required"],
            trim: true,
            minlength: [2, "Name must be at least 2 characters"],
            maxlength: [100, "Name cannot exceed 100 characters"]
        },

        email: {
            type: String,
            required: [true, "Email is required"],
            unique: true,
            lowercase: true,
            trim: true,
            maxlength: [254, "Email is too long"]
        },

        passwordHash: {
            type: String,
            required: true,
            select: false
        },

        role: {
            type: String,
            required: true,
            enum: {
                values: [
                    "ADMIN",
                    "WORKSTATION_HEAD",
                    "WORKSTATION_EMPLOYEE",
                    "CUSTOMER"
                ],
                message: "Invalid user role"
            },
            default: "CUSTOMER"
        },

        status: {
            type: String,
            required: true,
            enum: {
                values: [
                    "ACTIVE",
                    "SUSPENDED",
                    "DISABLED"
                ],
                message: "Invalid account status"
            },
            default: "ACTIVE"
        },

        emailVerified: {
            type: Boolean,
            default: false
        },

        lastLoginAt: {
            type: Date,
            default: null
        }
    },
    {
        timestamps: true
    }
);

const User = mongoose.model("User", userSchema);

module.exports = User;