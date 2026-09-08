const mongoose = require("mongoose");

const sanitizationCertificateSchema = new mongoose.Schema(
    {
        certificateId: {
            type: String,
            required: true,
            unique: true,
            index: true
        },

        operationId: {
            type: String,
            required: true,
            unique: true,
            index: true
        },

        requestId: {
            type: String,
            required: true,
            index: true
        },

        workstationId: {
            type: String,
            required: true,
            trim: true,
            index: true
        },

        result: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "SanitizationResult",
            required: true,
            index: true
        },

        generatedBy: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "User",
            required: true
        },

        deviceId: {
            type: String,
            required: true,
            trim: true
        },

        model: {
            type: String,
            required: true,
            trim: true
        },

        serialNumber: {
            type: String,
            required: true,
            trim: true
        },

        capacityBytes: {
            type: Number,
            required: true,
            min: 1
        },

        interfaceType: {
            type: String,
            required: true,
            trim: true
        },

        method: {
            type: String,
            enum: [
                "HOST_OVERWRITE",
                "ATA_SANITIZE",
                "NVME_SANITIZE",
                "UNSUPPORTED"
            ],
            required: true
        },

        status: {
            type: String,
            enum: [
                "NOT_STARTED",
                "IN_PROGRESS",
                "COMPLETED",
                "FAILED",
                "ABORTED"
            ],
            required: true
        },

        bytesProcessed: {
            type: Number,
            default: 0,
            min: 0
        },

        operationDurationMs: {
            type: Number,
            default: 0,
            min: 0
        },

        verificationStatus: {
            type: String,
            enum: [
                "NOT_PERFORMED",
                "IN_PROGRESS",
                "PASSED",
                "FAILED"
            ],
            required: true
        },

        verificationPerformed: {
            type: Boolean,
            required: true
        },

        verificationPassed: {
            type: Boolean,
            required: true
        },

        bytesVerified: {
            type: Number,
            default: 0,
            min: 0
        },

        verificationSamples: {
            type: Number,
            default: 0,
            min: 0
        },

        deviceReportedSuccess: {
            type: Boolean,
            default: false
        },

        globalDataErased: {
            type: Boolean,
            default: false
        },

        nativeErrorCode: {
            type: Number,
            default: 0,
            min: 0
        },

        verificationMessage: {
            type: String,
            trim: true,
            default: ""
        },

        generatedAt: {
            type: Date,
            required: true
        },

        hashAlgorithm: {
            type: String,
            enum: ["SHA-256"],
            required: true
        },

        certificateHash: {
            type: String,
            required: true,
            index: true,
            match: /^[a-fA-F0-9]{64}$/
        },

        message: {
            type: String,
            trim: true,
            default: ""
        },

        integrityVerified: {
            type: Boolean,
            default: true
        }
    },
    {
        timestamps: true
    }
);

module.exports = mongoose.model(
    "SanitizationCertificate",
    sanitizationCertificateSchema
);