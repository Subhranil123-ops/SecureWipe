const mongoose = require("mongoose");

/*
 * One document represents the audit evidence uploaded
 * for one sanitization request.
 *
 * The native C++ application maintains the actual
 * hash-linked JSONL audit ledger. The backend stores
 * the verified ledger so that it can be displayed and
 * verified from the website.
 */

const sanitizationAuditChainSchema = new mongoose.Schema(
    {
        // --------------------------------------------------
        // SANITIZATION REQUEST
        // --------------------------------------------------

        requestId: {
            type: String,
            required: true,
            unique: true,
            index: true,
            trim: true
        },

        // --------------------------------------------------
        // OPERATION
        // --------------------------------------------------

        operationId: {
            type: String,
            required: true,
            index: true,
            trim: true
        },

        // --------------------------------------------------
        // CERTIFICATE
        // --------------------------------------------------

        certificateId: {
            type: String,
            required: true,
            index: true,
            trim: true
        },

        // --------------------------------------------------
        // WORKSTATION
        // --------------------------------------------------

        workstationId: {
            type: String,
            required: true,
            index: true,
            trim: true
        },

        // --------------------------------------------------
        // PERSON WHO UPLOADED THE EVIDENCE
        // --------------------------------------------------

        uploadedBy: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "User",
            required: true,
            index: true
        },

        // --------------------------------------------------
        // AUDIT CHAIN METADATA
        // --------------------------------------------------

        /*
         * For the first event in the persisted audit
         * ledger, previousEventHash can either contain
         * the hash of the immediately preceding event
         * from an earlier operation or be empty when
         * the audit ledger starts there.
         */
        firstEventPreviousHash: {
            type: String,
            default: "",
            trim: true
        },

        /*
         * Hash of the final event in the uploaded
         * audit ledger.
         */
        finalEventHash: {
            type: String,
            default: "",
            trim: true
        },

        // --------------------------------------------------
        // EVENT COUNTS
        // --------------------------------------------------

        eventCount: {
            type: Number,
            required: true,
            min: 0
        },

        verifiedEventCount: {
            type: Number,
            required: true,
            min: 0
        },

        // --------------------------------------------------
        // VERIFICATION STATE
        // --------------------------------------------------

        /*
         * This is set to true only after the backend has
         * independently recalculated the event SHA-256
         * values and verified the previousEventHash
         * links.
         */
        chainValid: {
            type: Boolean,
            required: true,
            default: false
        },

        verificationMessage: {
            type: String,
            trim: true,
            default: ""
        },

        verifiedAt: {
            type: Date,
            required: true
        },

        // --------------------------------------------------
        // REAL AUDIT EVENTS
        // --------------------------------------------------

        /*
         * The native C++ application writes the audit
         * ledger as JSONL:
         *
         * one JSON object per line.
         *
         * We intentionally use Mixed here because the
         * event structure is owned by the C++ sanitization
         * engine and must be preserved without mongoose
         * changing the original event representation.
         */
        events: {
            type: [
                mongoose.Schema.Types.Mixed
            ],
            required: true,
            default: []
        }
    },
    {
        timestamps: true
    }
);

// --------------------------------------------------
// INDEXES
// --------------------------------------------------

sanitizationAuditChainSchema.index({
    requestId: 1,
    operationId: 1
});

sanitizationAuditChainSchema.index({
    certificateId: 1,
    operationId: 1
});

sanitizationAuditChainSchema.index({
    workstationId: 1,
    createdAt: -1
});

module.exports = mongoose.model(
    "SanitizationAuditChain",
    sanitizationAuditChainSchema
);