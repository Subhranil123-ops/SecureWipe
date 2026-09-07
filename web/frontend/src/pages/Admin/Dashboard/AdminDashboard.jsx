import { useEffect, useMemo, useState } from "react";

import {
    getAllUsers,
} from "../../../services/userService";

import {
    getAllSanitizationRequests,
} from "../../../services/sanitizationRequestService";

function AdminDashboard() {
    const [users, setUsers] =
        useState([]);

    const [sanitizationRequests, setSanitizationRequests] =
        useState([]);

    const [loading, setLoading] =
        useState(true);

    const [error, setError] =
        useState("");

    const loadDashboard =
        async () => {
            try {
                setLoading(true);
                setError("");

                const [
                    usersResponse,
                    requestsResponse,
                ] = await Promise.all([
                    getAllUsers(),
                    getAllSanitizationRequests(),
                ]);

                const userList =
                    Array.isArray(
                        usersResponse
                    )
                        ? usersResponse
                        : usersResponse?.users ||
                          usersResponse?.data ||
                          [];

                const requestList =
                    Array.isArray(
                        requestsResponse
                    )
                        ? requestsResponse
                        : [];

                setUsers(
                    userList
                );

                setSanitizationRequests(
                    requestList
                );
            } catch (error) {
                console.error(
                    "Failed to load admin dashboard:",
                    error
                );

                setError(
                    error.message ||
                    "Unable to load dashboard data."
                );
            } finally {
                setLoading(false);
            }
        };

    useEffect(() => {
        loadDashboard();
    }, []);

    const userStats =
        useMemo(() => {
            const active =
                users.filter(
                    (user) =>
                        user.status ===
                        "ACTIVE"
                ).length;

            const heads =
                users.filter(
                    (user) =>
                        user.role ===
                        "WORKSTATION_HEAD"
                ).length;

            const employees =
                users.filter(
                    (user) =>
                        user.role ===
                        "WORKSTATION_EMPLOYEE"
                ).length;

            const customers =
                users.filter(
                    (user) =>
                        user.role ===
                        "CUSTOMER"
                ).length;

            return {
                total: users.length,
                active,
                heads,
                employees,
                customers,
            };
        }, [users]);

    const requestStats =
        useMemo(() => {
            const count = (
                status
            ) =>
                sanitizationRequests.filter(
                    (request) =>
                        request.status ===
                        status
                ).length;

            return {
                total:
                    sanitizationRequests.length,
                pending:
                    count("PENDING"),
                approved:
                    count("APPROVED"),
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
                rejected:
                    count("REJECTED"),
            };
        }, [
            sanitizationRequests,
        ]);

    if (loading) {
        return (
            <div className="flex min-h-[420px] items-center justify-center">
                <div className="w-full max-w-md rounded-2xl border border-slate-200 bg-white p-8 text-center shadow-sm">

                    <div className="mx-auto flex h-12 w-12 items-center justify-center rounded-2xl bg-indigo-50">
                        <div className="h-5 w-5 animate-spin rounded-full border-2 border-indigo-600 border-t-transparent" />
                    </div>

                    <h2 className="mt-5 text-base font-semibold text-slate-900">
                        Loading admin dashboard
                    </h2>

                    <p className="mt-2 text-sm leading-6 text-slate-500">
                        Fetching current users and sanitization request data.
                    </p>

                </div>
            </div>
        );
    }

    if (error) {
        return (
            <div className="rounded-2xl border border-red-200 bg-red-50 p-6">
                <p className="text-sm font-medium text-red-700">
                    {error}
                </p>

                <button
                    type="button"
                    onClick={loadDashboard}
                    className="mt-4 rounded-lg border border-red-200 bg-white px-4 py-2 text-sm font-medium text-red-700 hover:bg-red-50"
                >
                    Try Again
                </button>
            </div>
        );
    }

    return (
        <div className="space-y-6">

            <div className="flex flex-col gap-4 lg:flex-row lg:items-end lg:justify-between">

                <div>
                    <p className="text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                        Administration
                    </p>

                    <h1 className="mt-2 text-2xl font-semibold tracking-tight text-slate-900">
                        Admin Dashboard
                    </h1>

                    <p className="mt-1 text-sm leading-6 text-slate-500">
                        Current system overview from backend user and
                        sanitization-request data.
                    </p>
                </div>

                <button
                    type="button"
                    onClick={loadDashboard}
                    className="w-fit rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 hover:bg-slate-50"
                >
                    Refresh
                </button>

            </div>

            <section>

                <div className="mb-3">
                    <h2 className="text-sm font-semibold uppercase tracking-[0.12em] text-slate-500">
                        User Overview
                    </h2>
                </div>

                <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-5">

                    <Metric
                        label="Total Users"
                        value={
                            userStats.total
                        }
                        description="Registered users"
                    />

                    <Metric
                        label="Active"
                        value={
                            userStats.active
                        }
                        description="Active accounts"
                        tone="success"
                    />

                    <Metric
                        label="Heads"
                        value={
                            userStats.heads
                        }
                        description="Workstation heads"
                    />

                    <Metric
                        label="Employees"
                        value={
                            userStats.employees
                        }
                        description="Workstation employees"
                        tone="info"
                    />

                    <Metric
                        label="Customers"
                        value={
                            userStats.customers
                        }
                        description="Customer accounts"
                    />

                </div>

            </section>

            <section>

                <div className="mb-3 flex items-center justify-between">

                    <div>
                        <h2 className="text-sm font-semibold uppercase tracking-[0.12em] text-slate-500">
                            Sanitization Operations
                        </h2>

                        <p className="mt-1 text-sm text-slate-400">
                            Current lifecycle distribution across requests.
                        </p>
                    </div>

                    <span className="rounded-full border border-slate-200 bg-white px-3 py-1 text-xs font-semibold text-slate-600">
                        {
                            requestStats.total
                        } Total
                    </span>

                </div>

                <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-5">

                    <Metric
                        label="Pending"
                        value={
                            requestStats.pending
                        }
                        description="Awaiting review"
                        tone="warning"
                    />

                    <Metric
                        label="Approved"
                        value={
                            requestStats.approved
                        }
                        description="Approved requests"
                    />

                    <Metric
                        label="Assigned"
                        value={
                            requestStats.assigned
                        }
                        description="Assigned jobs"
                        tone="info"
                    />

                    <Metric
                        label="Verifying"
                        value={
                            requestStats.verifying
                        }
                        description="Verification stage"
                        tone="warning"
                    />

                    <Metric
                        label="Completed"
                        value={
                            requestStats.completed
                        }
                        description="Completed requests"
                        tone="success"
                    />

                </div>

            </section>

            <section className="overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-sm">

                <div className="border-b border-slate-200 p-5">

                    <div className="flex flex-col gap-2 sm:flex-row sm:items-center sm:justify-between">

                        <div>
                            <h2 className="text-base font-semibold text-slate-900">
                                Recent Sanitization Requests
                            </h2>

                            <p className="mt-1 text-sm text-slate-500">
                                Backend-backed operational records available to administrators.
                            </p>
                        </div>

                        <div className="flex flex-wrap gap-2 text-xs">

                            <StatusSummary
                                label="Failed"
                                value={
                                    requestStats.failed
                                }
                            />

                            <StatusSummary
                                label="Rejected"
                                value={
                                    requestStats.rejected
                                }
                            />

                            <StatusSummary
                                label="In Progress"
                                value={
                                    requestStats.inProgress
                                }
                            />

                        </div>

                    </div>

                </div>

                {sanitizationRequests.length ===
                0 ? (
                    <div className="p-10 text-center">

                        <div className="mx-auto flex h-14 w-14 items-center justify-center rounded-2xl bg-slate-100 text-xl text-slate-400">
                            —
                        </div>

                        <h3 className="mt-4 text-sm font-semibold text-slate-900">
                            No sanitization requests
                        </h3>

                        <p className="mt-1 text-sm text-slate-500">
                            No request records are currently available.
                        </p>

                    </div>
                ) : (
                    <div className="overflow-x-auto">

                        <table className="min-w-[1150px] w-full">

                            <thead className="border-b border-slate-200 bg-slate-50">

                                <tr>

                                    <Th>
                                        Request
                                    </Th>

                                    <Th>
                                        Customer
                                    </Th>

                                    <Th>
                                        Centre
                                    </Th>

                                    <Th>
                                        Device
                                    </Th>

                                    <Th>
                                        Method
                                    </Th>

                                    <Th>
                                        Status
                                    </Th>

                                    <Th>
                                        Submitted
                                    </Th>

                                </tr>

                            </thead>

                            <tbody className="divide-y divide-slate-100">

                                {sanitizationRequests
                                    .slice(
                                        0,
                                        10
                                    )
                                    .map(
                                        (
                                            request
                                        ) => (
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
                                                        {
                                                            request.preferredDate
                                                                ? `Preferred: ${formatDate(
                                                                      request.preferredDate
                                                                  )}`
                                                                : "No preferred date"
                                                        }
                                                    </p>

                                                </td>

                                                <td className="px-4 py-4">

                                                    <p className="font-medium text-slate-800">
                                                        {
                                                            request.customer?.name ||
                                                            request.name ||
                                                            "N/A"
                                                        }
                                                    </p>

                                                    <p className="mt-1 text-xs text-slate-500">
                                                        {
                                                            request.customer?.email ||
                                                            request.email ||
                                                            "N/A"
                                                        }
                                                    </p>

                                                </td>

                                                <td className="px-4 py-4">

                                                    <p className="text-sm font-medium text-slate-800">
                                                        {
                                                            request.workstationCenter?.name ||
                                                            "N/A"
                                                        }
                                                    </p>

                                                    <p className="mt-1 font-mono text-xs text-slate-400">
                                                        {
                                                            request.workstationCenter?.centerId ||
                                                            "N/A"
                                                        }
                                                    </p>

                                                </td>

                                                <td className="px-4 py-4">

                                                    <p className="text-sm font-medium text-slate-800">
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

                                                </td>

                                                <td className="px-4 py-4 text-sm text-slate-700">
                                                    {formatMethod(
                                                        request.sanitizationMethod
                                                    )}
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
                                                        request.createdAt
                                                    )}
                                                </td>

                                            </tr>
                                        )
                                    )}

                            </tbody>

                        </table>

                    </div>
                )}

                {sanitizationRequests.length >
                    10 && (
                    <div className="border-t border-slate-200 bg-slate-50 px-5 py-3 text-xs text-slate-500">
                        Showing the 10 most recent records from{" "}
                        <span className="font-semibold text-slate-700">
                            {
                                sanitizationRequests.length
                            }
                        </span>{" "}
                        total requests.
                    </div>
                )}

            </section>

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
        warning:
            "bg-amber-100 text-amber-700",
        info:
            "bg-indigo-100 text-indigo-700",
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

function StatusSummary({
    label,
    value,
}) {
    return (
        <span className="rounded-full border border-slate-200 bg-slate-50 px-3 py-1 font-medium text-slate-500">
            {label}:{" "}
            <span className="font-semibold text-slate-700">
                {value}
            </span>
        </span>
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
                "border-slate-200 bg-slate-100 text-slate-600"
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

export default AdminDashboard;