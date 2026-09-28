import {
    useEffect,
    useMemo,
    useState,
} from "react";

import {
    Link,
} from "react-router-dom";

import {
    getMySanitizationRequests,
} from "../../../services/sanitizationRequestService";


function CustomerSanitizationRequests() {

    const [
        requests,
        setRequests,
    ] = useState([]);

    const [
        loading,
        setLoading,
    ] = useState(true);

    const [
        error,
        setError,
    ] = useState("");

    const [
        search,
        setSearch,
    ] = useState("");

    const [
        statusFilter,
        setStatusFilter,
    ] = useState("ALL");


    const loadRequests =
        async () => {

            try {

                setLoading(true);

                setError("");


                const data =
                    await getMySanitizationRequests();


                setRequests(
                    Array.isArray(data)
                        ? data
                        : []
                );

            } catch (err) {

                console.error(
                    "Failed to load customer sanitization requests:",
                    err
                );


                setError(
                    err.message ||
                    "Failed to load your sanitization requests."
                );

            } finally {

                setLoading(false);

            }
        };


    useEffect(
        () => {

            loadRequests();

        },
        []
    );


    const counts =
        useMemo(
            () => {

                const count =
                    (
                        status
                    ) =>
                        requests.filter(
                            (request) =>
                                request.status ===
                                status
                        ).length;


                return {

                    total:
                        requests.length,

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

                };

            },
            [
                requests,
            ]
        );


    const filteredRequests =
        useMemo(
            () => {

                const normalized =
                    search
                        .trim()
                        .toLowerCase();


                return requests.filter(
                    (
                        request
                    ) => {

                        const matchesStatus =
                            statusFilter ===
                                "ALL" ||
                            request.status ===
                                statusFilter;


                        if (
                            !normalized
                        ) {
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

                                request.workstationCenter
                                    ?.centerId,

                                request.workstationCenter
                                    ?.name,

                                request.assignedEmployee
                                    ?.name,

                                request.assignedWorkstation
                                    ?.workstationId,

                                request.assignedWorkstation
                                    ?.name,

                            ]
                                .filter(
                                    Boolean
                                )
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

            },
            [
                requests,
                search,
                statusFilter,
            ]
        );


    return (
        <div className="space-y-6">

            {/* HEADER */}

            <div className="flex flex-col gap-4 lg:flex-row lg:items-end lg:justify-between">

                <div>

                    <p className="text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                        Sanitization
                    </p>


                    <h1 className="mt-2 text-2xl font-semibold tracking-tight text-slate-900">
                        My Sanitization Requests
                    </h1>


                    <p className="mt-1 max-w-2xl text-sm leading-6 text-slate-500">
                        Track the sanitization requests submitted from your
                        account, view their current status, and open a job to
                        follow its execution progress.
                    </p>

                </div>


                <div className="flex flex-wrap gap-2">

                    <Link
                        to="/customer/sanitization-request"
                        className="inline-flex items-center justify-center rounded-lg bg-indigo-600 px-4 py-2.5 text-sm font-semibold text-white transition hover:bg-indigo-700"
                    >
                        New Sanitization Request
                    </Link>


                    <button
                        type="button"
                        onClick={
                            loadRequests
                        }
                        className="inline-flex items-center justify-center rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                    >
                        Refresh
                    </button>

                </div>

            </div>


            {/* SUMMARY */}

            <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-5">

                <Metric
                    label="Total"
                    value={
                        counts.total
                    }
                    description="Your requests"
                />


                <Metric
                    label="Pending"
                    value={
                        counts.pending
                    }
                    description="Awaiting processing"
                />


                <Metric
                    label="Assigned"
                    value={
                        counts.assigned
                    }
                    description="Assigned to workstation"
                />


                <Metric
                    label="In Progress"
                    value={
                        counts.inProgress
                    }
                    description="Sanitization active"
                    tone="info"
                />


                <Metric
                    label="Completed"
                    value={
                        counts.completed
                    }
                    description="Certificate available"
                    tone="success"
                />

            </div>


            {/* LOADING */}

            {loading && (

                <div className="flex min-h-[320px] items-center justify-center">

                    <div className="w-full max-w-md rounded-2xl border border-slate-200 bg-white p-8 text-center shadow-sm">

                        <div className="mx-auto flex h-12 w-12 items-center justify-center rounded-2xl bg-indigo-50">

                            <div className="h-5 w-5 animate-spin rounded-full border-2 border-indigo-600 border-t-transparent" />

                        </div>


                        <h2 className="mt-5 text-base font-semibold text-slate-900">
                            Loading your requests
                        </h2>


                        <p className="mt-2 text-sm leading-6 text-slate-500">
                            Fetching sanitization requests associated with
                            your account.
                        </p>

                    </div>

                </div>

            )}


            {/* ERROR */}

            {!loading &&
                error && (

                    <div className="rounded-2xl border border-red-200 bg-red-50 p-5">

                        <p className="text-sm font-medium text-red-700">
                            {error}
                        </p>


                        <button
                            type="button"
                            onClick={
                                loadRequests
                            }
                            className="mt-4 rounded-lg border border-red-200 bg-white px-4 py-2 text-sm font-medium text-red-700 transition hover:bg-red-50"
                        >
                            Try Again
                        </button>

                    </div>

                )}


            {/* REQUEST TABLE */}

            {!loading &&
                !error && (

                    <section className="overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-sm">

                        {/* TABLE HEADER */}

                        <div className="border-b border-slate-200 p-5">

                            <div className="flex flex-col gap-4 xl:flex-row xl:items-end xl:justify-between">

                                <div>

                                    <h2 className="text-base font-semibold text-slate-900">
                                        Your Sanitization Jobs
                                    </h2>


                                    <p className="mt-1 text-sm text-slate-500">
                                        Select a request to view its details
                                        and live processing status.
                                    </p>

                                </div>


                                <div className="flex flex-col gap-2 sm:flex-row">

                                    <input
                                        type="text"
                                        value={
                                            search
                                        }
                                        onChange={
                                            (
                                                event
                                            ) =>
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
                                        onChange={
                                            (
                                                event
                                            ) =>
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


                            <p className="mt-4 text-xs text-slate-400">

                                Showing{" "}

                                <span className="font-semibold text-slate-600">
                                    {
                                        filteredRequests.length
                                    }
                                </span>

                                {" "}of{" "}

                                <span className="font-semibold text-slate-600">
                                    {
                                        requests.length
                                    }
                                </span>

                                {" "}requests

                            </p>

                        </div>


                        {/* EMPTY STATE */}

                        {filteredRequests.length ===
                            0 ? (

                            <div className="p-10 text-center">

                                <div className="mx-auto flex h-14 w-14 items-center justify-center rounded-2xl bg-slate-100 text-xl text-slate-400">
                                    ⌕
                                </div>


                                <h3 className="mt-4 text-sm font-semibold text-slate-900">
                                    No matching requests
                                </h3>


                                <p className="mt-1 text-sm leading-6 text-slate-500">
                                    {requests.length === 0
                                        ? "You have not submitted any sanitization requests yet."
                                        : "No requests match the current search or status filter."}
                                </p>


                                {requests.length ===
                                    0 && (

                                    <Link
                                        to="/customer/sanitization-request"
                                        className="mt-5 inline-flex rounded-lg bg-indigo-600 px-4 py-2.5 text-sm font-semibold text-white transition hover:bg-indigo-700"
                                    >
                                        Create Sanitization Request
                                    </Link>

                                )}

                            </div>

                        ) : (

                            /* TABLE */

                            <div className="overflow-x-auto">

                                <table className="min-w-[1100px] w-full">

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
                                                Created
                                            </Th>


                                            <Th>
                                                Action
                                            </Th>

                                        </tr>

                                    </thead>


                                    <tbody className="divide-y divide-slate-100">

                                        {filteredRequests.map(
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

                                                    {/* REQUEST */}

                                                    <td className="px-4 py-4">

                                                        <p className="font-medium text-slate-900">
                                                            {
                                                                request.requestId
                                                            }
                                                        </p>


                                                        {request.workstationCenter && (

                                                            <p className="mt-1 text-xs text-slate-400">

                                                                {request
                                                                    .workstationCenter
                                                                    .name ||
                                                                    request
                                                                        .workstationCenter
                                                                        .centerId ||
                                                                    "Workstation"}

                                                            </p>

                                                        )}

                                                    </td>


                                                    {/* DEVICE */}

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


                                                    {/* METHOD */}

                                                    <td className="px-4 py-4 text-sm text-slate-700">

                                                        {
                                                            formatMethod(
                                                                request.sanitizationMethod
                                                            )
                                                        }

                                                    </td>


                                                    {/* WORKSTATION */}

                                                    <td className="px-4 py-4">

                                                        <p className="text-sm font-medium text-slate-800">

                                                            {
                                                                request
                                                                    .assignedWorkstation
                                                                    ?.name ||
                                                                "Not assigned"
                                                            }

                                                        </p>


                                                        {request
                                                            .assignedWorkstation
                                                            ?.workstationId && (

                                                            <p className="mt-1 font-mono text-xs text-slate-400">

                                                                {
                                                                    request
                                                                        .assignedWorkstation
                                                                        .workstationId
                                                                }

                                                            </p>

                                                        )}

                                                    </td>


                                                    {/* STATUS */}

                                                    <td className="px-4 py-4">

                                                        <StatusBadge
                                                            status={
                                                                request.status
                                                            }
                                                        />

                                                    </td>


                                                    {/* CREATED */}

                                                    <td className="px-4 py-4 text-sm text-slate-600">

                                                        {
                                                            formatDate(
                                                                request.createdAt
                                                            )
                                                        }

                                                    </td>


                                                    {/* ACTION */}

                                                    <td className="px-4 py-4">

                                                        <Link
                                                            to={`/customer/sanitization-requests/${request.requestId}`}
                                                            className="inline-flex items-center rounded-lg bg-indigo-600 px-3.5 py-2 text-xs font-semibold text-white transition hover:bg-indigo-700"
                                                        >
                                                            View Job
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


            {/* INFORMATION */}

            {!loading &&
                !error &&
                requests.length > 0 && (

                    <div className="rounded-2xl border border-slate-200 bg-slate-50 p-5">

                        <div className="flex flex-col gap-2 sm:flex-row sm:items-start sm:justify-between">

                            <div>

                                <p className="text-sm font-semibold text-slate-800">
                                    Track your sanitization
                                </p>


                                <p className="mt-1 max-w-3xl text-sm leading-6 text-slate-500">
                                    Open any request to see its current
                                    processing state. When the workstation
                                    completes verification, the generated
                                    sanitization certificate will be available
                                    from the job details.
                                </p>

                            </div>


                            <div className="shrink-0 rounded-full border border-slate-200 bg-white px-3 py-1 text-xs font-medium text-slate-500">
                                {
                                    counts.completed
                                }{" "}
                                Completed
                            </div>

                        </div>

                    </div>

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

    const styles = {

        default:
            "bg-slate-100 text-slate-600",

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

                    {tone ===
                        "success"
                        ? "✓"
                        : tone ===
                            "info"
                            ? "•"
                            : "#"}

                </div>

            </div>

        </div>

    );
}


function Th({
    children,
}) {

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

        PENDING:
            "Pending",

        APPROVED:
            "Approved",

        REJECTED:
            "Rejected",

        ASSIGNED:
            "Assigned",

        IN_PROGRESS:
            "In Progress",

        VERIFYING:
            "Verifying",

        COMPLETED:
            "Completed",

        FAILED:
            "Failed",

        CANCELLED:
            "Cancelled",

    };


    return (

        <span
            className={`inline-flex items-center gap-1.5 rounded-full border px-3 py-1 text-xs font-semibold ${
                styles[status] ||
                "border-slate-200 bg-slate-100 text-slate-700"
            }`}
        >

            <span className="h-1.5 w-1.5 rounded-full bg-current" />


            {
                labels[status] ||
                status ||
                "Unknown"
            }

        </span>

    );

}


function formatMethod(
    method
) {

    if (!method) {
        return "—";
    }


    return String(
        method
    )
        .replaceAll(
            "_",
            " "
        )
        .replace(
            /\b\w/g,
            (
                character
            ) =>
                character.toUpperCase()
        );

}


function formatDate(
    value
) {

    if (!value) {
        return "—";
    }


    const date =
        new Date(
            value
        );


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
                "short",
        }
    );

}


export default CustomerSanitizationRequests;