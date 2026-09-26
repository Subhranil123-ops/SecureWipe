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

        // Actual recovered artifact bytes.
        // Used by the web evidence viewer for the demo.
        contentBase64: {
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

        progress: {
            type: Number,
            min: 0,
            max: 100,
            default: 0
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

module.exports =
    mongoose.model(
        "ForensicCase",
        forensicCaseSchema
    );