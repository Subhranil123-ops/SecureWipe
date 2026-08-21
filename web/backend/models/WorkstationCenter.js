const mongoose = require("mongoose");
const crypto = require("crypto");
const workstationCenterSchema = new mongoose.Schema({
    centerId: {
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
        unique: true,
        maxlength: 100
    },

    head: {
        type: mongoose.Schema.Types.ObjectId,
        ref: 'User',
        required: true,
    },

    employees: [{
        type: mongoose.Schema.Types.ObjectId,
        ref: 'User',
    }],

    location: {
        address: {
            type: String,
            trim: true,
            default: null
        },

        city: {
            type: String,
            trim: true,
            default: null
        },

        state: {
            type: String,
            trim: true,
            default: null
        },

        postalCode: {
            type: String,
            trim: true,
            default: null
        },

        country: {
            type: String,
            trim: true,
            default: "India"
        }
    },

    status: {
        type: String,
        enum: ["ACTIVE", "INACTIVE"],
        default: "ACTIVE"
    },

}, { timestamps: true })

const WorkstationCenter = mongoose.model("WorkstationCenter", workstationCenterSchema);
module.exports = WorkstationCenter;