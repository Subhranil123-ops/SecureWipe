import { useEffect, useMemo, useState } from "react";
import { Link, useNavigate, useParams } from "react-router-dom";
import toast from "react-hot-toast";

import { useAuth } from "../../context/AuthContext";

import {
    assignForensicCase,
    generateForensicReport,
    getForensicAuditTrail,
    getForensicCase,
    updateForensicStatus,
} from "../../services/forensicService";

import { getWorkstationCenter } from "../../services/workstationCenterService";

import ForensicStatusBadge from "../../components/Forensics/ForensicStatusBadge";
import ForensicEvidenceDrawer from "../../components/Forensics/ForensicEvidenceDrawer";
import ForensicModal from "../../components/Forensics/ForensicModal";

function ForensicCaseDetails() {
    const { caseId } = useParams();
    const { user } = useAuth();
    const navigate = useNavigate();

    const [item, setItem] = useState(null);
    const [center, setCenter] = useState(null);
    const [auditTrail, setAuditTrail] = useState([]);
    const [reportPayload, setReportPayload] = useState(null);

    const [loading, setLoading] = useState(true);
    const [auditLoading, setAuditLoading] = useState(false);
    const [actionLoading, setActionLoading] = useState(false);

    const [selectedArtifact, setSelectedArtifact] = useState(null);

    const [assignOpen, setAssignOpen] = useState(false);
    const [employeeId, setEmployeeId] = useState("");
    const [workstationId, setWorkstationId] = useState("");

    const [activeTab, setActiveTab] = useState("overview");

    useEffect(() => {
        loadCase();
    }, [caseId]);

    useEffect(() => {
        if (activeTab === "audit" && caseId) {
            loadAuditTrail();
        }
    }, [activeTab, caseId]);

    const loadCase = async () => {
        try {
            setLoading(true);

            const result = await getForensicCase(caseId);

            setItem(result);

            if (
                result?.workstationCenter?.centerId &&
                ["ADMIN", "WORKSTATION_HEAD"].includes(user?.role)
            ) {
                try {
                    const response = await getWorkstationCenter(
                        result.workstationCenter.centerId
                    );

                    setCenter(response.data || null);
                } catch {
                    setCenter(null);
                }
            }
        } catch (error) {
            toast.error(
                error.message ||
                    "Unable to load forensic case"
            );

            navigate("/forensics/cases");
        } finally {
            setLoading(false);
        }
    };

    const loadAuditTrail = async () => {
        try {
            setAuditLoading(true);

            const result =
                await getForensicAuditTrail(caseId);

            setAuditTrail(
                Array.isArray(result)
                    ? result
                    : []
            );
        } catch (error) {
            toast.error(
                error.message ||
                    "Unable to load forensic audit trail"
            );

            setAuditTrail([]);
        } finally {
            setAuditLoading(false);
        }
    };

    const refreshCaseAndAudit = async () => {
        try {
            const result =
                await getForensicCase(caseId);

            setItem(result);

            if (activeTab === "audit") {
                await loadAuditTrail();
            }
        } catch (error) {
            toast.error(
                error.message ||
                    "Unable to refresh forensic case"
            );
        }
    };

    const getCurrentWorkstationId = () => {
        return (
            item?.assignedWorkstation?._id ||
            item?.assignedWorkstation?.id ||
            ""
        );
    };

    const getCurrentWorkstationCode = () => {
        return (
            item?.assignedWorkstation?.workstationId ||
            ""
        );
    };

    const changeStatus = async (
        status,
        note = ""
    ) => {
        try {
            setActionLoading(true);

            const result =
                await updateForensicStatus(
                    caseId,
                    status,
                    note,
                    getCurrentWorkstationId()
                );

            setItem(result);

            await loadAuditTrail();

            toast.success(
                `Case moved to ${status.toLowerCase()}`
            );
        } catch (error) {
            toast.error(
                error.message ||
                    "Unable to update case"
            );
        } finally {
            setActionLoading(false);
        }
    };

    const assignCase = async () => {
        if (!employeeId) {
            toast.error(
                "Select an employee"
            );
            return;
        }

        if (!workstationId) {
            toast.error(
                "Select a workstation"
            );
            return;
        }

        try {
            setActionLoading(true);

            const result =
                await assignForensicCase(
                    caseId,
                    employeeId,
                    workstationId
                );

            setItem(result);

            setAssignOpen(false);
            setEmployeeId("");
            setWorkstationId("");

            await loadAuditTrail();

            toast.success(
                "Forensic case assigned successfully"
            );
        } catch (error) {
            toast.error(
                error.message ||
                    "Unable to assign case"
            );
        } finally {
            setActionLoading(false);
        }
    };

    const generateReport = async () => {
        try {
            setActionLoading(true);

            const result =
                await generateForensicReport(
                    caseId
                );

            setReportPayload(result?.report || null);

            setItem(previous => ({
                ...previous,
                report: {
                    generated: true,
                    generatedAt:
                        result?.report?.generatedAt ||
                        new Date().toISOString(),
                    reportHash:
                        result?.reportHash ||
                        ""
                }
            }));

            await loadAuditTrail();

            setActiveTab("audit");

            toast.success(
                "Forensic report generated"
            );
        } catch (error) {
            toast.error(
                error.message ||
                    "Unable to generate report"
            );
        } finally {
            setActionLoading(false);
        }
    };

    const artifacts =
        item?.artifacts || [];

    const validationRate = useMemo(() => {
        if (!item?.recoveredArtifacts) {
            return 0;
        }

        return Math.round(
            (
                Number(
                    item.validatedArtifacts ||
                        0
                ) /
                Number(
                    item.recoveredArtifacts
                )
            ) *
                100
        );
    }, [item]);

    const auditStats = useMemo(() => {
        const total =
            auditTrail.length;

        const hashCount =
            auditTrail.filter(
                entry =>
                    Boolean(
                        entry.eventHash
                    )
            ).length;

        const chainedCount =
            auditTrail.filter(
                entry =>
                    Boolean(
                        entry.previousEventHash
                    )
            ).length;

        const validCount =
            auditTrail.filter(
                entry => entry.valid === true
            ).length;

        const invalidCount =
            auditTrail.filter(
                entry => entry.valid === false
            ).length;

        const validChain =
            total > 0 &&
            invalidCount === 0 &&
            auditTrail.every(
                entry =>
                    entry.valid !== false &&
                    entry.hashValid !== false &&
                    entry.chainValid !== false
            );

        return {
            total,
            hashCount,
            chainedCount,
            validCount,
            invalidCount,
            validChain
        };
    }, [auditTrail]);

    if (loading) {
        return (
            <div className="p-8 text-center text-sm text-slate-500">
                Loading forensic case...
            </div>
        );
    }

    if (!item) {
        return null;
    }

    const canAssign =
        ["ADMIN", "WORKSTATION_HEAD"].includes(
            user?.role
        ) &&
        ["PENDING", "ASSIGNED"].includes(
            item.status
        );

    const canOperate =
        [
            "ADMIN",
            "WORKSTATION_HEAD",
            "WORKSTATION_EMPLOYEE"
        ].includes(user?.role);

    const tabs = [
        {
            id: "overview",
            label: "Overview"
        },
        {
            id: "evidence",
            label: `Evidence (${artifacts.length})`
        },
        {
            id: "timeline",
            label: "Timeline"
        },
        {
            id: "audit",
            label: `Audit (${auditTrail.length})`
        },
        {
            id: "report",
            label: "Report"
        }
    ];

    return (
        <div className="space-y-6">
            <div className="flex flex-col gap-4 lg:flex-row lg:items-start lg:justify-between">
                <div>
                    <div className="flex flex-wrap items-center gap-2">
                        <Link
                            to="/forensics/cases"
                            className="text-sm font-medium text-slate-500 hover:text-indigo-600"
                        >
                            ← Cases
                        </Link>

                        <span className="text-slate-300">
                            /
                        </span>

                        <span className="font-mono text-xs font-semibold text-indigo-600">
                            {item.caseId}
                        </span>

                        <ForensicStatusBadge
                            status={
                                item.status
                            }
                        />
                    </div>

                    <h1 className="mt-3 text-2xl font-semibold text-slate-900">
                        {item.title}
                    </h1>

                    <p className="mt-1 max-w-3xl text-sm text-slate-500">
                        {item.description ||
                            "No case description provided."}
                    </p>
                </div>

                <div className="flex flex-wrap gap-2">
                    {canAssign && (
                        <button
                            type="button"
                            onClick={() =>
                                setAssignOpen(
                                    true
                                )
                            }
                            className="rounded-lg border border-slate-300 bg-white px-4 py-2 text-sm font-medium text-slate-700 hover:bg-slate-50"
                        >
                            Assign
                        </button>
                    )}

                    {canOperate &&
                        item.status ===
                            "ASSIGNED" && (
                            <button
                                type="button"
                                disabled={
                                    actionLoading
                                }
                                onClick={() =>
                                    changeStatus(
                                        "ACQUIRING",
                                        "Forensic acquisition started"
                                    )
                                }
                                className="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700 disabled:opacity-50"
                            >
                                Start acquisition
                            </button>
                        )}

                    {canOperate &&
                        item.status ===
                            "ACQUIRING" && (
                            <button
                                type="button"
                                disabled={
                                    actionLoading
                                }
                                onClick={() =>
                                    changeStatus(
                                        "ANALYZING",
                                        "Forensic analysis started"
                                    )
                                }
                                className="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700 disabled:opacity-50"
                            >
                                Begin analysis
                            </button>
                        )}

                    {item.status ===
                        "COMPLETED" && (
                        <button
                            type="button"
                            disabled={
                                actionLoading
                            }
                            onClick={
                                generateReport
                            }
                            className="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700 disabled:opacity-50"
                        >
                            {item.report
                                ?.generated
                                ? "Regenerate report"
                                : "Generate report"}
                        </button>
                    )}
                </div>
            </div>

            <div className="grid grid-cols-1 gap-4 sm:grid-cols-2 xl:grid-cols-4">
                <Metric
                    title="Recovered artifacts"
                    value={
                        item.recoveredArtifacts
                    }
                />

                <Metric
                    title="Validated artifacts"
                    value={
                        item.validatedArtifacts
                    }
                />

                <Metric
                    title="High confidence"
                    value={
                        item.highConfidenceArtifacts
                    }
                />

                <Metric
                    title="Validation rate"
                    value={`${validationRate}%`}
                />
            </div>

            <div className="overflow-hidden rounded-xl border border-slate-200 bg-white shadow-sm">
                <div className="border-b border-slate-200 px-4">
                    <div className="flex gap-1 overflow-x-auto">
                        {tabs.map(tab => (
                            <button
                                key={
                                    tab.id
                                }
                                type="button"
                                onClick={() =>
                                    setActiveTab(
                                        tab.id
                                    )
                                }
                                className={`whitespace-nowrap border-b-2 px-4 py-3 text-sm font-medium transition ${
                                    activeTab ===
                                    tab.id
                                        ? "border-indigo-600 text-indigo-600"
                                        : "border-transparent text-slate-500 hover:border-slate-300 hover:text-slate-700"
                                }`}
                            >
                                {tab.label}
                            </button>
                        ))}
                    </div>
                </div>

                <div className="p-5">
                    {activeTab ===
                        "overview" && (
                        <OverviewTab
                            item={item}
                        />
                    )}

                    {activeTab ===
                        "evidence" && (
                        <EvidenceTab
                            artifacts={
                                artifacts
                            }
                            onInspect={
                                setSelectedArtifact
                            }
                        />
                    )}

                    {activeTab ===
                        "timeline" && (
                        <TimelineTab
                            history={
                                item.history ||
                                []
                            }
                        />
                    )}

                    {activeTab ===
                        "audit" && (
                        <AuditTab
                            auditTrail={
                                auditTrail
                            }
                            loading={
                                auditLoading
                            }
                            stats={
                                auditStats
                            }
                            onRefresh={
                                loadAuditTrail
                            }
                        />
                    )}

                    {activeTab ===
                        "report" && (
                        <ReportTab
                            item={item}
                            onGenerate={
                                generateReport
                            }
                            loading={
                                actionLoading
                            }
                            reportPayload={
                                reportPayload
                            }
                        />
                    )}
                </div>
            </div>

            <div className="grid grid-cols-1 gap-6 xl:grid-cols-3">
                <section className="rounded-lg border border-slate-200 bg-white shadow-sm">
                    <Header
                        title="Case assignment"
                        description="Current operational ownership."
                    />

                    <div className="space-y-4 p-5">
                        <Meta
                            label="Customer"
                            value={
                                item.customer
                                    ?.name ||
                                "—"
                            }
                        />

                        <Meta
                            label="Workstation center"
                            value={
                                item
                                    .workstationCenter
                                    ?.name ||
                                item
                                    .workstationCenter
                                    ?.centerId ||
                                "—"
                            }
                        />

                        <Meta
                            label="Assigned employee"
                            value={
                                item
                                    .assignedEmployee
                                    ?.name ||
                                "Not assigned"
                            }
                        />

                        <Meta
                            label="Assigned workstation"
                            value={
                                item
                                    .assignedWorkstation
                                    ?.name ||
                                item
                                    .assignedWorkstation
                                    ?.workstationId ||
                                "Not assigned"
                            }
                        />

                        {getCurrentWorkstationCode() && (
                            <Meta
                                label="Workstation ID"
                                value={getCurrentWorkstationCode()}
                                mono
                            />
                        )}
                    </div>
                </section>

                <section className="rounded-lg border border-slate-200 bg-white shadow-sm xl:col-span-2">
                    <Header
                        title="Chain of custody"
                        description="The case is linked to the registered source, operator and assigned workstation."
                    />

                    <div className="grid grid-cols-1 gap-5 p-5 sm:grid-cols-3">
                        <Meta
                            label="Case ID"
                            value={
                                item.caseId
                            }
                            mono
                        />

                        <Meta
                            label="Source identifier"
                            value={
                                item.sourceIdentifier ||
                                "Not provided"
                            }
                            mono
                        />

                        <Meta
                            label="Read-only"
                            value={
                                item.readOnly
                                    ? "YES"
                                    : "NO"
                            }
                        />

                        <Meta
                            label="Employee"
                            value={
                                item
                                    .assignedEmployee
                                    ?.name ||
                                "Not assigned"
                            }
                        />

                        <Meta
                            label="Workstation"
                            value={
                                getCurrentWorkstationCode() ||
                                "Not assigned"
                            }
                        />

                        <Meta
                            label="Case created"
                            value={formatDateTime(
                                item.createdAt
                            )}
                        />
                    </div>
                </section>
            </div>

            <ForensicEvidenceDrawer
                artifact={
                    selectedArtifact
                }
                onClose={() =>
                    setSelectedArtifact(null)
                }
            />

            <ForensicModal
                open={assignOpen}
                title="Assign forensic case"
                description="Assign this case to an active employee and workstation belonging to the selected workstation center."
                onClose={() =>
                    setAssignOpen(false)
                }
            >
                <div className="space-y-5">
                    {center ? (
                        <>
                            <div>
                                <label className="text-sm font-medium text-slate-700">
                                    Employee
                                </label>

                                <select
                                    value={
                                        employeeId
                                    }
                                    onChange={event =>
                                        setEmployeeId(
                                            event
                                                .target
                                                .value
                                        )
                                    }
                                    className="mt-1.5 w-full rounded-lg border border-slate-300 bg-white px-3 py-2.5 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100"
                                >
                                    <option value="">
                                        Select employee
                                    </option>

                                    {(
                                        center.employees ||
                                        []
                                    )
                                        .filter(
                                            employee =>
                                                employee.status ===
                                                "ACTIVE"
                                        )
                                        .map(
                                            employee => (
                                                <option
                                                    key={
                                                        employee._id
                                                    }
                                                    value={
                                                        employee._id
                                                    }
                                                >
                                                    {employee.name} —{" "}
                                                    {
                                                        employee.email
                                                    }
                                                </option>
                                            )
                                        )}
                                </select>
                            </div>

                            <div>
                                <label className="text-sm font-medium text-slate-700">
                                    Workstation
                                </label>

                                <select
                                    value={
                                        workstationId
                                    }
                                    onChange={event =>
                                        setWorkstationId(
                                            event
                                                .target
                                                .value
                                        )
                                    }
                                    className="mt-1.5 w-full rounded-lg border border-slate-300 bg-white px-3 py-2.5 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100"
                                >
                                    <option value="">
                                        Select workstation
                                    </option>

                                    {(
                                        center.workstations ||
                                        []
                                    )
                                        .filter(
                                            workstation =>
                                                workstation.status ===
                                                "ACTIVE"
                                        )
                                        .map(
                                            workstation => (
                                                <option
                                                    key={
                                                        workstation._id
                                                    }
                                                    value={
                                                        workstation._id
                                                    }
                                                >
                                                    {workstation.name ||
                                                        "Unnamed workstation"}
                                                    {workstation.workstationId
                                                        ? ` — ${workstation.workstationId}`
                                                        : ""}
                                                </option>
                                            )
                                        )}
                                </select>
                            </div>

                            <div className="rounded-lg border border-amber-200 bg-amber-50 px-4 py-3">
                                <p className="text-xs font-semibold text-amber-900">
                                    Assignment binding
                                </p>

                                <p className="mt-1 text-xs leading-5 text-amber-700">
                                    The selected employee and
                                    workstation become part of
                                    the forensic case ownership
                                    record.
                                </p>
                            </div>

                            <div className="flex justify-end gap-2">
                                <button
                                    type="button"
                                    onClick={() =>
                                        setAssignOpen(
                                            false
                                        )
                                    }
                                    className="rounded-lg border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 hover:bg-slate-50"
                                >
                                    Cancel
                                </button>

                                <button
                                    type="button"
                                    disabled={
                                        actionLoading
                                    }
                                    onClick={
                                        assignCase
                                    }
                                    className="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700 disabled:opacity-50"
                                >
                                    {actionLoading
                                        ? "Assigning..."
                                        : "Assign case"}
                                </button>
                            </div>
                        </>
                    ) : (
                        <p className="text-sm text-slate-500">
                            Unable to load the workstation
                            center assignment data.
                        </p>
                    )}
                </div>
            </ForensicModal>
        </div>
    );
}

function OverviewTab({ item }) {
    return (
        <div className="grid grid-cols-1 gap-6 xl:grid-cols-2">
            <section className="rounded-lg border border-slate-200">
                <Header
                    title="Source information"
                    description="Original source recorded for this investigation."
                />

                <div className="grid grid-cols-1 gap-5 p-5 sm:grid-cols-2">
                    <Meta
                        label="Source type"
                        value={
                            item.sourceType ===
                            "PHYSICAL_DEVICE"
                                ? "Physical device"
                                : "Forensic image"
                        }
                    />

                    <Meta
                        label="Source name"
                        value={
                            item.sourceName
                        }
                    />

                    <Meta
                        label="Identifier"
                        value={
                            item.sourceIdentifier ||
                            "Not provided"
                        }
                        mono
                    />

                    <Meta
                        label="Device type"
                        value={
                            item.deviceType ||
                            "Not specified"
                        }
                    />

                    <Meta
                        label="Capacity"
                        value={
                            item.capacity ||
                            "Not specified"
                        }
                    />

                    <Meta
                        label="Asset identifier"
                        value={
                            item.assetIdentifier ||
                            "Not specified"
                        }
                    />
                </div>

                <div className="border-t border-slate-200 bg-green-50 px-5 py-4">
                    <div className="flex items-center gap-3">
                        <div className="flex h-8 w-8 items-center justify-center rounded-full bg-green-100 text-green-700">
                            ✓
                        </div>

                        <div>
                            <p className="text-sm font-semibold text-green-900">
                                Read-only acquisition mode
                            </p>

                            <p className="text-xs text-green-700">
                                Source modification is not
                                part of the forensic workflow.
                            </p>
                        </div>
                    </div>
                </div>
            </section>

            <section className="rounded-lg border border-slate-200">
                <Header
                    title="Acquisition summary"
                    description="Scan-level evidence collected by the forensic engine."
                />

                <div className="grid grid-cols-2 gap-px bg-slate-200 sm:grid-cols-4">
                    <Summary
                        label="Bytes scanned"
                        value={formatBytes(
                            item.bytesScanned
                        )}
                    />

                    <Summary
                        label="Total bytes"
                        value={formatBytes(
                            item.totalBytes
                        )}
                    />

                    <Summary
                        label="Candidates"
                        value={
                            item.candidatesFound ||
                            0
                        }
                    />

                    <Summary
                        label="Recovered bytes"
                        value={formatBytes(
                            item.recoveredBytes
                        )}
                    />
                </div>

                <div className="border-t border-slate-200 p-5">
                    <div className="flex items-center justify-between text-xs">
                        <span className="font-medium text-slate-600">
                            Processing progress
                        </span>

                        <span className="font-semibold text-slate-900">
                            {item.progress ||
                                0}
                            %
                        </span>
                    </div>

                    <div className="mt-2 h-2 overflow-hidden rounded-full bg-slate-100">
                        <div
                            className="h-full rounded-full bg-indigo-600 transition-all"
                            style={{
                                width: `${Math.min(
                                    100,
                                    Math.max(
                                        0,
                                        item.progress ||
                                            0
                                    )
                                )}%`
                            }}
                        />
                    </div>
                </div>
            </section>

            <section className="rounded-lg border border-slate-200 xl:col-span-2">
                <Header
                    title="Forensic integrity"
                    description="Integrity fields are displayed from the stored acquisition results."
                />

                <div className="grid grid-cols-1 gap-5 p-5 sm:grid-cols-3">
                    <IntegrityCard
                        label="Validated artifacts"
                        value={
                            item.validatedArtifacts ||
                            0
                        }
                        description="Passed validation"
                    />

                    <IntegrityCard
                        label="High confidence"
                        value={
                            item.highConfidenceArtifacts ||
                            0
                        }
                        description="Highest confidence tier"
                    />

                    <IntegrityCard
                        label="Rejected artifacts"
                        value={
                            item.rejectedArtifacts ||
                            0
                        }
                        description="Excluded from final evidence"
                    />
                </div>
            </section>
        </div>
    );
}

function EvidenceTab({
    artifacts,
    onInspect
}) {
    if (!artifacts.length) {
        return (
            <EmptyPanel
                title="No evidence submitted"
                description="No recovered artifacts have been stored for this case yet."
            />
        );
    }

    return (
        <div className="overflow-hidden rounded-lg border border-slate-200">
            <div className="border-b border-slate-200 bg-slate-50 px-5 py-4">
                <p className="text-sm font-semibold text-slate-900">
                    Evidence artifacts
                </p>

                <p className="mt-1 text-xs text-slate-500">
                    Recovered artifacts with validation, confidence
                    and cryptographic integrity information.
                </p>
            </div>

            <div className="overflow-x-auto">
                <table className="min-w-full">
                    <thead className="bg-white">
                        <tr>
                            <Th>
                                Artifact
                            </Th>

                            <Th>
                                Type
                            </Th>

                            <Th>
                                Offset
                            </Th>

                            <Th>
                                Size
                            </Th>

                            <Th>
                                Validation
                            </Th>

                            <Th>
                                Confidence
                            </Th>

                            <Th>
                                SHA-256
                            </Th>

                            <Th />
                        </tr>
                    </thead>

                    <tbody className="divide-y divide-slate-100">
                        {artifacts.map(
                            artifact => (
                                <tr
                                    key={
                                        artifact._id ||
                                        artifact.artifactId
                                    }
                                    className="hover:bg-slate-50"
                                >
                                    <td className="px-4 py-4">
                                        <p className="font-mono text-xs font-semibold text-indigo-600">
                                            {
                                                artifact.artifactId
                                            }
                                        </p>

                                        <p className="mt-1 max-w-[220px] truncate text-sm font-medium text-slate-900">
                                            {artifact.fileName ||
                                                "Recovered artifact"}
                                        </p>
                                    </td>

                                    <td className="px-4 py-4 text-sm text-slate-600">
                                        {
                                            artifact.fileType
                                        }
                                    </td>

                                    <td className="px-4 py-4 font-mono text-xs text-slate-600">
                                        {formatOffset(
                                            artifact.offset
                                        )}
                                    </td>

                                    <td className="px-4 py-4 text-sm text-slate-600">
                                        {formatBytes(
                                            artifact.size
                                        )}
                                    </td>

                                    <td className="px-4 py-4">
                                        <span
                                            className={`rounded-full px-2.5 py-1 text-xs font-semibold ${
                                                artifact.validated
                                                    ? "bg-green-50 text-green-700"
                                                    : "bg-red-50 text-red-700"
                                            }`}
                                        >
                                            {artifact.validated
                                                ? "Validated"
                                                : "Not validated"}
                                        </span>
                                    </td>

                                    <td className="px-4 py-4">
                                        <span className="text-sm font-semibold text-slate-900">
                                            {
                                                artifact.confidenceScore
                                            }
                                            %
                                        </span>

                                        <span className="ml-1 text-xs text-slate-500">
                                            {
                                                artifact.confidenceLevel
                                            }
                                        </span>
                                    </td>

                                    <td className="max-w-[240px] px-4 py-4">
                                        <p
                                            className="truncate font-mono text-[10px] text-slate-500"
                                            title={
                                                artifact.sha256 ||
                                                ""
                                            }
                                        >
                                            {artifact.sha256 ||
                                                "Not available"}
                                        </p>
                                    </td>

                                    <td className="px-4 py-4 text-right">
                                        <button
                                            type="button"
                                            onClick={() =>
                                                onInspect(
                                                    artifact
                                                )
                                            }
                                            className="text-sm font-medium text-indigo-600 hover:text-indigo-700"
                                        >
                                            Inspect
                                        </button>
                                    </td>
                                </tr>
                            )
                        )}
                    </tbody>
                </table>
            </div>
        </div>
    );
}

function TimelineTab({
    history
}) {
    if (!history.length) {
        return (
            <EmptyPanel
                title="No timeline events"
                description="No case status history is available."
            />
        );
    }

    return (
        <div className="max-w-3xl">
            <div className="space-y-5">
                {history.map(
                    (entry, index) => (
                        <div
                            key={`${entry.changedAt}-${index}`}
                            className="relative flex gap-3"
                        >
                            {index <
                                history.length -
                                    1 && (
                                <span className="absolute left-1.5 top-4 h-full w-px bg-slate-200" />
                            )}

                            <span className="relative mt-1.5 h-3 w-3 shrink-0 rounded-full bg-indigo-600 ring-4 ring-indigo-50" />

                            <div className="min-w-0 rounded-lg border border-slate-200 bg-white p-4">
                                <div className="flex flex-wrap items-center gap-2">
                                    <span className="text-sm font-semibold text-slate-900">
                                        {
                                            entry.status
                                        }
                                    </span>

                                    <span className="text-xs text-slate-400">
                                        {formatDateTime(
                                            entry.changedAt
                                        )}
                                    </span>
                                </div>

                                <p className="mt-2 text-xs leading-5 text-slate-500">
                                    {entry.note ||
                                        "Status updated"}
                                </p>

                                {entry.changedBy && (
                                    <p className="mt-2 text-[11px] text-slate-400">
                                        Changed by{" "}
                                        {typeof entry.changedBy ===
                                        "object"
                                            ? entry
                                                  .changedBy
                                                  .name ||
                                              entry
                                                  .changedBy
                                                  .email ||
                                              "User"
                                            : String(
                                                  entry.changedBy
                                              )}
                                    </p>
                                )}
                            </div>
                        </div>
                    )
                )}
            </div>
        </div>
    );
}

function AuditTab({
    auditTrail,
    loading,
    stats,
    onRefresh
}) {
    if (loading) {
        return (
            <div className="py-12 text-center text-sm text-slate-500">
                Loading forensic audit trail...
            </div>
        );
    }

    return (
        <div className="space-y-6">
            <div className="grid grid-cols-1 gap-4 sm:grid-cols-3">
                <IntegrityCard
                    label="Audit events"
                    value={stats.total}
                    description="Recorded lifecycle events"
                />

                <IntegrityCard
                    label="Hashed events"
                    value={stats.hashCount}
                    description="Events with SHA-256 event hash"
                />

                <IntegrityCard
                    label="Chained events"
                    value={stats.chainedCount}
                    description="Events linked to a previous hash"
                />
            </div>

            <div className="flex flex-col gap-3 rounded-lg border border-slate-200 bg-slate-50 p-4 sm:flex-row sm:items-center sm:justify-between">
                <div>
                    <div className="flex items-center gap-2">
                        <span className={`h-2.5 w-2.5 rounded-full ${
                            stats.validChain
                                ? "bg-green-500"
                                : "bg-red-500"
                        }`} />
                        <p className="text-sm font-semibold text-slate-900">
                            Audit integrity: {stats.validChain ? "VALID" : "INVALID"}
                        </p>
                    </div>

                    <p className="mt-1 text-xs text-slate-500">
                        {stats.validChain
                            ? `${stats.validCount} of ${stats.total} recorded event(s) passed hash and chain verification.`
                            : `${stats.invalidCount} audit event(s) failed hash or chain verification. Review the event details below.`}
                    </p>
                </div>

                <button
                    type="button"
                    onClick={onRefresh}
                    className="rounded-lg border border-slate-300 bg-white px-3 py-2 text-xs font-medium text-slate-700 hover:bg-slate-50"
                >
                    Refresh audit
                </button>
            </div>

            {!auditTrail.length ? (
                <EmptyPanel
                    title="No audit events"
                    description="The case does not have audit records yet."
                />
            ) : (
                <div className="overflow-hidden rounded-lg border border-slate-200">
                    <div className="overflow-x-auto">
                        <table className="min-w-full">
                            <thead className="bg-slate-50">
                                <tr>
                                    <Th>
                                        Time
                                    </Th>

                                    <Th>
                                        Action
                                    </Th>

                                    <Th>
                                        Actor
                                    </Th>

                                    <Th>
                                        Workstation
                                    </Th>

                                    <Th>
                                        Status
                                    </Th>

                                    <Th>
                                        Event hash
                                    </Th>
                                </tr>
                            </thead>

                            <tbody className="divide-y divide-slate-100">
                                {auditTrail.map(
                                    (
                                        event,
                                        index
                                    ) => (
                                        <tr
                                            key={
                                                event.auditId ||
                                                event._id ||
                                                `${event.timestamp}-${index}`
                                            }
                                            className="align-top hover:bg-slate-50"
                                        >
                                            <td className="whitespace-nowrap px-4 py-4 text-xs text-slate-500">
                                                {formatDateTime(
                                                    event.timestamp
                                                )}
                                            </td>

                                            <td className="px-4 py-4">
                                                <p className="text-xs font-semibold text-slate-900">
                                                    {formatAuditAction(
                                                        event.action
                                                    )}
                                                </p>

                                                {event.note && (
                                                    <p className="mt-1 max-w-[260px] text-[11px] leading-5 text-slate-500">
                                                        {
                                                            event.note
                                                        }
                                                    </p>
                                                )}

                                                {event.hashValid !== undefined && (
                                                    <span className={`mt-2 inline-flex rounded-full px-2 py-1 text-[10px] font-semibold ${
                                                        event.valid
                                                            ? "bg-green-50 text-green-700"
                                                            : "bg-red-50 text-red-700"
                                                    }`}>
                                                        {event.valid ? "Integrity valid" : "Integrity check failed"}
                                                    </span>
                                                )}
                                            </td>

                                            <td className="px-4 py-4">
                                                <p className="text-xs font-medium text-slate-900">
                                                    {event.actor
                                                        ?.name ||
                                                        event.actor
                                                            ?.email ||
                                                        "Unknown"}
                                                </p>

                                                <p className="mt-1 text-[10px] text-slate-400">
                                                    {
                                                        event.actorRole
                                                    }
                                                </p>
                                            </td>

                                            <td className="px-4 py-4">
                                                <p className="text-xs font-medium text-slate-700">
                                                    {event
                                                        .workstation
                                                        ?.workstationId ||
                                                        event.workstationId ||
                                                        "—"}
                                                </p>

                                                {event
                                                    .workstation
                                                    ?.name && (
                                                    <p className="mt-1 text-[10px] text-slate-400">
                                                        {
                                                            event
                                                                .workstation
                                                                .name
                                                        }
                                                    </p>
                                                )}
                                            </td>

                                            <td className="px-4 py-4">
                                                <div className="flex flex-wrap items-center gap-1.5">
                                                    {event.fromStatus && (
                                                        <StatusPill
                                                            value={
                                                                event.fromStatus
                                                            }
                                                        />
                                                    )}

                                                    {event.toStatus && (
                                                        <>
                                                            <span className="text-slate-300">
                                                                →
                                                            </span>

                                                            <StatusPill
                                                                value={
                                                                    event.toStatus
                                                                }
                                                            />
                                                        </>
                                                    )}
                                                </div>
                                            </td>

                                            <td className="px-4 py-4">
                                                <div className="max-w-[260px]">
                                                    <p
                                                        className="truncate font-mono text-[10px] text-slate-500"
                                                        title={
                                                            event.eventHash ||
                                                            ""
                                                        }
                                                    >
                                                        {
                                                            event.eventHash
                                                        }
                                                    </p>

                                                    {event.previousEventHash && (
                                                        <p
                                                            className="mt-1 truncate font-mono text-[10px] text-slate-400"
                                                            title={
                                                                event.previousEventHash
                                                            }
                                                        >
                                                            prev:{" "}
                                                            {
                                                                event.previousEventHash
                                                            }
                                                        </p>
                                                    )}
                                                </div>
                                            </td>
                                        </tr>
                                    )
                                )}
                            </tbody>
                        </table>
                    </div>
                </div>
            )}
        </div>
    );
}

function ReportTab({
    item,
    onGenerate,
    loading,
    reportPayload
}) {
    const downloadReport = () => {
        if (!reportPayload) {
            return;
        }

        const content = JSON.stringify(
            reportPayload,
            null,
            2
        );

        const blob = new Blob(
            [content],
            { type: "application/json" }
        );

        const url = URL.createObjectURL(blob);
        const anchor = document.createElement("a");
        anchor.href = url;
        anchor.download = `${item.caseId || "forensic-case"}-forensic-report.json`;
        document.body.appendChild(anchor);
        anchor.click();
        anchor.remove();
        URL.revokeObjectURL(url);
    };

    if (!item.report?.generated) {
        return (
            <div className="rounded-lg border border-dashed border-slate-300 bg-slate-50 px-6 py-12 text-center">
                <p className="text-sm font-semibold text-slate-900">
                    No forensic report generated
                </p>

                <p className="mx-auto mt-2 max-w-lg text-xs leading-5 text-slate-500">
                    The report becomes available after the case reaches COMPLETED status.
                </p>

                {item.status === "COMPLETED" && (
                    <button
                        type="button"
                        disabled={loading}
                        onClick={onGenerate}
                        className="mt-5 rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700 disabled:opacity-50"
                    >
                        {loading ? "Generating..." : "Generate report"}
                    </button>
                )}
            </div>
        );
    }

    return (
        <div className="space-y-6">
            <section className="rounded-lg border border-green-200 bg-green-50 p-5">
                <div className="flex flex-col gap-4 sm:flex-row sm:items-start sm:justify-between">
                    <div>
                        <p className="text-sm font-semibold text-green-900">
                            Forensic report generated
                        </p>

                        <p className="mt-1 text-xs text-green-700">
                            The report contains the forensic case summary, evidence and audit snapshot together with its SHA-256 integrity hash.
                        </p>
                    </div>

                    <span className="self-start rounded-full bg-green-100 px-2.5 py-1 text-[10px] font-semibold text-green-700">
                        GENERATED
                    </span>
                </div>

                <div className="mt-5 grid grid-cols-1 gap-5 sm:grid-cols-2">
                    <Meta
                        label="Generated at"
                        value={formatDateTime(item.report.generatedAt)}
                    />

                    <Meta
                        label="Case status"
                        value={item.status}
                    />
                </div>

                <div className="mt-5 rounded-lg border border-green-200 bg-white p-4">
                    <p className="text-xs font-semibold uppercase tracking-wide text-slate-400">
                        Report SHA-256
                    </p>

                    <p className="mt-2 break-all font-mono text-xs leading-5 text-slate-700">
                        {item.report.reportHash || "Not available"}
                    </p>
                </div>

                <div className="mt-4 flex flex-wrap gap-2">
                    <button
                        type="button"
                        disabled={!reportPayload}
                        onClick={downloadReport}
                        className="rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700 disabled:cursor-not-allowed disabled:opacity-50"
                    >
                        {reportPayload
                            ? "Download JSON report"
                            : "Download available after generation"}
                    </button>

                    <button
                        type="button"
                        disabled={loading}
                        onClick={onGenerate}
                        className="rounded-lg border border-slate-300 bg-white px-4 py-2 text-sm font-medium text-slate-700 hover:bg-slate-50 disabled:opacity-50"
                    >
                        {loading ? "Generating..." : "Regenerate report"}
                    </button>
                </div>
            </section>

            <section className="rounded-lg border border-slate-200">
                <Header
                    title="Report coverage"
                    description="Evidence and acquisition values included in the report record."
                />

                <div className="grid grid-cols-1 gap-5 p-5 sm:grid-cols-3">
                    <IntegrityCard
                        label="Artifacts"
                        value={item.artifacts?.length || 0}
                        description="Stored evidence artifacts"
                    />

                    <IntegrityCard
                        label="Validated"
                        value={item.validatedArtifacts || 0}
                        description="Validated artifacts"
                    />

                    <IntegrityCard
                        label="High confidence"
                        value={item.highConfidenceArtifacts || 0}
                        description="Highest confidence tier"
                    />
                </div>
            </section>

            {!reportPayload && (
                <div className="rounded-lg border border-amber-200 bg-amber-50 px-4 py-3">
                    <p className="text-xs font-semibold text-amber-900">
                        Report download state
                    </p>
                    <p className="mt-1 text-xs leading-5 text-amber-700">
                        This page has the stored report hash, but the current backend response only exposes the full report payload during generation. Generate the report again in this session to download its JSON payload.
                    </p>
                </div>
            )}
        </div>
    );
}

function Header({
    title,
    description
}) {
    return (
        <div className="border-b border-slate-200 px-5 py-4">
            <h2 className="text-base font-semibold text-slate-900">
                {title}
            </h2>

            {description && (
                <p className="mt-1 text-sm text-slate-500">
                    {description}
                </p>
            )}
        </div>
    );
}

function Metric({
    title,
    value
}) {
    return (
        <div className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
            <p className="text-sm text-slate-500">
                {title}
            </p>

            <p className="mt-2 text-2xl font-semibold text-slate-900">
                {value ?? 0}
            </p>
        </div>
    );
}

function Meta({
    label,
    value,
    mono = false
}) {
    return (
        <div>
            <p className="text-xs font-medium text-slate-400">
                {label}
            </p>

            <p
                className={`mt-1 text-sm text-slate-700 ${
                    mono
                        ? "break-all font-mono text-xs"
                        : ""
                }`}
            >
                {value || "—"}
            </p>
        </div>
    );
}

function Summary({
    label,
    value
}) {
    return (
        <div className="bg-white p-4">
            <p className="text-xs text-slate-500">
                {label}
            </p>

            <p className="mt-1 text-sm font-semibold text-slate-900">
                {value}
            </p>
        </div>
    );
}

function IntegrityCard({
    label,
    value,
    description
}) {
    return (
        <div className="rounded-lg border border-slate-200 bg-white p-4">
            <p className="text-xs font-medium text-slate-400">
                {label}
            </p>

            <p className="mt-2 text-2xl font-semibold text-slate-900">
                {value ?? 0}
            </p>

            {description && (
                <p className="mt-1 text-xs text-slate-500">
                    {description}
                </p>
            )}
        </div>
    );
}

function EmptyPanel({
    title,
    description
}) {
    return (
        <div className="rounded-lg border border-dashed border-slate-300 bg-slate-50 px-6 py-12 text-center">
            <p className="text-sm font-semibold text-slate-900">
                {title}
            </p>

            <p className="mx-auto mt-2 max-w-lg text-xs leading-5 text-slate-500">
                {description}
            </p>
        </div>
    );
}

function StatusPill({
    value
}) {
    if (!value) {
        return null;
    }

    return (
        <span className="rounded-full bg-slate-100 px-2 py-1 text-[10px] font-semibold text-slate-600">
            {value}
        </span>
    );
}

function Th({
    children
}) {
    return (
        <th className="px-4 py-3 text-left text-[11px] font-semibold uppercase tracking-wide text-slate-500">
            {children}
        </th>
    );
}

function formatAuditAction(
    action
) {
    if (!action) {
        return "Unknown event";
    }

    return action
        .replaceAll("_", " ")
        .toLowerCase()
        .replace(/\b\w/g, char =>
            char.toUpperCase()
        );
}

function formatOffset(offset) {
    const value =
        Number(offset) || 0;

    return `0x${value
        .toString(16)
        .toUpperCase()}`;
}

function formatBytes(bytes) {
    const value =
        Number(bytes) || 0;

    if (value < 1024) {
        return `${value} B`;
    }

    if (value < 1024 ** 2) {
        return `${(
            value / 1024
        ).toFixed(1)} KB`;
    }

    if (value < 1024 ** 3) {
        return `${(
            value /
            1024 ** 2
        ).toFixed(1)} MB`;
    }

    return `${(
        value /
        1024 ** 3
    ).toFixed(2)} GB`;
}

function formatDateTime(value) {
    if (!value) {
        return "—";
    }

    const date =
        new Date(value);

    if (
        Number.isNaN(
            date.getTime()
        )
    ) {
        return "—";
    }

    return date.toLocaleString(
        undefined,
        {
            dateStyle:
                "medium",
            timeStyle:
                "short"
        }
    );
}

export default ForensicCaseDetails;