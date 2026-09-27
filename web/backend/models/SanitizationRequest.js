const mongoose = require("mongoose");

const sanitizationRequestSchema = new mongoose.Schema(
    {
        // --------------------------------------------------
        // REQUEST IDENTIFICATION
        // --------------------------------------------------

        requestId: {
            type: String,
            unique: true,
            index: true,
            required: true
        },

        // --------------------------------------------------
        // CUSTOMER
        // --------------------------------------------------

        customer: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "User",
            required: true,
            index: true
        },

        name: {
            type: String,
            required: true,
            trim: true
        },

        email: {
            type: String,
            required: true,
            trim: true,
            lowercase: true
        },

        phone: {
            type: String,
            required: true,
            trim: true
        },

        // --------------------------------------------------
        // DEVICE INFORMATION
        // --------------------------------------------------

        deviceType: {
            type: String,
            required: true,
            trim: true
        },

        capacity: {
            type: String,
            required: true,
            trim: true
        },

        deviceCount: {
            type: Number,
            required: true,
            min: 1
        },

        serialNumber: {
            type: String,
            required: true,
            trim: true
        },

        assetIdentifier: {
            type: String,
            trim: true,
            default: ""
        },

        // --------------------------------------------------
        // SANITIZATION INFORMATION
        // --------------------------------------------------

        additionalRequirements: {
            type: String,
            trim: true,
            default: ""
        },

        preferredDate: {
            type: Date,
            default: null
        },

        notes: {
            type: String,
            trim: true,
            default: ""
        },

        consent: {
            type: Boolean,
            required: true,
            default: false
        },

        // --------------------------------------------------
        // WORKSTATION CENTER
        // --------------------------------------------------

        workstationCenter: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "WorkstationCenter",
            required: true,
            index: true
        },

        // --------------------------------------------------
        // REQUEST STATUS
        // --------------------------------------------------

        status: {
            type: String,
            enum: [
                "PENDING",
                "APPROVED",
                "REJECTED",
                "ASSIGNED",
                "IN_PROGRESS",
                "VERIFYING",
                "COMPLETED",
                "FAILED",
                "CANCELLED"
            ],
            default: "PENDING",
            required: true,
            index: true
        },

        // --------------------------------------------------
        // REVIEW INFORMATION
        // --------------------------------------------------

        reviewedBy: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "User",
            default: null
        },

        reviewedAt: {
            type: Date,
            default: null
        },

        rejectionReason: {
            type: String,
            trim: true,
            maxlength: 1000,
            default: ""
        },

        // --------------------------------------------------
        // ASSIGNMENT INFORMATION
        // --------------------------------------------------

        assignedCenter: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "WorkstationCenter",
            default: null
        },

        assignedEmployee: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "User",
            default: null
        },

        assignedWorkstation: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "Workstation",
            default: null
        },

        assignedAt: {
            type: Date,
            default: null
        },

        // --------------------------------------------------
        // EXECUTION TIMESTAMPS
        // --------------------------------------------------

        startedAt: {
            type: Date,
            default: null
        },

        completedAt: {
            type: Date,
            default: null
        },

        // --------------------------------------------------
        // LIVE SANITIZATION PROGRESS
        //
        // These fields represent the ACTUAL running
        // sanitization operation reported by the CLI/C++
        // engine.
        //
        // They are deliberately stored on the request
        // itself so the website can read the current state
        // without depending on the desktop GUI.
        // --------------------------------------------------

        progress: {
            type: Number,
            min: 0,
            max: 100,
            default: 0
        },

        progressKnown: {
            type: Boolean,
            default: false
        },

        processedBytes: {
            type: Number,
            min: 0,
            default: 0
        },

        totalBytes: {
            type: Number,
            min: 0,
            default: 0
        },

        remainingBytes: {
            type: Number,
            min: 0,
            default: 0
        },

        phase: {
            type: String,
            trim: true,
            default: ""
        },

        progressMessage: {
            type: String,
            trim: true,
            default: ""
        },

        operationId: {
            type: String,
            trim: true,
            default: ""
        },

        lastProgressAt: {
            type: Date,
            default: null,
            index: true
        },

        // --------------------------------------------------
        // REQUEST HISTORY
        // --------------------------------------------------

        history: [
            {
                status: {
                    type: String,
                    enum: [
                        "PENDING",
                        "APPROVED",
                        "REJECTED",
                        "ASSIGNED",
                        "IN_PROGRESS",
                        "VERIFYING",
                        "COMPLETED",
                        "FAILED",
                        "CANCELLED"
                    ],
                    required: true
                },

                changedBy: {
                    type: mongoose.Schema.Types.ObjectId,
                    ref: "User",
                    required: true
                },

                changedAt: {
                    type: Date,
                    default: Date.now
                },

                note: {
                    type: String,
                    trim: true,
                    maxlength: 1000,
                    default: ""
                }
            }
        ]
    },
    {
        timestamps: true
    }
);


// --------------------------------------------------
// LIVE PROGRESS INDEX
// --------------------------------------------------

sanitizationRequestSchema.index({
    status: 1,
    lastProgressAt: -1
});

sanitizationRequestSchema.index({
    assignedEmployee: 1,
    status: 1,
    lastProgressAt: -1
});

sanitizationRequestSchema.index({
    workstationCenter: 1,
    status: 1,
    lastProgressAt: -1
});


module.exports = mongoose.model(
    "SanitizationRequest",
    sanitizationRequestSchema
);