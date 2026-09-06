import { useEffect, useState } from "react";
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

    const loadRequests =
        async () => {

            try {

                setLoading(true);
                setError("");

                const data =
                    await getEmployeeSanitizationRequests();

                setRequests(
                    data
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

    return (
        <div className="space-y-6">

            <div className="flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">

                <div>

                    <h1 className="text-2xl font-semibold text-slate-900">
                        Workstation Employee Dashboard
                    </h1>

                    <p className="mt-1 text-sm text-slate-500">
                        Welcome, {user?.name}.
                    </p>

                </div>

                <Link
                    to="/workstation-employee/sanitization/history"
                    className="w-fit rounded-lg border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 hover:bg-slate-50"
                >
                    Sanitization History
                </Link>

            </div>

            {loading && (
                <div className="rounded-lg border border-slate-200 bg-white p-6 shadow-sm">
                    <p className="text-sm text-slate-500">
                        Loading assigned requests...
                    </p>
                </div>
            )}

            {!loading && error && (
                <div className="rounded-lg border border-red-200 bg-red-50 p-6">
                    <p className="text-sm text-red-600">
                        {error}
                    </p>
                </div>
            )}

            {!loading &&
                !error &&
                requests.length === 0 && (
                    <div className="rounded-lg border border-slate-200 bg-white p-6 shadow-sm">
                        <p className="text-sm text-slate-500">
                            No sanitization requests have been assigned to you.
                        </p>
                    </div>
                )}

            {!loading &&
                !error &&
                requests.length > 0 && (

                    <div className="rounded-lg border border-slate-200 bg-white shadow-sm">

                        <div className="border-b border-slate-200 p-6">

                            <div className="flex flex-col gap-2 sm:flex-row sm:items-center sm:justify-between">

                                <div>

                                    <h2 className="text-lg font-semibold text-slate-900">
                                        My Assigned Requests
                                    </h2>

                                    <p className="mt-1 text-sm text-slate-500">
                                        Open a request to view the complete sanitization execution pipeline.
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

                        </div>

                        <div className="overflow-x-auto">

                            <table className="min-w-full">

                                <thead className="border-b border-slate-200 bg-slate-50">

                                    <tr>

                                        <th className="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider text-slate-500">
                                            Request
                                        </th>

                                        <th className="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider text-slate-500">
                                            Device
                                        </th>

                                        <th className="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider text-slate-500">
                                            Method
                                        </th>

                                        <th className="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider text-slate-500">
                                            Workstation
                                        </th>

                                        <th className="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider text-slate-500">
                                            Status
                                        </th>

                                        <th className="px-6 py-3 text-left text-xs font-medium uppercase tracking-wider text-slate-500">
                                            Action
                                        </th>

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

                                                <td className="px-6 py-4">

                                                    <div className="font-medium text-slate-900">
                                                        {
                                                            request.requestId
                                                        }
                                                    </div>

                                                </td>

                                                <td className="px-6 py-4 text-sm text-slate-700">

                                                    {
                                                        request.deviceType ||
                                                        "N/A"
                                                    }

                                                </td>

                                                <td className="px-6 py-4 text-sm text-slate-700">

                                                    {
                                                        request.sanitizationMethod ||
                                                        "N/A"
                                                    }

                                                </td>

                                                <td className="px-6 py-4 text-sm text-slate-700">

                                                    {
                                                        request
                                                            .assignedWorkstation
                                                            ?.workstationId ||
                                                        "N/A"
                                                    }

                                                </td>

                                                <td className="px-6 py-4">

                                                    <StatusBadge
                                                        status={
                                                            request.status
                                                        }
                                                    />

                                                </td>

                                                <td className="px-6 py-4">

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

                    </div>
                )}

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
            className={`inline-flex rounded-full px-3 py-1 text-xs font-medium ${
                styles[status] ||
                "bg-slate-100 text-slate-700"
            }`}
        >
            {status}
        </span>
    );
}

export default WorkstationEmployeeDashboard;