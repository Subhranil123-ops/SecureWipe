import { useEffect, useMemo, useState } from "react";
import toast from "react-hot-toast";

import {
    getAllHeadSanitizationRequests,
} from "../../../services/sanitizationRequestService";

function WorkstationHeadSanitizationRequests() {
    const [requests, setRequests] = useState([]);
    const [loading, setLoading] = useState(true);
    const [search, setSearch] = useState("");
    const [statusFilter, setStatusFilter] = useState("ALL");

    const loadRequests = async () => {
        try {
            setLoading(true);

            const data =
                await getAllHeadSanitizationRequests();

            setRequests(
                Array.isArray(data)
                    ? data
                    : []
            );
        } catch (error) {
            console.error(
                "Failed to load sanitization requests:",
                error
            );

            toast.error(
                error.message ||
                "Unable to load sanitization requests"
            );
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        loadRequests();
    }, []);

    const counts = useMemo(() => {
        const count = (status) =>
            requests.filter(
                (request) =>
                    request.status === status
            ).length;

        return {
            total: requests.length,
            pending: count("PENDING"),
            approved: count("APPROVED"),
            assigned: count("ASSIGNED"),
            active:
                count("IN_PROGRESS") +
                count("VERIFYING"),
            completed: count("COMPLETED"),
            failed: count("FAILED"),
        };
    }, [requests]);

    const filteredRequests = useMemo(() => {
        const searchText =
            search
                .trim()
                .toLowerCase();

        return requests.filter(
            (request) => {
                const matchesStatus =
                    statusFilter === "ALL" ||
                    request.status ===
                        statusFilter;

                if (!searchText) {
                    return matchesStatus;
                }

                const searchableText = [
                    request.requestId,
                    request.name,
                    request.email,
                    request.phone,
                    request.deviceType,
                    request.capacity,
                    request.assetIdentifier,
                    request.serialNumber,
                    request.status,
                    request.assignedEmployee?.name,
                    request.assignedWorkstation?.name,
                    request.assignedWorkstation?.workstationId,
                ]
                    .filter(Boolean)
                    .join(" ")
                    .toLowerCase();

                return (
                    matchesStatus &&
                    searchableText.includes(
                        searchText
                    )
                );
            }
        );
    }, [
        requests,
        search,
        statusFilter,
    ]);

    const clearFilters = () => {
        setSearch("");
        setStatusFilter("ALL");
    };

    if (loading) {
        return (
            <div className="flex min-h-[420px] items-center justify-center">
                <div className="w-full max-w-md rounded-2xl border border-slate-200 bg-white p-8 text-center shadow-sm">
                    <div className="mx-auto flex h-12 w-12 items-center justify-center rounded-2xl bg-indigo-50">
                        <div className="h-5 w-5 animate-spin rounded-full border-2 border-indigo-600 border-t-transparent" />
                    </div>

                    <h2 className="mt-5 text-base font-semibold text-slate-900">
                        Loading sanitization requests
                    </h2>

                    <p className="mt-2 text-sm leading-6 text-slate-500">
                        Fetching requests associated with your
                        workstation centre.
                    </p>
                </div>
            </div>
        );
    }

    return (
        <div className="space-y-6">
            <div className="flex flex-col gap-4 lg:flex-row lg:items-end lg:justify-between">
                <div>
                    <div className="flex items-center gap-2 text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                        <span className="h-1.5 w-1.5 rounded-full bg-indigo-600" />
                        Operations
                    </div>

                    <h1 className="mt-2 text-2xl font-semibold tracking-tight text-slate-900">
                        Sanitization Requests
                    </h1>

                    <p className="mt-1 max-w-2xl text-sm leading-6 text-slate-500">
                        Monitor the sanitization request lifecycle
                        across your workstation centre.
                    </p>
                </div>

                <button
                    type="button"
                    onClick={loadRequests}
                    disabled={loading}
                    className="inline-flex w-fit items-center gap-2 rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50 disabled:cursor-not-allowed disabled:opacity-50"
                >
                    <span>↻</span>
                    Refresh
                </button>
            </div>

            <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">
                <Metric
                    label="Total Requests"
                    value={counts.total}
                    description="Centre-wide requests"
                />

                <Metric
                    label="Pending Review"
                    value={counts.pending}
                    description="Awaiting review"
                    tone="warning"
                />

                <Metric
                    label="Active Jobs"
                    value={counts.active}
                    description="In progress or verifying"
                    tone="info"
                />

                <Metric
                    label="Completed"
                    value={counts.completed}
                    description="Completed records"
                    tone="success"
                />
            </div>

            <section className="overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-sm">
                <div className="border-b border-slate-200 p-5">
                    <div className="flex flex-col gap-4 xl:flex-row xl:items-end xl:justify-between">
                        <div>
                            <h2 className="text-base font-semibold text-slate-900">
                                Request Registry
                            </h2>

                            <p className="mt-1 text-sm text-slate-500">
                                Search and review the operational state of
                                sanitization requests.
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
                                    placeholder="Search request, customer, device..."
                                    className="w-full rounded-lg border border-slate-300 bg-white py-2.5 pl-9 pr-3 text-sm text-slate-800 outline-none placeholder:text-slate-400 focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 sm:w-80"
                                />
                            </div>

                            <select
                                value={statusFilter}
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
                                <option value="PENDING">
                                    Pending
                                </option>
                                <option value="APPROVED">
                                    Approved
                                </option>
                                <option value="REJECTED">
                                    Rejected
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
                                <option value="CANCELLED">
                                    Cancelled
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
                            requests
                        </span>

                        {(search || statusFilter !== "ALL") && (
                            <button
                                type="button"
                                onClick={clearFilters}
                                className="rounded-full border border-slate-200 bg-slate-50 px-3 py-1 font-medium text-slate-600 hover:bg-slate-100"
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
                            No matching requests
                        </h3>

                        <p className="mt-1 text-sm text-slate-500">
                            Try another search term or status filter.
                        </p>

                        {(search ||
                            statusFilter !== "ALL") && (
                            <button
                                type="button"
                                onClick={clearFilters}
                                className="mt-4 rounded-lg border border-slate-300 bg-white px-4 py-2 text-sm font-medium text-slate-700 hover:bg-slate-50"
                            >
                                Reset filters
                            </button>
                        )}
                    </div>
                ) : (
                    <div className="overflow-x-auto">
                        <table className="min-w-[1120px] w-full text-sm">
                            <thead className="border-b border-slate-200 bg-slate-50">
                                <tr>
                                    <Th>Request</Th>
                                    <Th>Customer</Th>
                                    <Th>Device</Th>
                                    <Th>Serial Number</Th>
                                    <Th>Status</Th>
                                    <Th>Assignment</Th>
                                </tr>
                            </thead>

                            <tbody className="divide-y divide-slate-100">
                                {filteredRequests.map(
                                    (request) => (
                                        <tr
                                            key={
                                                request.requestId ||
                                                request._id
                                            }
                                            className="transition hover:bg-slate-50"
                                        >
                                            <td className="px-4 py-4">
                                                <p className="font-medium text-slate-900">
                                                    {request.requestId ||
                                                        "—"}
                                                </p>

                                                <p className="mt-1 text-xs text-slate-400">
                                                    {formatDate(
                                                        request.createdAt
                                                    )}
                                                </p>
                                            </td>

                                            <td className="px-4 py-4">
                                                <p className="font-medium text-slate-800">
                                                    {request.name ||
                                                        request.customer
                                                            ?.name ||
                                                        "N/A"}
                                                </p>

                                                <p className="mt-1 text-xs text-slate-500">
                                                    {request.email ||
                                                        request.customer
                                                            ?.email ||
                                                        "N/A"}
                                                </p>

                                                <p className="mt-1 text-xs text-slate-500">
                                                    {request.phone ||
                                                        request.customer
                                                            ?.phone ||
                                                        "N/A"}
                                                </p>
                                            </td>

                                            <td className="px-4 py-4">
                                                <p className="font-medium text-slate-800">
                                                    {request.deviceType ||
                                                        "N/A"}
                                                </p>

                                                <p className="mt-1 text-xs text-slate-500">
                                                    {request.capacity ||
                                                        "Capacity unavailable"}
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

                                            <td className="px-4 py-4">
                                                <span className="inline-flex rounded-md bg-slate-100 px-2.5 py-1 text-xs font-medium text-slate-700">
                                                    {request.serialNumber || "N/A"}
                                                </span>
                                            </td>

                                            <td className="px-4 py-4">
                                                <StatusBadge
                                                    status={
                                                        request.status
                                                    }
                                                />
                                            </td>

                                            <td className="px-4 py-4">
                                                {request.assignedEmployee ||
                                                request.assignedWorkstation ? (
                                                    <div className="space-y-1">
                                                        <p className="text-sm font-medium text-slate-800">
                                                            {request
                                                                .assignedEmployee
                                                                ?.name ||
                                                                "Employee assigned"}
                                                        </p>

                                                        <p className="text-xs text-slate-500">
                                                            {request
                                                                .assignedWorkstation
                                                                ?.name ||
                                                                request
                                                                    .assignedWorkstation
                                                                    ?.workstationId ||
                                                                "Workstation assigned"}
                                                        </p>

                                                        {request.assignedAt && (
                                                            <p className="text-xs text-slate-400">
                                                                {formatDate(
                                                                    request.assignedAt
                                                                )}
                                                            </p>
                                                        )}
                                                    </div>
                                                ) : (
                                                    <span className="text-sm text-slate-400">
                                                        Not assigned
                                                    </span>
                                                )}
                                            </td>
                                        </tr>
                                    )
                                )}
                            </tbody>
                        </table>
                    </div>
                )}
            </section>

            <div className="grid gap-4 sm:grid-cols-3">
                <MiniMetric
                    label="Awaiting Assignment"
                    value={counts.approved}
                />

                <MiniMetric
                    label="Assigned"
                    value={counts.assigned}
                />

                <MiniMetric
                    label="Failed"
                    value={counts.failed}
                />
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
    const iconStyles = {
        default: "bg-slate-100 text-slate-600",
        warning: "bg-amber-100 text-amber-700",
        info: "bg-blue-100 text-blue-700",
        success: "bg-green-100 text-green-700",
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
                        iconStyles[tone] ||
                        iconStyles.default
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

function MiniMetric({
    label,
    value,
}) {
    return (
        <div className="rounded-xl border border-slate-200 bg-white px-4 py-3">
            <p className="text-xs font-medium text-slate-400">
                {label}
            </p>

            <p className="mt-1 text-lg font-semibold text-slate-800">
                {value}
            </p>
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
        PENDING:
            "border-amber-200 bg-amber-50 text-amber-700",
        APPROVED:
            "border-blue-200 bg-blue-50 text-blue-700",
        REJECTED:
            "border-red-200 bg-red-50 text-red-700",
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
        CANCELLED:
            "border-slate-200 bg-slate-100 text-slate-600",
    };

    const labels = {
        PENDING: "Pending",
        APPROVED: "Approved",
        REJECTED: "Rejected",
        ASSIGNED: "Assigned",
        IN_PROGRESS: "In Progress",
        VERIFYING: "Verifying",
        COMPLETED: "Completed",
        FAILED: "Failed",
        CANCELLED: "Cancelled",
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

export default WorkstationHeadSanitizationRequests;