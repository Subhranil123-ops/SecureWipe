import { useEffect, useMemo, useState } from "react";
import { Link, useNavigate, useParams } from "react-router-dom";

import {
    getEmployeeSanitizationRequests,
    updateEmployeeSanitizationStatus,
} from "../../../services/sanitizationRequestService";

import {
    getSanitizationResult,
} from "../../../services/sanitizationResultService";

import SanitizationPipeline from "../../../components/sanitization/SanitizationPipeline";
import SanitizationResultCard from "../../../components/sanitization/SanitizationResultCard";
import SanitizationCertificateCard from "../../../components/sanitization/SanitizationCertificateCard";

function SanitizationExecution() {

    const {
        requestId
    } = useParams();

    const navigate = useNavigate();

    const [request, setRequest] =
        useState(null);

    const [result, setResult] =
        useState(null);

    const [certificate, setCertificate] =
        useState(null);

    const [loading, setLoading] =
        useState(true);

    const [actionLoading, setActionLoading] =
        useState(false);

    const [error, setError] =
        useState("");

    const loadData =
        async () => {

            try {

                setLoading(true);
                setError("");

                const requests =
                    await getEmployeeSanitizationRequests();

                const matchedRequest =
                    requests.find(
                        (item) =>
                            item.requestId ===
                            requestId
                    );

                if (!matchedRequest) {
                    throw new Error(
                        "Assigned sanitization request not found."
                    );
                }

                setRequest(
                    matchedRequest
                );

                try {

                    const resultData =
                        await getSanitizationResult(
                            requestId
                        );

                    setResult(
                        resultData
                    );

                    if (
                        resultData?.certificate
                    ) {
                        setCertificate(
                            resultData.certificate
                        );
                    }

                } catch (resultError) {

                    if (
                        resultError.status !==
                        404
                    ) {
                        console.warn(
                            "Sanitization result not available:",
                            resultError
                        );
                    }

                    setResult(null);
                }

            } catch (err) {

                console.error(
                    "Failed to load sanitization execution:",
                    err
                );

                setError(
                    err.message ||
                    "Failed to load sanitization execution."
                );

            } finally {

                setLoading(false);
            }
        };

    useEffect(() => {

        loadData();

    }, [requestId]);

    const canStart =
        request?.status ===
        "ASSIGNED";

    const waitingForDesktop =
        request?.status ===
        "IN_PROGRESS";

    const verifying =
        request?.status ===
        "VERIFYING";

    const completed =
        request?.status ===
        "COMPLETED";

    const failed =
        request?.status ===
        "FAILED";

    const handleStart =
        async () => {

            try {

                setActionLoading(true);
                setError("");

                await updateEmployeeSanitizationStatus(
                    requestId,
                    "IN_PROGRESS"
                );

                await loadData();

            } catch (err) {

                console.error(
                    "Failed to start sanitization:",
                    err
                );

                setError(
                    err.message ||
                    "Failed to start sanitization."
                );

            } finally {

                setActionLoading(false);
            }
        };

    const deviceLabel =
        useMemo(
            () =>
                [
                    request?.deviceType,
                    request?.capacity
                ]
                    .filter(Boolean)
                    .join(" • ") ||
                "Device information unavailable",
            [request]
        );

    if (loading) {

        return (
            <div className="rounded-lg border border-slate-200 bg-white p-6 shadow-sm">
                <p className="text-sm text-slate-500">
                    Loading sanitization execution...
                </p>
            </div>
        );
    }

    if (error && !request) {

        return (
            <div className="space-y-4">

                <div className="rounded-lg border border-red-200 bg-red-50 p-5">
                    <p className="text-sm text-red-700">
                        {error}
                    </p>
                </div>

                <button
                    type="button"
                    onClick={() =>
                        navigate(
                            "/workstation-employee/dashboard"
                        )
                    }
                    className="rounded-lg border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 hover:bg-slate-50"
                >
                    Back to Dashboard
                </button>

            </div>
        );
    }

    return (
        <div className="space-y-6">

            <div className="flex flex-col gap-3 lg:flex-row lg:items-end lg:justify-between">

                <div>
                    <p className="text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                        Sanitization Execution
                    </p>

                    <h1 className="mt-1 text-2xl font-semibold text-slate-900">
                        {request.requestId}
                    </h1>

                    <p className="mt-1 text-sm text-slate-500">
                        {deviceLabel}
                    </p>
                </div>

                <Link
                    to="/workstation-employee/dashboard"
                    className="w-fit rounded-lg border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 hover:bg-slate-50"
                >
                    Back to Requests
                </Link>

            </div>

            {error && (
                <div className="rounded-lg border border-red-200 bg-red-50 px-4 py-3">
                    <p className="text-sm text-red-700">
                        {error}
                    </p>
                </div>
            )}

            <SanitizationPipeline
                status={request.status}
            />

            <section className="rounded-lg border border-slate-200 bg-white shadow-sm">

                <div className="border-b border-slate-200 px-5 py-4">

                    <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">

                        <div>
                            <h2 className="text-base font-semibold text-slate-900">
                                Target & Assignment
                            </h2>

                            <p className="mt-1 text-sm text-slate-500">
                                Operational information for the assigned sanitization job.
                            </p>
                        </div>

                        <StatusBadge
                            status={request.status}
                        />

                    </div>

                </div>

                <div className="grid gap-5 p-5 md:grid-cols-2 xl:grid-cols-4">

                    <Meta
                        label="Device"
                        value={request.deviceType}
                    />

                    <Meta
                        label="Capacity"
                        value={request.capacity}
                    />

                    <Meta
                        label="Requested Method"
                        value={request.sanitizationMethod}
                    />

                    <Meta
                        label="Asset Identifier"
                        value={request.assetIdentifier}
                    />

                    <Meta
                        label="Workstation Center"
                        value={
                            request.workstationCenter?.name ||
                            request.workstationCenter?.centerId
                        }
                    />

                    <Meta
                        label="Assigned Employee"
                        value={
                            request.assignedEmployee?.name
                        }
                    />

                    <Meta
                        label="Workstation"
                        value={
                            request.assignedWorkstation?.name ||
                            request.assignedWorkstation?.workstationId
                        }
                    />

                    <Meta
                        label="Assigned At"
                        value={formatDate(request.assignedAt)}
                    />

                </div>

            </section>

            {canStart && (
                <section className="rounded-lg border border-indigo-200 bg-indigo-50 p-5">

                    <h2 className="text-base font-semibold text-indigo-900">
                        Ready for execution
                    </h2>

                    <p className="mt-1 max-w-3xl text-sm leading-6 text-indigo-700">
                        Starting this job changes the request state to
                        <strong> IN_PROGRESS</strong>. The actual device
                        sanitization is performed by the SecureWipe desktop
                        engine on the assigned workstation.
                    </p>

                    <button
                        type="button"
                        disabled={actionLoading}
                        onClick={handleStart}
                        className="mt-4 rounded-lg bg-indigo-600 px-4 py-2.5 text-sm font-medium text-white hover:bg-indigo-700 disabled:cursor-not-allowed disabled:opacity-50"
                    >
                        {actionLoading
                            ? "Starting..."
                            : "Start Sanitization"}
                    </button>

                </section>
            )}

            {waitingForDesktop && (
                <section className="rounded-lg border border-amber-200 bg-amber-50 p-5">

                    <h2 className="text-base font-semibold text-amber-900">
                        Desktop execution in progress
                    </h2>

                    <p className="mt-1 text-sm leading-6 text-amber-700">
                        The request is now IN_PROGRESS. The desktop workstation
                        should perform the real safety checks, capability
                        detection, sanitization and verification, then submit
                        the structured result to SecureWipe.
                    </p>

                    <button
                        type="button"
                        onClick={loadData}
                        className="mt-4 rounded-lg border border-amber-300 bg-white px-4 py-2 text-sm font-medium text-amber-800 hover:bg-amber-100"
                    >
                        Refresh Execution State
                    </button>

                </section>
            )}

            {verifying && (
                <section className="rounded-lg border border-blue-200 bg-blue-50 p-5">

                    <h2 className="text-base font-semibold text-blue-900">
                        Verification in progress
                    </h2>

                    <p className="mt-1 text-sm leading-6 text-blue-700">
                        The sanitization result has been received and is being
                        validated before the request can become COMPLETED.
                    </p>

                    <button
                        type="button"
                        onClick={loadData}
                        className="mt-4 rounded-lg border border-blue-300 bg-white px-4 py-2 text-sm font-medium text-blue-800 hover:bg-blue-100"
                    >
                        Refresh Verification
                    </button>

                </section>
            )}

            {failed && (
                <section className="rounded-lg border border-red-200 bg-red-50 p-5">

                    <h2 className="text-base font-semibold text-red-900">
                        Sanitization failed
                    </h2>

                    <p className="mt-1 text-sm text-red-700">
                        Review the recorded result and failure evidence before
                        retrying through the operational workflow.
                    </p>

                </section>
            )}

            {result && (
                <SanitizationResultCard
                    result={result}
                />
            )}

            {certificate && (
                <SanitizationCertificateCard
                    certificate={certificate}
                />
            )}

            {completed && !certificate && (
                <section className="rounded-lg border border-green-200 bg-green-50 p-5">

                    <h2 className="text-base font-semibold text-green-900">
                        Sanitization completed
                    </h2>

                    <p className="mt-1 text-sm text-green-700">
                        The request is COMPLETED, but the certificate has not
                        been loaded on this page yet.
                    </p>

                </section>
            )}

        </div>
    );
}

function Meta({
    label,
    value
}) {

    return (
        <div>
            <p className="text-xs font-medium text-slate-400">
                {label}
            </p>

            <p className="mt-1 text-sm text-slate-700">
                {value || "—"}
            </p>
        </div>
    );
}

function StatusBadge({
    status
}) {

    const styles = {
        ASSIGNED:
            "bg-slate-100 text-slate-700",
        IN_PROGRESS:
            "bg-indigo-100 text-indigo-700",
        VERIFYING:
            "bg-blue-100 text-blue-700",
        COMPLETED:
            "bg-green-100 text-green-700",
        FAILED:
            "bg-red-100 text-red-700",
    };

    return (
        <span
            className={`inline-flex w-fit rounded-full px-3 py-1 text-xs font-semibold ${
                styles[status] ||
                "bg-slate-100 text-slate-700"
            }`}
        >
            {status}
        </span>
    );
}

function formatDate(value) {

    if (!value) {
        return "—";
    }

    return new Date(
        value
    ).toLocaleString(
        undefined,
        {
            dateStyle: "medium",
            timeStyle: "short",
        }
    );
}

export default SanitizationExecution;