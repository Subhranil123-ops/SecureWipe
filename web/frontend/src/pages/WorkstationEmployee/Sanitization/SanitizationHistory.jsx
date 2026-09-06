import { useEffect, useState } from "react";
import { Link } from "react-router-dom";

import {
    getEmployeeSanitizationRequests,
} from "../../../services/sanitizationRequestService";

function SanitizationHistory() {

    const [requests, setRequests] =
        useState([]);

    const [loading, setLoading] =
        useState(true);

    const [error, setError] =
        useState("");

    const loadRequests =
        async () => {

            try {

                setLoading(true);
                setError("");

                const data =
                    await getEmployeeSanitizationRequests();

                setRequests(
                    data || []
                );

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

    const completed =
        requests.filter(
            (request) =>
                request.status ===
                "COMPLETED"
        );

    const failed =
        requests.filter(
            (request) =>
                request.status ===
                "FAILED"
        );

    return (
        <div className="space-y-6">

            <div className="flex flex-col gap-2 sm:flex-row sm:items-end sm:justify-between">

                <div>
                    <p className="text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                        Sanitization
                    </p>

                    <h1 className="mt-1 text-2xl font-semibold text-slate-900">
                        Sanitization History
                    </h1>

                    <p className="mt-1 text-sm text-slate-500">
                        Review assigned sanitization jobs and their lifecycle state.
                    </p>
                </div>

                <button
                    type="button"
                    onClick={loadRequests}
                    className="w-fit rounded-lg border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 hover:bg-slate-50"
                >
                    Refresh
                </button>

            </div>

            <div className="grid gap-4 sm:grid-cols-3">

                <Metric
                    label="Total Jobs"
                    value={requests.length}
                />

                <Metric
                    label="Completed"
                    value={completed.length}
                />

                <Metric
                    label="Failed"
                    value={failed.length}
                />

            </div>

            {loading && (
                <div className="rounded-lg border border-slate-200 bg-white p-6 shadow-sm">
                    <p className="text-sm text-slate-500">
                        Loading sanitization history...
                    </p>
                </div>
            )}

            {!loading && error && (
                <div className="rounded-lg border border-red-200 bg-red-50 p-5">
                    <p className="text-sm text-red-700">
                        {error}
                    </p>
                </div>
            )}

            {!loading &&
                !error &&
                requests.length === 0 && (
                    <div className="rounded-lg border border-slate-200 bg-white p-6 shadow-sm">
                        <p className="text-sm text-slate-500">
                            No sanitization jobs have been assigned to you.
                        </p>
                    </div>
                )}

            {!loading &&
                !error &&
                requests.length > 0 && (

                    <section className="rounded-lg border border-slate-200 bg-white shadow-sm">

                        <div className="border-b border-slate-200 px-5 py-4">
                            <h2 className="text-base font-semibold text-slate-900">
                                Job History
                            </h2>

                            <p className="mt-1 text-sm text-slate-500">
                                Every request remains traceable through its current status.
                            </p>
                        </div>

                        <div className="overflow-x-auto">

                            <table className="min-w-full">

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

                                    {requests.map(
                                        (request) => (

                                            <tr
                                                key={
                                                    request._id
                                                }
                                                className="hover:bg-slate-50"
                                            >

                                                <td className="px-4 py-4">
                                                    <p className="font-medium text-slate-900">
                                                        {request.requestId}
                                                    </p>
                                                </td>

                                                <td className="px-4 py-4 text-sm text-slate-700">
                                                    {request.deviceType || "—"}
                                                </td>

                                                <td className="px-4 py-4 text-sm text-slate-700">
                                                    {request.sanitizationMethod || "—"}
                                                </td>

                                                <td className="px-4 py-4">
                                                    <StatusBadge
                                                        status={request.status}
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
                                                        className="text-sm font-medium text-indigo-600 hover:text-indigo-800"
                                                    >
                                                        Open
                                                    </Link>

                                                </td>

                                            </tr>
                                        )
                                    )}

                                </tbody>

                            </table>

                        </div>

                    </section>
                )}

        </div>
    );
}

function Metric({
    label,
    value
}) {

    return (
        <div className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
            <p className="text-sm text-slate-500">
                {label}
            </p>

            <p className="mt-2 text-2xl font-semibold text-slate-900">
                {value}
            </p>
        </div>
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
            className={`inline-flex rounded-full px-3 py-1 text-xs font-semibold ${
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

export default SanitizationHistory;