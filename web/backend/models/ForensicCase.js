const mongoose = require("mongoose");

const evidenceArtifactSchema = new mongoose.Schema(
    {
        artifactId: {
            type: String,
            required: true,
            trim: true
        },

        fileName: {
            type: String,
            default: "",
            trim: true
        },

        fileType: {
            type: String,
            required: true,
            trim: true
        },

        offset: {
            type: Number,
            required: true,
            min: 0
        },

        size: {
            type: Number,
            required: true,
            min: 0
        },

        recoveredPath: {
            type: String,
            default: "",
            trim: true
        },

        headerValid: {
            type: Boolean,
            default: false
        },

        footerValid: {
            type: Boolean,
            default: false
        },

        structureValid: {
            type: Boolean,
            default: false
        },

        sizeValid: {
            type: Boolean,
            default: false
        },

        decodable: {
            type: Boolean,
            default: false
        },

        confidenceScore: {
            type: Number,
            min: 0,
            max: 100,
            default: 0
        },

        confidenceLevel: {
            type: String,
            enum: [
                "HIGH",
                "MEDIUM",
                "LOW",
                "REJECTED"
            ],
            default: "LOW"
        },

        confidenceReasons: {
            type: [String],
            default: []
        },

        sha256: {
            type: String,
            default: "",
            trim: true
        },

        recovered: {
            type: Boolean,
            default: true
        },

        validated: {
            type: Boolean,
            default: false
        },

        // Backward-compatible field used by the
        // existing forensic evidence viewer/demo.
        contentBase64: {
            type: String,
            default: ""
        },

        /*
         * Server-side GridFS storage metadata.
         *
         * The actual recovered bytes are stored in GridFS.
         * These fields allow the web layer to locate and
         * independently verify the stored bytes.
         */
        contentAvailable: {
            type: Boolean,
            default: false
        },

        contentMimeType: {
            type: String,
            default: ""
        },

        contentLength: {
            type: Number,
            min: 0,
            default: 0
        },

        contentSha256: {
            type: String,
            default: ""
        },

        storageId: {
            type: String,
            default: ""
        },

        contentUrl: {
            type: String,
            default: ""
        },

        createdAt: {
            type: Date,
            default: Date.now
        }
    },
    {
        _id: true
    }
);

const forensicCaseSchema = new mongoose.Schema(
    {
        caseId: {
            type: String,
            unique: true,
            index: true,
            required: true,
            immutable: true
        },

        customer: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "User",
            required: true,
            index: true
        },

        title: {
            type: String,
            required: true,
            trim: true,
            maxlength: 150
        },

        description: {
            type: String,
            trim: true,
            default: "",
            maxlength: 2000
        },

        sourceType: {
            type: String,
            enum: [
                "PHYSICAL_DEVICE",
                "FORENSIC_IMAGE"
            ],
            required: true
        },

        sourceName: {
            type: String,
            required: true,
            trim: true,
            maxlength: 200
        },

        sourceIdentifier: {
            type: String,
            trim: true,
            default: "",
            maxlength: 300
        },

        deviceType: {
            type: String,
            trim: true,
            default: ""
        },

        capacity: {
            type: String,
            trim: true,
            default: ""
        },

        assetIdentifier: {
            type: String,
            trim: true,
            default: ""
        },

        readOnly: {
            type: Boolean,
            default: true
        },

        status: {
            type: String,
            enum: [
                "PENDING",
                "ASSIGNED",
                "ACQUIRING",
                "ANALYZING",
                "COMPLETED",
                "FAILED",
                "CANCELLED"
            ],
            default: "PENDING",
            index: true
        },

        workstationCenter: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "WorkstationCenter",
            default: null,
            index: true
        },

        assignedEmployee: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "User",
            default: null,
            index: true
        },

        assignedWorkstation: {
            type: mongoose.Schema.Types.ObjectId,
            ref: "Workstation",
            default: null
        },

        startedAt: {
            type: Date,
            default: null
        },

        completedAt: {
            type: Date,
            default: null
        },

        failedAt: {
            type: Date,
            default: null
        },

        failureReason: {
            type: String,
            default: "",
            maxlength: 1000
        },

        /*
         * Persistent forensic live-progress state.
         *
         * The native desktop reports these values through
         * LiveProgressReporter. The web layer stores the latest
         * snapshot so dashboards and case details can retrieve it.
         */
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

        progressPhase: {
            type: String,
            default: "",
            trim: true,
            maxlength: 200
        },

        progressMessage: {
            type: String,
            default: "",
            trim: true,
            maxlength: 1000
        },

        operationId: {
            type: String,
            default: "",
            trim: true,
            maxlength: 300,
            index: true
        },

        lastProgressAt: {
            type: Date,
            default: null
        },

        bytesScanned: {
            type: Number,
            min: 0,
            default: 0
        },

        totalBytes: {
            type: Number,
            min: 0,
            default: 0
        },

        candidatesFound: {
            type: Number,
            min: 0,
            default: 0
        },

        recoveredArtifacts: {
            type: Number,
            min: 0,
            default: 0
        },

        validatedArtifacts: {
            type: Number,
            min: 0,
            default: 0
        },

        rejectedArtifacts: {
            type: Number,
            min: 0,
            default: 0
        },

        highConfidenceArtifacts: {
            type: Number,
            min: 0,
            default: 0
        },

        recoveredBytes: {
            type: Number,
            min: 0,
            default: 0
        },

        artifacts: {
            type: [evidenceArtifactSchema],
            default: []
        },

        /*
         * Native forensic certificate and integrity metadata
         * are cryptographically validated by
         * forensicEvidence.services before persistence.
         *
         * Mixed is intentional so the native certificate
         * structure is preserved exactly.
         */
        forensicCertificate: {
            type: mongoose.Schema.Types.Mixed,
            default: null
        },

        forensicIntegrity: {
            type: mongoose.Schema.Types.Mixed,
            default: null
        },

        nativeIntegrity: {
            received: {
                type: Boolean,
                default: false
            },

            certificateId: {
                type: String,
                default: "",
                trim: true
            },

            certificateHash: {
                type: String,
                default: "",
                trim: true
            },

            auditAnchorHash: {
                type: String,
                default: "",
                trim: true
            },

            auditChainHeadHash: {
                type: String,
                default: "",
                trim: true
            },

            auditEventCount: {
                type: Number,
                min: 0,
                default: 0
            },

            receivedAt: {
                type: Date,
                default: null
            }
        },

        report: {
            generated: {
                type: Boolean,
                default: false
            },

            generatedAt: {
                type: Date,
                default: null
            },

            reportHash: {
                type: String,
                default: "",
                trim: true
            }
        },

        history: [
            {
                status: {
                    type: String,
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
                    default: ""
                }
            }
        ]
    },
    {
        timestamps: true
    }
);

forensicCaseSchema.index({
    customer: 1,
    createdAt: -1
});

forensicCaseSchema.index({
    assignedEmployee: 1,
    status: 1
});

forensicCaseSchema.index({
    workstationCenter: 1,
    status: 1
});

forensicCaseSchema.index({
    "forensicCertificate.certificateId": 1
});

forensicCaseSchema.index({
    "forensicIntegrity.runId": 1
});

module.exports =
    mongoose.model(
        "ForensicCase",
        forensicCaseSchema
    );