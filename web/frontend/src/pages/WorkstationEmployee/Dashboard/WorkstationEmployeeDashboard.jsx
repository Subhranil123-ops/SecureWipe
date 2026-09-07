import { useEffect, useMemo, useState } from "react";
import { Link } from "react-router-dom";

import { useAuth } from "../../../context/AuthContext";

import {
    getEmployeeSanitizationRequests,
} from "../../../services/sanitizationRequestService";

function WorkstationEmployeeDashboard() {
    const { user } = useAuth();

    const [requests, setRequests] =
        useState([]);

    const [loading, setLoading] =
        useState(true);

    const [error, setError] =
        useState("");

    const [search, setSearch] =
        useState("");

    const [statusFilter, setStatusFilter] =
        useState("ALL");

    const loadRequests =
        async () => {
            try {
                setLoading(true);
                setError("");

                const data =
                    await getEmployeeSanitizationRequests();

                setRequests(
                    Array.isArray(data)
                        ? data
                        : []
                );
            } catch (err) {
                console.error(
                    "Failed to load employee requests:",
                    err
                );

                setError(
                    err.message ||
                    "Failed to load assigned requests."
                );
            } finally {
                setLoading(false);
            }
        };

    useEffect(() => {
        loadRequests();
    }, []);

    const counts =
        useMemo(() => {
            const count = (
                status
            ) =>
                requests.filter(
                    (request) =>
                        request.status ===
                        status
                ).length;

            return {
                total: requests.length,
                assigned:
                    count("ASSIGNED"),
                inProgress:
                    count("IN_PROGRESS"),
                verifying:
                    count("VERIFYING"),
                completed:
                    count("COMPLETED"),
                failed:
                    count("FAILED"),
            };
        }, [requests]);

    const filteredRequests =
        useMemo(() => {
            const normalized =
                search
                    .trim()
                    .toLowerCase();

            return requests.filter(
                (request) => {
                    const matchesStatus =
                        statusFilter ===
                            "ALL" ||
                        request.status ===
                            statusFilter;

                    if (!normalized) {
                        return matchesStatus;
                    }

                    const searchable =
                        [
                            request.requestId,
                            request.deviceType,
                            request.capacity,
                            request.assetIdentifier,
                            request.sanitizationMethod,
                            request.status,
                            request.assignedWorkstation
                                ?.workstationId,
                            request.assignedWorkstation
                                ?.name,
                        ]
                            .filter(Boolean)
                            .join(" ")
                            .toLowerCase();

                    return (
                        matchesStatus &&
                        searchable.includes(
                            normalized
                        )
                    );
                }
            );
        }, [
            requests,
            search,
            statusFilter,
        ]);

    return (
        <div className="space-y-6">

            <div className="flex flex-col gap-4 lg:flex-row lg:items-end lg:justify-between">

                <div>
                    <p className="text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                        Workstation Operations
                    </p>

                    <h1 className="mt-2 text-2xl font-semibold tracking-tight text-slate-900">
                        Employee Dashboard
                    </h1>

                    <p className="mt-1 text-sm leading-6 text-slate-500">
                        Welcome, {user?.name || "Employee"}. Manage the
                        sanitization jobs assigned to your workstation.
                    </p>
                </div>

                <div className="flex flex-wrap gap-2">

                    <Link
                        to="/workstation-employee/sanitization/history"
                        className="inline-flex items-center justify-center rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                    >
                        View History
                    </Link>

                    <button
                        type="button"
                        onClick={loadRequests}
                        className="inline-flex items-center justify-center rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                    >
                        Refresh
                    </button>

                </div>

            </div>

            <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-5">

                <Metric
                    label="Total"
                    value={counts.total}
                    description="Assigned jobs"
                />

                <Metric
                    label="Ready"
                    value={counts.assigned}
                    description="Ready to start"
                />

                <Metric
                    label="In Progress"
                    value={counts.inProgress}
                    description="Execution active"
                    tone="info"
                />

                <Metric
                    label="Verifying"
                    value={counts.verifying}
                    description="Verification stage"
                    tone="warning"
                />

                <Metric
                    label="Completed"
                    value={counts.completed}
                    description="Completed jobs"
                    tone="success"
                />

            </div>

            {loading && (
                <div className="flex min-h-[300px] items-center justify-center">
                    <div className="w-full max-w-md rounded-2xl border border-slate-200 bg-white p-8 text-center shadow-sm">

                        <div className="mx-auto flex h-12 w-12 items-center justify-center rounded-2xl bg-indigo-50">
                            <div className="h-5 w-5 animate-spin rounded-full border-2 border-indigo-600 border-t-transparent" />
                        </div>

                        <h2 className="mt-5 text-base font-semibold text-slate-900">
                            Loading assigned jobs
                        </h2>

                        <p className="mt-2 text-sm leading-6 text-slate-500">
                            Fetching operational jobs assigned to your account.
                        </p>

                    </div>
                </div>
            )}

            {!loading && error && (
                <div className="rounded-2xl border border-red-200 bg-red-50 p-5">
                    <p className="text-sm font-medium text-red-700">
                        {error}
                    </p>

                    <button
                        type="button"
                        onClick={loadRequests}
                        className="mt-4 rounded-lg border border-red-200 bg-white px-4 py-2 text-sm font-medium text-red-700 hover:bg-red-50"
                    >
                        Try Again
                    </button>
                </div>
            )}

            {!loading &&
                !error && (
                    <section className="overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-sm">

                        <div className="border-b border-slate-200 p-5">

                            <div className="flex flex-col gap-4 xl:flex-row xl:items-end xl:justify-between">

                                <div>
                                    <h2 className="text-base font-semibold text-slate-900">
                                        Assigned Sanitization Jobs
                                    </h2>

                                    <p className="mt-1 text-sm text-slate-500">
                                        Open a job to inspect its request details,
                                        execution pipeline and result state.
                                    </p>
                                </div>

                                <div className="flex flex-col gap-2 sm:flex-row">

                                    <input
                                        type="text"
                                        value={search}
                                        onChange={(event) =>
                                            setSearch(
                                                event.target.value
                                            )
                                        }
                                        placeholder="Search request, device..."
                                        className="w-full rounded-lg border border-slate-300 bg-white px-3 py-2.5 text-sm text-slate-800 outline-none placeholder:text-slate-400 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 sm:w-64"
                                    />

                                    <select
                                        value={
                                            statusFilter
                                        }
                                        onChange={(event) =>
                                            setStatusFilter(
                                                event.target.value
                                            )
                                        }
                                        className="rounded-lg border border-slate-300 bg-white px-3 py-2.5 text-sm font-medium text-slate-700 outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100"
                                    >
                                        <option value="ALL">
                                            All Statuses
                                        </option>

                                        <option value="ASSIGNED">
                                            Assigned
                                        </option>

                                        <option value="IN_PROGRESS">
                                            In Progress
                                        </option>

                                        <option value="VERIFYING">
                                            Verifying
                                        </option>

                                        <option value="COMPLETED">
                                            Completed
                                        </option>

                                        <option value="FAILED">
                                            Failed
                                        </option>
                                    </select>

                                </div>

                            </div>

                            <p className="mt-4 text-xs text-slate-400">
                                Showing{" "}
                                <span className="font-semibold text-slate-600">
                                    {
                                        filteredRequests.length
                                    }
                                </span>{" "}
                                of{" "}
                                <span className="font-semibold text-slate-600">
                                    {
                                        requests.length
                                    }
                                </span>{" "}
                                assigned jobs
                            </p>

                        </div>

                        {filteredRequests.length === 0 ? (
                            <div className="p-10 text-center">

                                <div className="mx-auto flex h-14 w-14 items-center justify-center rounded-2xl bg-slate-100 text-xl text-slate-400">
                                    ⌕
                                </div>

                                <h3 className="mt-4 text-sm font-semibold text-slate-900">
                                    No matching jobs
                                </h3>

                                <p className="mt-1 text-sm leading-6 text-slate-500">
                                    No jobs match the current search or status filter.
                                </p>

                            </div>
                        ) : (
                            <div className="overflow-x-auto">

                                <table className="min-w-[1050px] w-full">

                                    <thead className="border-b border-slate-200 bg-slate-50">

                                        <tr>

                                            <Th>
                                                Request
                                            </Th>

                                            <Th>
                                                Device
                                            </Th>

                                            <Th>
                                                Method
                                            </Th>

                                            <Th>
                                                Workstation
                                            </Th>

                                            <Th>
                                                Status
                                            </Th>

                                            <Th>
                                                Action
                                            </Th>

                                        </tr>

                                    </thead>

                                    <tbody className="divide-y divide-slate-100">

                                        {filteredRequests.map(
                                            (request) => (
                                                <tr
                                                    key={
                                                        request._id ||
                                                        request.requestId
                                                    }
                                                    className="transition hover:bg-slate-50"
                                                >

                                                    <td className="px-4 py-4">
                                                        <p className="font-medium text-slate-900">
                                                            {
                                                                request.requestId
                                                            }
                                                        </p>

                                                        <p className="mt-1 text-xs text-slate-400">
                                                            Assigned{" "}
                                                            {
                                                                formatDate(
                                                                    request.assignedAt
                                                                )
                                                            }
                                                        </p>
                                                    </td>

                                                    <td className="px-4 py-4">
                                                        <p className="font-medium text-slate-800">
                                                            {
                                                                request.deviceType ||
                                                                "N/A"
                                                            }
                                                        </p>

                                                        <p className="mt-1 text-xs text-slate-500">
                                                            {
                                                                request.capacity ||
                                                                "Capacity unavailable"
                                                            }
                                                        </p>

                                                        {request.assetIdentifier && (
                                                            <p className="mt-1 text-xs text-slate-400">
                                                                Asset:{" "}
                                                                {
                                                                    request.assetIdentifier
                                                                }
                                                            </p>
                                                        )}
                                                    </td>

                                                    <td className="px-4 py-4 text-sm text-slate-700">
                                                        {formatMethod(
                                                            request.sanitizationMethod
                                                        )}
                                                    </td>

                                                    <td className="px-4 py-4">
                                                        <p className="text-sm font-medium text-slate-800">
                                                            {
                                                                request.assignedWorkstation
                                                                    ?.name ||
                                                                "N/A"
                                                            }
                                                        </p>

                                                        <p className="mt-1 font-mono text-xs text-slate-400">
                                                            {
                                                                request
                                                                    .assignedWorkstation
                                                                    ?.workstationId ||
                                                                "N/A"
                                                            }
                                                        </p>
                                                    </td>

                                                    <td className="px-4 py-4">
                                                        <StatusBadge
                                                            status={
                                                                request.status
                                                            }
                                                        />
                                                    </td>

                                                    <td className="px-4 py-4">
                                                        <Link
                                                            to={`/workstation-employee/sanitization/${request.requestId}`}
                                                            className="inline-flex rounded-lg bg-indigo-600 px-3.5 py-2 text-xs font-semibold text-white transition hover:bg-indigo-700"
                                                        >
                                                            Open Job
                                                        </Link>
                                                    </td>

                                                </tr>
                                            )
                                        )}

                                    </tbody>

                                </table>

                            </div>
                        )}

                    </section>
                )}

            <div className="rounded-2xl border border-slate-200 bg-slate-50 p-5">

                <div className="flex flex-col gap-2 sm:flex-row sm:items-start sm:justify-between">
                    <div>
                        <p className="text-sm font-semibold text-slate-800">
                            Operational workflow
                        </p>

                        <p className="mt-1 text-sm leading-6 text-slate-500">
                            Assigned jobs move through execution, verification
                            and final completion using backend state.
                        </p>
                    </div>

                    <div className="shrink-0 rounded-full border border-slate-200 bg-white px-3 py-1 text-xs font-medium text-slate-500">
                        {counts.failed} Failed
                    </div>
                </div>

            </div>

        </div>
    );
}

function Metric({
    label,
    value,
    description,
    tone = "default",
}) {
    const styles = {
        default:
            "bg-slate-100 text-slate-600",
        info:
            "bg-indigo-100 text-indigo-700",
        warning:
            "bg-amber-100 text-amber-700",
        success:
            "bg-green-100 text-green-700",
    };

    return (
        <div className="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">

            <div className="flex items-start justify-between gap-3">

                <div>
                    <p className="text-sm font-medium text-slate-500">
                        {label}
                    </p>

                    <p className="mt-2 text-2xl font-semibold tracking-tight text-slate-900">
                        {value}
                    </p>

                    <p className="mt-1 text-xs text-slate-400">
                        {description}
                    </p>
                </div>

                <div
                    className={`flex h-9 w-9 items-center justify-center rounded-lg text-sm font-semibold ${
                        styles[tone] ||
                        styles.default
                    }`}
                >
                    {tone === "success"
                        ? "✓"
                        : tone === "warning"
                            ? "!"
                            : tone === "info"
                                ? "•"
                                : "#"}
                </div>

            </div>

        </div>
    );
}

function Th({ children }) {
    return (
        <th className="px-4 py-3 text-left text-[11px] font-semibold uppercase tracking-[0.08em] text-slate-500">
            {children}
        </th>
    );
}

function StatusBadge({
    status,
}) {
    const styles = {
        ASSIGNED:
            "border-slate-200 bg-slate-100 text-slate-700",
        IN_PROGRESS:
            "border-indigo-200 bg-indigo-50 text-indigo-700",
        VERIFYING:
            "border-sky-200 bg-sky-50 text-sky-700",
        COMPLETED:
            "border-green-200 bg-green-50 text-green-700",
        FAILED:
            "border-red-200 bg-red-50 text-red-700",
    };

    const labels = {
        ASSIGNED: "Assigned",
        IN_PROGRESS: "In Progress",
        VERIFYING: "Verifying",
        COMPLETED: "Completed",
        FAILED: "Failed",
    };

    return (
        <span
            className={`inline-flex items-center gap-1.5 rounded-full border px-3 py-1 text-xs font-semibold ${
                styles[status] ||
                "border-slate-200 bg-slate-100 text-slate-700"
            }`}
        >
            <span className="h-1.5 w-1.5 rounded-full bg-current" />

            {labels[status] ||
                status ||
                "Unknown"}
        </span>
    );
}

function formatMethod(method) {
    if (!method) {
        return "—";
    }

    return String(method)
        .replaceAll("_", " ")
        .replace(
            /\b\w/g,
            (character) =>
                character.toUpperCase()
        );
}

function formatDate(value) {
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
            dateStyle: "medium",
            timeStyle: "short",
        }
    );
}

export default WorkstationEmployeeDashboard;