const mongoose = require("mongoose");
const crypto = require("crypto");

const workstationSchema = new mongoose.Schema(
    {
        workstationId: {
            type: String,
            unique: true,
            index: true,
            immutable: true,
            default: () => crypto.randomUUID()
        },

        name: {
            type: String,
            required: true,
            trim: true,
            maxlength: 100
        },

        assignedEmployee: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "User",
            default: null
        },

        status: {
            type: String,
            enum: ["ACTIVE", "INACTIVE", "MAINTENANCE"],
            default: "ACTIVE"
        },

        connectionStatus: {
            type: String,
            enum: ["ONLINE", "OFFLINE"],
            default: "OFFLINE"
        },

        lastSeen: {
            type: Date,
            default: null
        },

        hostname: {
            type: String,
            trim: true,
            default: null
        },

        operatingSystem: {
            name: {
                type: String,
                trim: true,
                default: null
            },
            version: {
                type: String,
                trim: true,
                default: null
            },
            architecture: {
                type: String,
                trim: true,
                default: null
            }
        },

        hardware: {
            manufacturer: {
                type: String,
                trim: true,
                default: null
            },
            model: {
                type: String,
                trim: true,
                default: null
            },
            serialNumber: {
                type: String,
                trim: true,
                default: null
            }
        },

        enrolledAt: {
            type: Date,
            default: Date.now
        }
    },
    {
        timestamps: true
    }
);

const Workstation = mongoose.model("Workstation", workstationSchema);
module.exports = Workstation;