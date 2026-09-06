const mongoose = require("mongoose");

const sanitizationResultSchema = new mongoose.Schema(
    {
        requestId: {
            type: String,
            required: true,
            index: true
        },

        operationId: {
            type: String,
            required: true,
            unique: true,
            index: true
        },

        submittedBy: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "User",
            required: true,
            index: true
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
            default: false
        },

        verificationPassed: {
            type: Boolean,
            default: false
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

        verificationMessage: {
            type: String,
            trim: true,
            default: ""
        },

        nativeErrorCode: {
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
        }
    },
    {
        timestamps: true
    }
);

sanitizationResultSchema.index({
    requestId: 1,
    createdAt: -1
});

module.exports = mongoose.model(
    "SanitizationResult",
    sanitizationResultSchema
);