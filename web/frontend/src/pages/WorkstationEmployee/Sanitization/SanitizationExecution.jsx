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
    const { requestId } = useParams();
    const navigate = useNavigate();

    const [request, setRequest] = useState(null);
    const [result, setResult] = useState(null);
    const [certificate, setCertificate] = useState(null);
    const [loading, setLoading] = useState(true);
    const [actionLoading, setActionLoading] = useState(false);
    const [error, setError] = useState("");

    const loadData = async (showLoading = true) => {
        try {
            if (showLoading) {
                setLoading(true);
            }

            setError("");

            const requests = await getEmployeeSanitizationRequests();

            const matchedRequest = requests.find(
                (item) => item.requestId === requestId
            );

            if (!matchedRequest) {
                throw new Error("Assigned sanitization request not found.");
            }

            setRequest(matchedRequest);

            try {
                const resultData = await getSanitizationResult(requestId);

                setResult(resultData);

                if (resultData?.certificate) {
                    setCertificate(resultData.certificate);
                } else {
                    setCertificate(null);
                }
            } catch (resultError) {
                if (resultError.status !== 404) {
                    console.warn(
                        "Sanitization result not available:",
                        resultError
                    );
                }

                setResult(null);
                setCertificate(null);
            }

            return matchedRequest;
        } catch (err) {
            console.error(
                "Failed to load sanitization execution:",
                err
            );

            setError(
                err.message ||
                    "Failed to load sanitization execution."
            );

            return null;
        } finally {
            if (showLoading) {
                setLoading(false);
            }
        }
    };

    useEffect(() => {
        let cancelled = false;
        let timerId = null;

        const poll = async () => {
            if (cancelled) {
                return;
            }

            const latestRequest = await loadData(false);

            if (cancelled || !latestRequest) {
                return;
            }

            if (
                [
                    "ASSIGNED",
                    "IN_PROGRESS",
                    "VERIFYING",
                ].includes(latestRequest.status)
            ) {
                timerId = window.setTimeout(poll, 2000);
            }
        };

        loadData(true).then((latestRequest) => {
            if (cancelled || !latestRequest) {
                return;
            }

            if (
                [
                    "ASSIGNED",
                    "IN_PROGRESS",
                    "VERIFYING",
                ].includes(latestRequest.status)
            ) {
                timerId = window.setTimeout(poll, 2000);
            }
        });

        return () => {
            cancelled = true;

            if (timerId !== null) {
                window.clearTimeout(timerId);
            }
        };
    }, [requestId]);

    const canStart = request?.status === "ASSIGNED";
    const waitingForDesktop = request?.status === "IN_PROGRESS";
    const verifying = request?.status === "VERIFYING";
    const completed = request?.status === "COMPLETED";
    const failed = request?.status === "FAILED";

    const handleStart = async () => {
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

    const deviceLabel = useMemo(
        () =>
            [
                request?.deviceType,
                request?.capacity,
            ]
                .filter(Boolean)
                .join(" • ") ||
            "Device information unavailable",
        [request]
    );

    const certificateId =
        certificate?.certificateId ||
        result?.certificate?.certificateId ||
        "";

    if (loading) {
        return (
            <div className="flex min-h-[420px] items-center justify-center">
                <div className="w-full max-w-md rounded-2xl border border-slate-200 bg-white p-8 text-center shadow-sm">
                    <div className="mx-auto flex h-12 w-12 items-center justify-center rounded-full bg-indigo-50">
                        <div className="h-5 w-5 animate-spin rounded-full border-2 border-indigo-600 border-t-transparent" />
                    </div>

                    <h2 className="mt-5 text-base font-semibold text-slate-900">
                        Loading sanitization job
                    </h2>

                    <p className="mt-2 text-sm leading-6 text-slate-500">
                        Fetching the latest request, execution result,
                        and certificate information.
                    </p>
                </div>
            </div>
        );
    }

    if (error && !request) {
        return (
            <div className="space-y-4">
                <div className="rounded-2xl border border-red-200 bg-red-50 p-5">
                    <div className="flex gap-3">
                        <div className="mt-0.5 flex h-8 w-8 shrink-0 items-center justify-center rounded-full bg-red-100 text-red-700">
                            !
                        </div>

                        <div>
                            <p className="text-sm font-semibold text-red-900">
                                Unable to load sanitization job
                            </p>

                            <p className="mt-1 text-sm leading-6 text-red-700">
                                {error}
                            </p>
                        </div>
                    </div>
                </div>

                <button
                    type="button"
                    onClick={() =>
                        navigate(
                            "/workstation-employee/dashboard"
                        )
                    }
                    className="rounded-lg border border-slate-300 bg-white px-4 py-2 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                >
                    Back to Dashboard
                </button>
            </div>
        );
    }

    return (
        <div className="space-y-6">
            <div className="flex flex-col gap-4 lg:flex-row lg:items-end lg:justify-between">
                <div>
                    <div className="flex items-center gap-2 text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                        <span className="h-1.5 w-1.5 rounded-full bg-indigo-600" />
                        Sanitization Execution
                    </div>

                    <h1 className="mt-2 text-2xl font-semibold tracking-tight text-slate-900">
                        {request.requestId}
                    </h1>

                    <p className="mt-1 text-sm text-slate-500">
                        {deviceLabel}
                    </p>
                </div>

                <div className="flex flex-wrap gap-2">
                    {certificateId && (
                        <Link
                            to={`/workstation-employee/sanitization/certificate/${certificateId}`}
                            className="inline-flex items-center justify-center rounded-lg bg-indigo-600 px-4 py-2.5 text-sm font-medium text-white transition hover:bg-indigo-700"
                        >
                            View Certificate
                        </Link>
                    )}

                    <Link
                        to="/workstation-employee/sanitization/history"
                        className="inline-flex items-center justify-center rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                    >
                        Back to History
                    </Link>
                </div>
            </div>

            {error && (
                <div className="rounded-xl border border-red-200 bg-red-50 px-4 py-3">
                    <p className="text-sm text-red-700">
                        {error}
                    </p>
                </div>
            )}

            <SanitizationPipeline
                status={request.status}
            />

            <section className="overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-sm">
                <div className="border-b border-slate-200 px-5 py-4">
                    <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
                        <div>
                            <h2 className="text-base font-semibold text-slate-900">
                                Target & Assignment
                            </h2>

                            <p className="mt-1 text-sm text-slate-500">
                                Operational information for this sanitization job.
                            </p>
                        </div>

                        <StatusBadge status={request.status} />
                    </div>
                </div>

                <div className="grid gap-x-6 gap-y-6 p-5 md:grid-cols-2 xl:grid-cols-4">
                    <Meta
                        label="Device"
                        value={request.deviceType}
                    />

                    <Meta
                        label="Capacity"
                        value={request.capacity}
                    />

                    <Meta
                        label="Authorized Serial Number"
                        value={request.serialNumber}
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
                        value={request.assignedEmployee?.name}
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
                <section className="overflow-hidden rounded-2xl border border-indigo-200 bg-indigo-50">
                    <div className="p-5">
                        <div className="flex flex-col gap-4 md:flex-row md:items-center md:justify-between">
                            <div>
                                <div className="flex items-center gap-2">
                                    <span className="flex h-8 w-8 items-center justify-center rounded-lg bg-indigo-100 text-sm font-bold text-indigo-700">
                                        1
                                    </span>

                                    <h2 className="text-base font-semibold text-indigo-900">
                                        Ready for execution
                                    </h2>
                                </div>

                                <p className="mt-2 max-w-3xl text-sm leading-6 text-indigo-700">
                                    Start the assigned job to move it into
                                    IN_PROGRESS. The actual device operation
                                    is performed by the SecureWipe desktop
                                    engine on the assigned workstation.
                                </p>
                            </div>

                            <button
                                type="button"
                                disabled={actionLoading}
                                onClick={handleStart}
                                className="shrink-0 rounded-lg bg-indigo-600 px-5 py-2.5 text-sm font-medium text-white transition hover:bg-indigo-700 disabled:cursor-not-allowed disabled:opacity-50"
                            >
                                {actionLoading
                                    ? "Starting..."
                                    : "Start Sanitization"}
                            </button>
                        </div>
                    </div>
                </section>
            )}

            {waitingForDesktop && (
                <section className="overflow-hidden rounded-2xl border border-amber-200 bg-amber-50">
                    <div className="p-5">
                        <div className="flex flex-col gap-4 md:flex-row md:items-center md:justify-between">
                            <div>
                                <div className="flex items-center gap-2">
                                    <span className="flex h-8 w-8 items-center justify-center rounded-lg bg-amber-100 text-sm font-bold text-amber-800">
                                        2
                                    </span>

                                    <h2 className="text-base font-semibold text-amber-900">
                                        Desktop execution in progress
                                    </h2>
                                </div>

                                <p className="mt-2 max-w-3xl text-sm leading-6 text-amber-700">
                                    This request is now IN_PROGRESS. The
                                    assigned desktop workstation should
                                    perform the safety checks, capability
                                    detection, sanitization, verification,
                                    and result submission.
                                </p>
                            </div>

                            <button
                                type="button"
                                onClick={loadData}
                                className="shrink-0 rounded-lg border border-amber-300 bg-white px-4 py-2.5 text-sm font-medium text-amber-800 transition hover:bg-amber-100"
                            >
                                Refresh State
                            </button>
                        </div>
                    </div>
                </section>
            )}

            {verifying && (
                <section className="overflow-hidden rounded-2xl border border-blue-200 bg-blue-50">
                    <div className="p-5">
                        <div className="flex flex-col gap-4 md:flex-row md:items-center md:justify-between">
                            <div>
                                <div className="flex items-center gap-2">
                                    <span className="flex h-8 w-8 items-center justify-center rounded-lg bg-blue-100 text-sm font-bold text-blue-700">
                                        3
                                    </span>

                                    <h2 className="text-base font-semibold text-blue-900">
                                        Verification in progress
                                    </h2>
                                </div>

                                <p className="mt-2 max-w-3xl text-sm leading-6 text-blue-700">
                                    The sanitization result has been received.
                                    SecureWipe is validating the recorded
                                    evidence before the request can become
                                    COMPLETED.
                                </p>
                            </div>

                            <button
                                type="button"
                                onClick={loadData}
                                className="shrink-0 rounded-lg border border-blue-300 bg-white px-4 py-2.5 text-sm font-medium text-blue-800 transition hover:bg-blue-100"
                            >
                                Refresh Verification
                            </button>
                        </div>
                    </div>
                </section>
            )}

            {failed && (
                <section className="overflow-hidden rounded-2xl border border-red-200 bg-red-50">
                    <div className="p-5">
                        <div className="flex items-start gap-3">
                            <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-lg bg-red-100 font-bold text-red-700">
                                !
                            </div>

                            <div>
                                <h2 className="text-base font-semibold text-red-900">
                                    Sanitization failed
                                </h2>

                                <p className="mt-1 text-sm leading-6 text-red-700">
                                    Review the recorded result and failure
                                    evidence before taking the next
                                    operational action.
                                </p>
                            </div>
                        </div>
                    </div>
                </section>
            )}

            {completed && certificateId && (
                <section className="overflow-hidden rounded-2xl border border-green-200 bg-green-50">
                    <div className="p-5">
                        <div className="flex flex-col gap-4 md:flex-row md:items-center md:justify-between">
                            <div>
                                <div className="flex items-center gap-2">
                                    <span className="flex h-8 w-8 items-center justify-center rounded-full bg-green-100 text-green-700">
                                        ✓
                                    </span>

                                    <h2 className="text-base font-semibold text-green-900">
                                        Sanitization completed
                                    </h2>
                                </div>

                                <p className="mt-2 text-sm leading-6 text-green-700">
                                    The request has reached COMPLETED and a
                                    sanitization certificate is available.
                                </p>

                                <p className="mt-2 break-all font-mono text-xs text-green-800">
                                    Certificate: {certificateId}
                                </p>
                            </div>

                            <Link
                                to={`/workstation-employee/sanitization/certificate/${certificateId}`}
                                className="shrink-0 rounded-lg bg-green-600 px-4 py-2.5 text-sm font-medium text-white transition hover:bg-green-700"
                            >
                                Open Certificate
                            </Link>
                        </div>
                    </div>
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

            {completed && !certificateId && (
                <section className="overflow-hidden rounded-2xl border border-green-200 bg-green-50 p-5">
                    <div className="flex items-start gap-3">
                        <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-full bg-green-100 text-green-700">
                            ✓
                        </div>

                        <div>
                            <h2 className="text-base font-semibold text-green-900">
                                Sanitization completed
                            </h2>

                            <p className="mt-1 text-sm leading-6 text-green-700">
                                The request is COMPLETED, but certificate
                                information has not been loaded on this page.
                                Refresh the page to retrieve the latest
                                certificate state.
                            </p>
                        </div>
                    </div>
                </section>
            )}
        </div>
    );
}

function Meta({ label, value }) {
    return (
        <div className="min-w-0">
            <p className="text-xs font-medium uppercase tracking-wide text-slate-400">
                {label}
            </p>

            <p className="mt-1 break-words text-sm font-medium text-slate-700">
                {value ?? "—"}
            </p>
        </div>
    );
}

function StatusBadge({ status }) {
    const styles = {
        ASSIGNED:
            "border-slate-200 bg-slate-100 text-slate-700",
        IN_PROGRESS:
            "border-indigo-200 bg-indigo-100 text-indigo-700",
        VERIFYING:
            "border-blue-200 bg-blue-100 text-blue-700",
        COMPLETED:
            "border-green-200 bg-green-100 text-green-700",
        FAILED:
            "border-red-200 bg-red-100 text-red-700",
    };

    return (
        <span
            className={`inline-flex w-fit items-center rounded-full border px-3 py-1 text-xs font-semibold ${
                styles[status] ||
                "border-slate-200 bg-slate-100 text-slate-700"
            }`}
        >
            {status || "UNKNOWN"}
        </span>
    );
}

function formatDate(value) {
    if (!value) {
        return "—";
    }

    const date = new Date(value);

    if (Number.isNaN(date.getTime())) {
        return "—";
    }

    return date.toLocaleString(undefined, {
        dateStyle: "medium",
        timeStyle: "short",
    });
}

export default SanitizationExecution;