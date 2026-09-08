const mongoose = require("mongoose");

const forensicAuditLogSchema = new mongoose.Schema({
    auditId: {
        type: String,
        required: true,
        unique: true,
        index: true,
        immutable: true,
        trim: true
    },

    caseId: {
        type: String,
        required: true,
        index: true,
        immutable: true,
        trim: true
    },

    sequence: {
        type: Number,
        required: true,
        min: 1
    },

    action: {
        type: String,
        required: true,
        trim: true
    },

    fromStatus: {
        type: String,
        default: ""
    },

    toStatus: {
        type: String,
        default: ""
    },

    actor: {
        type: mongoose.Schema.Types.ObjectId,
        ref: "User",
        default: null
    },

    actorName: {
        type: String,
        default: "",
        trim: true
    },

    actorRole: {
        type: String,
        default: "",
        trim: true
    },

    workstation: {
        type: mongoose.Schema.Types.ObjectId,
        ref: "Workstation",
        default: null
    },

    workstationId: {
        type: String,
        default: "",
        trim: true
    },

    workstationName: {
        type: String,
        default: "",
        trim: true
    },

    workstationCenter: {
        type: mongoose.Schema.Types.ObjectId,
        ref: "WorkstationCenter",
        default: null
    },

    workstationCenterId: {
        type: String,
        default: "",
        trim: true
    },

    sourceIdentifier: {
        type: String,
        default: "",
        trim: true
    },

    sourceName: {
        type: String,
        default: "",
        trim: true
    },

    note: {
        type: String,
        default: "",
        trim: true,
        maxlength: 2000
    },

    metadata: {
        type: mongoose.Schema.Types.Mixed,
        default: {}
    },

    previousEventHash: {
        type: String,
        default: "",
        trim: true
    },

    eventHash: {
        type: String,
        required: true,
        unique: true,
        index: true,
        trim: true,
        immutable: true
    },

    timestamp: {
        type: Date,
        default: Date.now,
        index: true,
        immutable: true
    }
}, {
    timestamps: true
});

forensicAuditLogSchema.index({ caseId: 1, sequence: 1 }, { unique: true });
forensicAuditLogSchema.index({ caseId: 1, timestamp: 1 });

module.exports = mongoose.model("ForensicAuditLog", forensicAuditLogSchema);