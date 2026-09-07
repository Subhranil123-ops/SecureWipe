import { useEffect, useMemo, useState } from "react";
import { Link } from "react-router-dom";

import {
    getEmployeeSanitizationRequests,
} from "../../../services/sanitizationRequestService";

function SanitizationHistory() {
    const [requests, setRequests] = useState([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState("");
    const [search, setSearch] = useState("");
    const [statusFilter, setStatusFilter] = useState("ALL");

    const loadRequests = async () => {
        try {
            setLoading(true);
            setError("");

            const data = await getEmployeeSanitizationRequests();

            setRequests(Array.isArray(data) ? data : []);
        } catch (err) {
            console.error(
                "Failed to load sanitization history:",
                err
            );

            setError(
                err.message ||
                "Failed to load sanitization history."
            );
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        loadRequests();
    }, []);

    const counts = useMemo(() => {
        return {
            total: requests.length,
            assigned: requests.filter(
                (request) => request.status === "ASSIGNED"
            ).length,
            inProgress: requests.filter(
                (request) => request.status === "IN_PROGRESS"
            ).length,
            verifying: requests.filter(
                (request) => request.status === "VERIFYING"
            ).length,
            completed: requests.filter(
                (request) => request.status === "COMPLETED"
            ).length,
            failed: requests.filter(
                (request) => request.status === "FAILED"
            ).length,
        };
    }, [requests]);

    const filteredRequests = useMemo(() => {
        const normalizedSearch = search.trim().toLowerCase();

        return requests.filter((request) => {
            const matchesStatus =
                statusFilter === "ALL" ||
                request.status === statusFilter;

            if (!normalizedSearch) {
                return matchesStatus;
            }

            const searchableText = [
                request.requestId,
                request.deviceType,
                request.sanitizationMethod,
                request.assetIdentifier,
                request.status,
                request.workstationCenter?.name,
                request.assignedWorkstation?.name,
            ]
                .filter(Boolean)
                .join(" ")
                .toLowerCase();

            return (
                matchesStatus &&
                searchableText.includes(normalizedSearch)
            );
        });
    }, [requests, search, statusFilter]);

    const hasFilters =
        search.trim() !== "" ||
        statusFilter !== "ALL";

    const clearFilters = () => {
        setSearch("");
        setStatusFilter("ALL");
    };

    return (
        <div className="space-y-6">
            <div className="flex flex-col gap-4 lg:flex-row lg:items-end lg:justify-between">
                <div>
                    <div className="flex items-center gap-2 text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                        <span className="h-1.5 w-1.5 rounded-full bg-indigo-600" />
                        Sanitization
                    </div>

                    <h1 className="mt-2 text-2xl font-semibold tracking-tight text-slate-900">
                        Sanitization History
                    </h1>

                    <p className="mt-1 max-w-2xl text-sm leading-6 text-slate-500">
                        Review assigned jobs, execution progress,
                        verification state, and completed sanitization records.
                    </p>
                </div>

                <button
                    type="button"
                    onClick={loadRequests}
                    disabled={loading}
                    className="inline-flex w-fit items-center justify-center gap-2 rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50 disabled:cursor-not-allowed disabled:opacity-50"
                >
                    <span className={loading ? "animate-spin" : ""}>
                        ↻
                    </span>

                    {loading ? "Refreshing..." : "Refresh"}
                </button>
            </div>

            <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
                <Metric
                    label="Total Jobs"
                    value={counts.total}
                    description="All assigned requests"
                />

                <Metric
                    label="Active"
                    value={
                        counts.assigned +
                        counts.inProgress +
                        counts.verifying
                    }
                    description="Jobs still in workflow"
                />

                <Metric
                    label="Completed"
                    value={counts.completed}
                    description="Successfully completed"
                    tone="success"
                />

                <Metric
                    label="Failed"
                    value={counts.failed}
                    description="Requires review"
                    tone="danger"
                />
            </div>

            {error && (
                <div className="rounded-2xl border border-red-200 bg-red-50 p-5">
                    <div className="flex items-start gap-3">
                        <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-full bg-red-100 font-bold text-red-700">
                            !
                        </div>

                        <div>
                            <p className="text-sm font-semibold text-red-900">
                                Unable to load sanitization history
                            </p>

                            <p className="mt-1 text-sm leading-6 text-red-700">
                                {error}
                            </p>

                            <button
                                type="button"
                                onClick={loadRequests}
                                className="mt-3 text-sm font-semibold text-red-800 underline underline-offset-2 hover:text-red-950"
                            >
                                Try again
                            </button>
                        </div>
                    </div>
                </div>
            )}

            {!loading && !error && requests.length === 0 && (
                <div className="rounded-2xl border border-slate-200 bg-white p-10 text-center shadow-sm">
                    <div className="mx-auto flex h-14 w-14 items-center justify-center rounded-2xl bg-slate-100 text-2xl text-slate-500">
                        ✓
                    </div>

                    <h2 className="mt-5 text-base font-semibold text-slate-900">
                        No sanitization jobs yet
                    </h2>

                    <p className="mx-auto mt-2 max-w-md text-sm leading-6 text-slate-500">
                        No sanitization requests have been assigned to your
                        workstation account.
                    </p>
                </div>
            )}

            {!loading && !error && requests.length > 0 && (
                <section className="overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-sm">
                    <div className="border-b border-slate-200 px-5 py-5">
                        <div className="flex flex-col gap-4 xl:flex-row xl:items-end xl:justify-between">
                            <div>
                                <h2 className="text-base font-semibold text-slate-900">
                                    Job History
                                </h2>

                                <p className="mt-1 text-sm text-slate-500">
                                    Every assigned request remains traceable
                                    throughout its operational lifecycle.
                                </p>
                            </div>

                            <div className="flex flex-col gap-2 sm:flex-row">
                                <div className="relative">
                                    <span className="pointer-events-none absolute inset-y-0 left-3 flex items-center text-slate-400">
                                        ⌕
                                    </span>

                                    <input
                                        type="text"
                                        value={search}
                                        onChange={(event) =>
                                            setSearch(
                                                event.target.value
                                            )
                                        }
                                        placeholder="Search request, device, asset..."
                                        className="w-full rounded-lg border border-slate-300 bg-white py-2.5 pl-9 pr-3 text-sm text-slate-800 outline-none transition placeholder:text-slate-400 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 sm:w-72"
                                    />
                                </div>

                                <select
                                    value={statusFilter}
                                    onChange={(event) =>
                                        setStatusFilter(
                                            event.target.value
                                        )
                                    }
                                    className="rounded-lg border border-slate-300 bg-white px-3 py-2.5 text-sm font-medium text-slate-700 outline-none transition focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100"
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

                        <div className="mt-4 flex flex-wrap items-center gap-2 text-xs text-slate-500">
                            <span>
                                Showing{" "}
                                <span className="font-semibold text-slate-700">
                                    {filteredRequests.length}
                                </span>{" "}
                                of{" "}
                                <span className="font-semibold text-slate-700">
                                    {requests.length}
                                </span>{" "}
                                jobs
                            </span>

                            {hasFilters && (
                                <button
                                    type="button"
                                    onClick={clearFilters}
                                    className="rounded-full border border-slate-200 bg-slate-50 px-3 py-1 font-medium text-slate-600 transition hover:bg-slate-100 hover:text-slate-800"
                                >
                                    Clear filters
                                </button>
                            )}
                        </div>
                    </div>

                    {filteredRequests.length === 0 ? (
                        <div className="p-10 text-center">
                            <div className="mx-auto flex h-12 w-12 items-center justify-center rounded-xl bg-slate-100 text-xl text-slate-400">
                                ⌕
                            </div>

                            <h3 className="mt-4 text-sm font-semibold text-slate-900">
                                No matching jobs
                            </h3>

                            <p className="mt-1 text-sm text-slate-500">
                                Try a different search term or status filter.
                            </p>

                            <button
                                type="button"
                                onClick={clearFilters}
                                className="mt-4 rounded-lg border border-slate-300 bg-white px-4 py-2 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                            >
                                Reset Filters
                            </button>
                        </div>
                    ) : (
                        <div className="overflow-x-auto">
                            <table className="min-w-[920px] w-full">
                                <thead className="border-b border-slate-200 bg-slate-50">
                                    <tr>
                                        <Th>Request</Th>
                                        <Th>Device</Th>
                                        <Th>Method</Th>
                                        <Th>Status</Th>
                                        <Th>Assigned At</Th>
                                        <Th>Action</Th>
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
                                                className="group transition hover:bg-slate-50"
                                            >
                                                <td className="px-4 py-4">
                                                    <div>
                                                        <p className="font-medium text-slate-900">
                                                            {request.requestId ||
                                                                "—"}
                                                        </p>

                                                        {request.assetIdentifier && (
                                                            <p className="mt-1 text-xs text-slate-400">
                                                                Asset:{" "}
                                                                {
                                                                    request.assetIdentifier
                                                                }
                                                            </p>
                                                        )}
                                                    </div>
                                                </td>

                                                <td className="px-4 py-4">
                                                    <p className="text-sm font-medium text-slate-700">
                                                        {request.deviceType ||
                                                            "—"}
                                                    </p>

                                                    {request.capacity && (
                                                        <p className="mt-1 text-xs text-slate-400">
                                                            {
                                                                request.capacity
                                                            }
                                                        </p>
                                                    )}
                                                </td>

                                                <td className="px-4 py-4">
                                                    <span className="inline-flex rounded-md bg-slate-100 px-2.5 py-1 text-xs font-medium text-slate-700">
                                                        {formatMethod(
                                                            request.sanitizationMethod
                                                        )}
                                                    </span>
                                                </td>

                                                <td className="px-4 py-4">
                                                    <StatusBadge
                                                        status={
                                                            request.status
                                                        }
                                                    />
                                                </td>

                                                <td className="px-4 py-4 text-sm text-slate-600">
                                                    {formatDate(
                                                        request.assignedAt
                                                    )}
                                                </td>

                                                <td className="px-4 py-4">
                                                    <Link
                                                        to={`/workstation-employee/sanitization/${request.requestId}`}
                                                        className="inline-flex items-center gap-1 rounded-lg border border-slate-200 bg-white px-3 py-2 text-sm font-medium text-indigo-600 transition hover:border-indigo-200 hover:bg-indigo-50 hover:text-indigo-800"
                                                    >
                                                        Open
                                                        <span>
                                                            →
                                                        </span>
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
        </div>
    );
}

function Metric({
    label,
    value,
    description,
    tone = "default",
}) {
    const iconStyles = {
        default: "bg-slate-100 text-slate-600",
        success: "bg-green-100 text-green-700",
        danger: "bg-red-100 text-red-700",
    };

    return (
        <div className="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
            <div className="flex items-start justify-between gap-4">
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
                        iconStyles[tone]
                    }`}
                >
                    {tone === "success"
                        ? "✓"
                        : tone === "danger"
                            ? "!"
                            : "•"}
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

    const labels = {
        ASSIGNED: "Assigned",
        IN_PROGRESS: "In Progress",
        VERIFYING: "Verifying",
        COMPLETED: "Completed",
        FAILED: "Failed",
    };

    return (
        <span
            className={`inline-flex items-center rounded-full border px-3 py-1 text-xs font-semibold ${
                styles[status] ||
                "border-slate-200 bg-slate-100 text-slate-700"
            }`}
        >
            <span className="mr-1.5 h-1.5 w-1.5 rounded-full bg-current" />
            {labels[status] || status || "Unknown"}
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
            (character) => character.toUpperCase()
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

    return date.toLocaleString(
        undefined,
        {
            dateStyle: "medium",
            timeStyle: "short",
        }
    );
}

export default SanitizationHistory;