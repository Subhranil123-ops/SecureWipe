import {
    useCallback,
    useEffect,
    useMemo,
    useState,
} from "react";

import {
    Link,
    useNavigate,
    useParams,
} from "react-router-dom";

import {
    ArrowLeft,
    CheckCircle2,
    Clock3,
    FileCheck2,
    HardDrive,
    MapPin,
    RefreshCw,
    ShieldCheck,
    UserRound,
    XCircle,
} from "lucide-react";

import {
    getMySanitizationRequests,
} from "../../../services/sanitizationRequestService";

import {
    getCertificateRegistry,
} from "../../../services/certificateService";

import SanitizationPipeline
    from "../../../components/sanitization/SanitizationPipeline";


function CustomerSanitizationDetails() {

    const {
        requestId,
    } = useParams();


    const navigate =
        useNavigate();


    const [
        request,
        setRequest,
    ] = useState(null);


    const [
        certificate,
        setCertificate,
    ] = useState(null);


    const [
        loading,
        setLoading,
    ] = useState(true);


    const [
        refreshing,
        setRefreshing,
    ] = useState(false);


    const [
        error,
        setError,
    ] = useState("");


    const [
        notFound,
        setNotFound,
    ] = useState(false);


    const loadRequest =
        useCallback(
            async (
                showLoader = true
            ) => {

                try {

                    if (
                        showLoader
                    ) {
                        setLoading(true);
                    } else {
                        setRefreshing(true);
                    }


                    setError("");
                    setNotFound(false);


                    const requests =
                        await getMySanitizationRequests();


                    const matchedRequest =
                        requests.find(
                            (
                                item
                            ) =>
                                String(
                                    item?.requestId
                                ) ===
                                String(
                                    requestId
                                )
                        );


                    if (
                        !matchedRequest
                    ) {

                        setRequest(null);
                        setCertificate(null);
                        setNotFound(true);

                        return;
                    }


                    setRequest(
                        matchedRequest
                    );


                    /*
                     * Certificates are already role-scoped by
                     * the backend. We only look for the certificate
                     * belonging to this request.
                     *
                     * Do not invent or call a request-specific
                     * certificate endpoint here.
                     */

                    if (
                        matchedRequest.status ===
                        "COMPLETED"
                    ) {

                        try {

                            const registry =
                                await getCertificateRegistry();


                            const sanitizationCertificates =
                                Array.isArray(
                                    registry?.sanitization
                                )
                                    ? registry.sanitization
                                    : [];


                            const matchedCertificate =
                                sanitizationCertificates.find(
                                    (
                                        item
                                    ) =>
                                        String(
                                            item?.requestId
                                        ) ===
                                        String(
                                            matchedRequest.requestId
                                        )
                                );


                            setCertificate(
                                matchedCertificate ||
                                null
                            );

                        } catch (
                            certificateError
                        ) {

                            /*
                             * Certificate lookup failure should not
                             * prevent the customer from viewing the
                             * completed request itself.
                             */

                            console.error(
                                "Failed to load sanitization certificate:",
                                certificateError
                            );

                            setCertificate(
                                null
                            );
                        }

                    } else {

                        setCertificate(
                            null
                        );

                    }

                } catch (
                    loadError
                ) {

                    console.error(
                        "Failed to load customer sanitization request:",
                        loadError
                    );


                    setError(
                        loadError?.message ||
                        "Failed to load this sanitization request."
                    );

                } finally {

                    setLoading(false);
                    setRefreshing(false);

                }

            },
            [
                requestId,
            ]
        );


    useEffect(
        () => {

            loadRequest(
                true
            );

        },
        [
            loadRequest,
        ]
    );


    /*
     * The request list is not continuously polled here.
     *
     * SanitizationLiveProgress already polls the backend every
     * second for the native operation.
     *
     * This page only refreshes the request record when the user
     * explicitly refreshes it.
     */

    const status =
        String(
            request?.status ||
            "UNKNOWN"
        ).toUpperCase();


    const isCompleted =
        status ===
        "COMPLETED";


    const isFailed =
        status ===
        "FAILED";


    const isActive =
        [
            "ASSIGNED",
            "IN_PROGRESS",
            "VERIFYING",
        ].includes(
            status
        );


    const statusDescription =
        useMemo(
            () => {

                const descriptions = {

                    PENDING:
                        "Your request has been submitted and is waiting for processing.",

                    APPROVED:
                        "Your request has been approved and is waiting for workstation assignment.",

                    REJECTED:
                        "This request was rejected by the workstation center.",

                    ASSIGNED:
                        "The request has been assigned to a workstation and is ready for sanitization.",

                    IN_PROGRESS:
                        "The workstation is currently performing the sanitization operation.",

                    VERIFYING:
                        "The sanitization operation has finished and the result is being verified.",

                    COMPLETED:
                        "Sanitization and verification have completed successfully.",

                    FAILED:
                        "The sanitization operation failed. Please contact the workstation center for assistance.",

                    CANCELLED:
                        "This sanitization request has been cancelled.",

                };


                return (
                    descriptions[status] ||
                    "Current request status is being processed."
                );

            },
            [
                status,
            ]
        );


    if (
        loading
    ) {

        return (

            <div className="flex min-h-[420px] items-center justify-center">

                <div className="w-full max-w-md rounded-2xl border border-slate-200 bg-white p-8 text-center shadow-sm">

                    <div className="mx-auto flex h-12 w-12 items-center justify-center rounded-2xl bg-indigo-50">

                        <div className="h-5 w-5 animate-spin rounded-full border-2 border-indigo-600 border-t-transparent" />

                    </div>


                    <h2 className="mt-5 text-base font-semibold text-slate-900">
                        Loading sanitization job
                    </h2>


                    <p className="mt-2 text-sm leading-6 text-slate-500">
                        Fetching request details from your account.
                    </p>

                </div>

            </div>

        );

    }


    if (
        notFound
    ) {

        return (

            <div className="space-y-6">

                <Link
                    to="/customer/sanitization-requests"
                    className="inline-flex items-center gap-2 text-sm font-medium text-slate-600 transition hover:text-slate-900"
                >

                    <ArrowLeft
                        className="h-4 w-4"
                    />

                    Back to My Sanitization Requests

                </Link>


                <div className="rounded-2xl border border-slate-200 bg-white p-10 text-center shadow-sm">

                    <div className="mx-auto flex h-14 w-14 items-center justify-center rounded-2xl bg-slate-100">

                        <XCircle
                            className="h-7 w-7 text-slate-400"
                        />

                    </div>


                    <h1 className="mt-5 text-lg font-semibold text-slate-900">
                        Sanitization request not found
                    </h1>


                    <p className="mx-auto mt-2 max-w-lg text-sm leading-6 text-slate-500">
                        The requested sanitization job does not exist in
                        your account, or it is no longer available.
                    </p>


                    <Link
                        to="/customer/sanitization-requests"
                        className="mt-6 inline-flex rounded-lg bg-indigo-600 px-4 py-2.5 text-sm font-semibold text-white transition hover:bg-indigo-700"
                    >
                        View My Requests
                    </Link>

                </div>

            </div>

        );

    }


    if (
        error &&
        !request
    ) {

        return (

            <div className="space-y-6">

                <Link
                    to="/customer/sanitization-requests"
                    className="inline-flex items-center gap-2 text-sm font-medium text-slate-600 transition hover:text-slate-900"
                >

                    <ArrowLeft
                        className="h-4 w-4"
                    />

                    Back to My Sanitization Requests

                </Link>


                <div className="rounded-2xl border border-red-200 bg-red-50 p-6">

                    <div className="flex items-start gap-3">

                        <XCircle
                            className="mt-0.5 h-5 w-5 shrink-0 text-red-600"
                        />


                        <div>

                            <p className="text-sm font-semibold text-red-900">
                                Unable to load sanitization job
                            </p>


                            <p className="mt-1 text-sm leading-6 text-red-700">
                                {error}
                            </p>


                            <button
                                type="button"
                                onClick={
                                    () =>
                                        loadRequest(
                                            true
                                        )
                                }
                                className="mt-4 rounded-lg border border-red-200 bg-white px-4 py-2 text-sm font-medium text-red-700 transition hover:bg-red-50"
                            >
                                Try Again
                            </button>

                        </div>

                    </div>

                </div>

            </div>

        );

    }


    return (

        <div className="space-y-6">

            {/* =====================================================
                HEADER
               ===================================================== */}

            <div className="flex flex-col gap-4 lg:flex-row lg:items-start lg:justify-between">

                <div>

                    <Link
                        to="/customer/sanitization-requests"
                        className="inline-flex items-center gap-2 text-sm font-medium text-slate-500 transition hover:text-slate-800"
                    >

                        <ArrowLeft
                            className="h-4 w-4"
                        />

                        My Sanitization Requests

                    </Link>


                    <div className="mt-4">

                        <p className="text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                            Sanitization Job
                        </p>


                        <div className="mt-2 flex flex-wrap items-center gap-3">

                            <h1 className="font-mono text-2xl font-semibold tracking-tight text-slate-900">
                                {
                                    request.requestId
                                }
                            </h1>


                            <StatusBadge
                                status={
                                    status
                                }
                            />

                        </div>


                        <p className="mt-2 max-w-3xl text-sm leading-6 text-slate-500">
                            {statusDescription}
                        </p>

                    </div>

                </div>


                <div className="flex flex-wrap gap-2">

                    <button
                        type="button"
                        onClick={
                            () =>
                                loadRequest(
                                    false
                                )
                        }
                        disabled={
                            refreshing
                        }
                        className="inline-flex items-center gap-2 rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50 disabled:cursor-not-allowed disabled:opacity-60"
                    >

                        <RefreshCw
                            className={`h-4 w-4 ${
                                refreshing
                                    ? "animate-spin"
                                    : ""
                            }`}
                        />

                        {
                            refreshing
                                ? "Refreshing..."
                                : "Refresh"
                        }

                    </button>


                    {certificate && (

                        <Link
                            to={`/certificates/sanitization/${encodeURIComponent(
                                certificate.certificateId
                            )}`}
                            className="inline-flex items-center gap-2 rounded-lg bg-green-600 px-4 py-2.5 text-sm font-semibold text-white transition hover:bg-green-700"
                        >

                            <FileCheck2
                                className="h-4 w-4"
                            />

                            View Certificate

                        </Link>

                    )}

                </div>

            </div>


            {/* ERROR WHEN REQUEST EXISTS */}

            {error && (

                <div className="rounded-xl border border-amber-200 bg-amber-50 px-4 py-3">

                    <p className="text-sm text-amber-800">
                        {error}
                    </p>

                </div>

            )}


            {/* =====================================================
                STATUS SUMMARY
               ===================================================== */}

            <div className="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">

                <InfoCard
                    icon={
                        <ShieldCheck
                            className="h-5 w-5"
                        />
                    }
                    label="Status"
                    value={
                        formatStatus(
                            status
                        )
                    }
                />


                <InfoCard
                    icon={
                        <HardDrive
                            className="h-5 w-5"
                        />
                    }
                    label="Device"
                    value={
                        request.deviceType ||
                        "Not specified"
                    }
                />


                <InfoCard
                    icon={
                        <Clock3
                            className="h-5 w-5"
                        />
                    }
                    label="Created"
                    value={
                        formatDate(
                            request.createdAt
                        )
                    }
                />


                <InfoCard
                    icon={
                        certificate
                            ? (
                                <CheckCircle2
                                    className="h-5 w-5"
                                />
                            )
                            : (
                                <FileCheck2
                                    className="h-5 w-5"
                                />
                            )
                    }
                    label="Certificate"
                    value={
                        certificate
                            ? "Available"
                            : isCompleted
                                ? "Processing"
                                : "Not available"
                    }
                    tone={
                        certificate
                            ? "success"
                            : "default"
                    }
                />

            </div>


            {/* =====================================================
                REQUEST INFORMATION
               ===================================================== */}

            <section className="rounded-2xl border border-slate-200 bg-white shadow-sm">

                <div className="border-b border-slate-200 px-5 py-4">

                    <h2 className="text-base font-semibold text-slate-900">
                        Request Details
                    </h2>


                    <p className="mt-1 text-sm text-slate-500">
                        Information associated with this sanitization request.
                    </p>

                </div>


                <div className="grid gap-5 p-5 md:grid-cols-2 xl:grid-cols-3">

                    <DetailItem
                        label="Request ID"
                        value={
                            request.requestId
                        }
                        mono
                    />


                    <DetailItem
                        label="Device Type"
                        value={
                            request.deviceType
                        }
                    />


                    <DetailItem
                        label="Capacity"
                        value={
                            request.capacity
                        }
                    />


                    <DetailItem
                        label="Asset Identifier"
                        value={
                            request.assetIdentifier
                        }
                    />


                    <DetailItem
                        label="Sanitization Method"
                        value={
                            formatMethod(
                                request.sanitizationMethod
                            )
                        }
                    />


                    <DetailItem
                        label="Created"
                        value={
                            formatDate(
                                request.createdAt
                            )
                        }
                    />


                    <DetailItem
                        label="Assigned Employee"
                        value={
                            request.assignedEmployee
                                ?.name ||
                            "Not assigned"
                        }
                        icon={
                            <UserRound
                                className="h-4 w-4"
                            />
                        }
                    />


                    <DetailItem
                        label="Assigned Workstation"
                        value={
                            request.assignedWorkstation
                                ?.name ||
                            "Not assigned"
                        }
                    />


                    <DetailItem
                        label="Workstation ID"
                        value={
                            request.assignedWorkstation
                                ?.workstationId ||
                            "Not assigned"
                        }
                        mono
                    />


                    <DetailItem
                        label="Workstation Center"
                        value={
                            request.workstationCenter
                                ?.name ||
                            request.workstationCenter
                                ?.centerId ||
                            "Not assigned"
                        }
                        icon={
                            <MapPin
                                className="h-4 w-4"
                            />
                        }
                    />

                </div>

            </section>


            {/* =====================================================
                LIVE PIPELINE
               ===================================================== */}

            {isActive ||
                isCompleted ||
                isFailed ? (

                <SanitizationPipeline
                    status={
                        status
                    }
                />

            ) : (

                <section className="rounded-2xl border border-slate-200 bg-white p-6 shadow-sm">

                    <div className="flex items-start gap-4">

                        <div className="flex h-11 w-11 shrink-0 items-center justify-center rounded-xl bg-slate-100">

                            <Clock3
                                className="h-5 w-5 text-slate-500"
                            />

                        </div>


                        <div>

                            <h2 className="text-base font-semibold text-slate-900">
                                Sanitization pipeline
                            </h2>


                            <p className="mt-1 text-sm leading-6 text-slate-500">
                                Live execution progress will appear here once
                                the workstation starts processing this request.
                            </p>

                        </div>

                    </div>

                </section>

            )}


            {/* =====================================================
                COMPLETION / CERTIFICATE
               ===================================================== */}

            {isCompleted && (

                <section
                    className={`rounded-2xl border p-5 shadow-sm ${
                        certificate
                            ? "border-green-200 bg-green-50"
                            : "border-amber-200 bg-amber-50"
                    }`}
                >

                    <div className="flex flex-col gap-5 lg:flex-row lg:items-center lg:justify-between">

                        <div className="flex items-start gap-4">

                            <div
                                className={`flex h-11 w-11 shrink-0 items-center justify-center rounded-xl ${
                                    certificate
                                        ? "bg-green-100 text-green-700"
                                        : "bg-amber-100 text-amber-700"
                                }`}
                            >

                                {certificate ? (

                                    <CheckCircle2
                                        className="h-5 w-5"
                                    />

                                ) : (

                                    <FileCheck2
                                        className="h-5 w-5"
                                    />

                                )}

                            </div>


                            <div>

                                <h2
                                    className={`text-base font-semibold ${
                                        certificate
                                            ? "text-green-900"
                                            : "text-amber-900"
                                    }`}
                                >

                                    {certificate
                                        ? "Sanitization completed"
                                        : "Sanitization completed"}

                                </h2>


                                <p
                                    className={`mt-1 text-sm leading-6 ${
                                        certificate
                                            ? "text-green-800"
                                            : "text-amber-800"
                                    }`}
                                >

                                    {certificate

                                        ? `Certificate ${certificate.certificateId} has been generated and is ready to view.`

                                        : "The request is marked completed. The certificate registry has not returned the certificate yet. Refresh shortly."}

                                </p>

                            </div>

                        </div>


                        {certificate && (

                            <Link
                                to={`/certificates/sanitization/${encodeURIComponent(
                                    certificate.certificateId
                                )}`}
                                className="inline-flex shrink-0 items-center justify-center gap-2 rounded-lg bg-green-600 px-5 py-2.5 text-sm font-semibold text-white transition hover:bg-green-700"
                            >

                                <FileCheck2
                                    className="h-4 w-4"
                                />

                                View Certificate

                            </Link>

                        )}

                    </div>

                </section>

            )}


            {/* =====================================================
                FAILURE
               ===================================================== */}

            {isFailed && (

                <section className="rounded-2xl border border-red-200 bg-red-50 p-5">

                    <div className="flex items-start gap-4">

                        <div className="flex h-11 w-11 shrink-0 items-center justify-center rounded-xl bg-red-100 text-red-700">

                            <XCircle
                                className="h-5 w-5"
                            />

                        </div>


                        <div>

                            <h2 className="text-base font-semibold text-red-900">
                                Sanitization failed
                            </h2>


                            <p className="mt-1 text-sm leading-6 text-red-800">
                                The workstation reported a failed sanitization
                                operation. Please contact the assigned
                                workstation center for further assistance.
                            </p>

                        </div>

                    </div>

                </section>

            )}


            {/* =====================================================
                FOOTER NAVIGATION
               ===================================================== */}

            <div className="flex flex-wrap gap-3">

                <Link
                    to="/customer/sanitization-requests"
                    className="inline-flex items-center gap-2 rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                >

                    <ArrowLeft
                        className="h-4 w-4"
                    />

                    All My Requests

                </Link>


                <Link
                    to="/certificates"
                    className="inline-flex items-center gap-2 rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                >

                    <FileCheck2
                        className="h-4 w-4"
                    />

                    Certificate Registry

                </Link>

            </div>

        </div>

    );
}


function InfoCard({
    icon,
    label,
    value,
    tone = "default",
}) {

    const iconClass =
        tone ===
        "success"

            ? "bg-green-100 text-green-700"

            : "bg-slate-100 text-slate-600";


    return (

        <div className="rounded-2xl border border-slate-200 bg-white p-4 shadow-sm">

            <div className="flex items-start justify-between gap-3">

                <div>

                    <p className="text-xs font-semibold uppercase tracking-[0.12em] text-slate-400">
                        {label}
                    </p>


                    <p className="mt-2 text-sm font-semibold text-slate-900">
                        {value || "—"}
                    </p>

                </div>


                <div
                    className={`flex h-9 w-9 shrink-0 items-center justify-center rounded-lg ${iconClass}`}
                >
                    {icon}
                </div>

            </div>

        </div>

    );
}


function DetailItem({
    label,
    value,
    mono = false,
    icon,
}) {

    return (

        <div>

            <p className="text-[11px] font-semibold uppercase tracking-[0.12em] text-slate-400">
                {label}
            </p>


            <div className="mt-1 flex items-center gap-2">

                {icon && (
                    <span className="shrink-0 text-slate-400">
                        {icon}
                    </span>
                )}


                <p
                    className={`break-all text-sm font-medium text-slate-800 ${
                        mono
                            ? "font-mono"
                            : ""
                    }`}
                >
                    {value || "—"}
                </p>

            </div>

        </div>

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


function formatStatus(
    value
) {

    if (
        !value
    ) {
        return "—";
    }


    return String(
        value
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


function formatMethod(
    value
) {

    if (
        !value
    ) {
        return "—";
    }


    return String(
        value
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

    if (
        !value
    ) {
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


export default CustomerSanitizationDetails;