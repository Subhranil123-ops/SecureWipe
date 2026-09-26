const mongoose = require("mongoose");

const forensicIntegrityLogSchema = new mongoose.Schema(
    {
        caseId: {
            type: String,
            required: true,
            index: true,
            trim: true,
            immutable: true
        },

        runId: {
            type: String,
            required: true,
            index: true,
            trim: true,
            immutable: true
        },

        sequence: {
            type: Number,
            required: true,
            min: 1,
            immutable: true
        },

        eventType: {
            type: String,
            required: true,
            trim: true,
            immutable: true
        },

        artifactId: {
            type: String,
            default: "",
            trim: true,
            immutable: true
        },

        timestampUtc: {
            type: String,
            required: true,
            trim: true,
            immutable: true
        },

        timestamp: {
            type: Date,
            required: true,
            immutable: true,
            index: true
        },

        workstationId: {
            type: String,
            default: "",
            trim: true,
            immutable: true
        },

        sourceIdentifier: {
            type: String,
            default: "",
            trim: true,
            immutable: true
        },

        details: {
            type: String,
            default: "",
            trim: true,
            immutable: true
        },

        previousEventHash: {
            type: String,
            default: "",
            trim: true,
            immutable: true
        },

        eventHash: {
            type: String,
            required: true,
            trim: true,
            unique: true,
            index: true,
            immutable: true
        }
    },
    {
        timestamps: true
    }
);

forensicIntegrityLogSchema.index(
    { caseId: 1, runId: 1, sequence: 1 },
    { unique: true }
);

forensicIntegrityLogSchema.index({
    caseId: 1,
    runId: 1
});

module.exports = mongoose.model(
    "ForensicIntegrityLog",
    forensicIntegrityLogSchema
);