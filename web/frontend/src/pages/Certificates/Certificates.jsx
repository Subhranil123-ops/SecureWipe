import {
    useEffect,
    useMemo,
    useState
} from "react";

import {
    Link
} from "react-router-dom";

import {
    Award,
    CheckCircle2,
    Database,
    FileCheck2,
    Fingerprint,
    Search,
    ShieldCheck,
    XCircle
} from "lucide-react";

import {
    getCertificateRegistry
} from "../../services/certificateService";


const TABS = [
    {
        id: "ALL",
        label: "All"
    },
    {
        id: "SANITIZATION",
        label: "Sanitization"
    },
    {
        id: "FORENSIC",
        label: "Forensics"
    }
];


function Certificates() {

    const [
        registry,
        setRegistry
    ] = useState({
        sanitization: [],
        forensic: []
    });


    const [
        loading,
        setLoading
    ] = useState(true);


    const [
        refreshing,
        setRefreshing
    ] = useState(false);


    const [
        error,
        setError
    ] = useState("");


    const [
        tab,
        setTab
    ] = useState("ALL");


    const [
        search,
        setSearch
    ] = useState("");


    const loadCertificates =
        async silent => {

            try {

                if (
                    silent
                ) {
                    setRefreshing(
                        true
                    );
                } else {
                    setLoading(
                        true
                    );
                }


                setError("");


                const data =
                    await getCertificateRegistry();


                setRegistry({

                    sanitization:
                        Array.isArray(
                            data?.sanitization
                        )
                            ? data.sanitization
                            : [],

                    forensic:
                        Array.isArray(
                            data?.forensic
                        )
                            ? data.forensic
                            : []
                });

            } catch (
                err
            ) {

                console.error(
                    "Failed to load certificate registry:",
                    err
                );


                setError(
                    err.message ||
                    "Unable to load certificates and evidence."
                );

            } finally {

                setLoading(
                    false
                );

                setRefreshing(
                    false
                );
            }
        };


    useEffect(
        () => {
            loadCertificates(
                false
            );
        },
        []
    );


    const allCertificates =
        useMemo(
            () => {

                return [
                    ...(
                        registry.sanitization ||
                        []
                    ),

                    ...(
                        registry.forensic ||
                        []
                    )

                ].sort(
                    (
                        a,
                        b
                    ) =>
                        new Date(
                            b.generatedAt ||
                            0
                        ).getTime() -
                        new Date(
                            a.generatedAt ||
                            0
                        ).getTime()
                );

            },
            [
                registry
            ]
        );


    const filteredCertificates =
        useMemo(
            () => {

                const query =
                    search
                        .trim()
                        .toLowerCase();


                let source =
                    allCertificates;


                if (
                    tab ===
                    "SANITIZATION"
                ) {
                    source =
                        registry.sanitization ||
                        [];
                }


                if (
                    tab ===
                    "FORENSIC"
                ) {
                    source =
                        registry.forensic ||
                        [];
                }


                if (
                    !query
                ) {
                    return source;
                }


                return source.filter(
                    item => {

                        const values = [

                            item.certificateId,

                            item.requestId,

                            item.caseId,

                            item.operationId,

                            item.runId,

                            item.workstationId,

                            item.deviceId,

                            item.model,

                            item.serialNumber,

                            item.sourceIdentifier,

                            item.method,

                            item.status,

                            item.certificateHash

                        ];


                        return values.some(
                            value =>
                                String(
                                    value ||
                                    ""
                                )
                                    .toLowerCase()
                                    .includes(
                                        query
                                    )
                        );
                    }
                );

            },
            [
                allCertificates,
                registry,
                search,
                tab
            ]
        );


    const counts = {

        all:
            allCertificates.length,

        sanitization:
            registry.sanitization?.length ||
            0,

        forensic:
            registry.forensic?.length ||
            0,

        verified:
            allCertificates.filter(
                item =>
                    item.verificationPassed ===
                        true ||
                    item.integrityVerified ===
                        true
            ).length
    };


    return (
        <div className="space-y-6">

            <div className="flex flex-col gap-4 xl:flex-row xl:items-end xl:justify-between">

                <div>

                    <div className="flex items-center gap-2 text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">

                        <span className="h-1.5 w-1.5 rounded-full bg-indigo-600" />

                        Certificates & Evidence

                    </div>


                    <h1 className="mt-2 text-2xl font-semibold tracking-tight text-slate-900">
                        Certificate Registry
                    </h1>


                    <p className="mt-2 max-w-3xl text-sm leading-6 text-slate-500">
                        A single place to find certificates produced
                        by both the sanitization and forensic workflows.
                        Access is automatically scoped to the
                        logged-in role.
                    </p>

                </div>


                <button
                    type="button"
                    onClick={() =>
                        loadCertificates(
                            true
                        )
                    }
                    disabled={
                        refreshing
                    }
                    className="inline-flex w-fit items-center gap-2 rounded-xl border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50 disabled:cursor-not-allowed disabled:opacity-60"
                >

                    <Database
                        className="h-4 w-4"
                    />

                    {
                        refreshing
                            ? "Refreshing..."
                            : "Refresh"
                    }

                </button>

            </div>


            <div className="grid gap-3 sm:grid-cols-2 xl:grid-cols-4">

                <SummaryCard
                    icon={
                        <Award
                            className="h-5 w-5"
                        />
                    }
                    label="All certificates"
                    value={
                        counts.all
                    }
                />


                <SummaryCard
                    icon={
                        <ShieldCheck
                            className="h-5 w-5"
                        />
                    }
                    label="Sanitization"
                    value={
                        counts.sanitization
                    }
                />


                <SummaryCard
                    icon={
                        <Fingerprint
                            className="h-5 w-5"
                        />
                    }
                    label="Forensic"
                    value={
                        counts.forensic
                    }
                />


                <SummaryCard
                    icon={
                        <CheckCircle2
                            className="h-5 w-5"
                        />
                    }
                    label="Verified records"
                    value={
                        counts.verified
                    }
                />

            </div>


            {error && (
                <div className="rounded-2xl border border-red-200 bg-red-50 p-5">

                    <div className="flex items-start gap-3">

                        <XCircle
                            className="mt-0.5 h-5 w-5 shrink-0 text-red-600"
                        />

                        <div>

                            <p className="text-sm font-semibold text-red-900">
                                Unable to load certificate registry
                            </p>

                            <p className="mt-1 text-sm text-red-700">
                                {error}
                            </p>

                        </div>

                    </div>

                </div>
            )}


            <section className="overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-sm">

                <div className="border-b border-slate-200 p-5">

                    <div className="flex flex-col gap-4 lg:flex-row lg:items-center lg:justify-between">

                        <div className="flex flex-wrap gap-2">

                            {TABS.map(
                                item => (

                                    <button
                                        key={
                                            item.id
                                        }
                                        type="button"
                                        onClick={() =>
                                            setTab(
                                                item.id
                                            )
                                        }
                                        className={`rounded-lg px-3.5 py-2 text-sm font-medium transition ${
                                            tab ===
                                            item.id
                                                ? "bg-slate-900 text-white"
                                                : "border border-slate-200 bg-white text-slate-600 hover:bg-slate-50"
                                        }`}
                                    >
                                        {
                                            item.label
                                        }
                                    </button>
                                )
                            )}

                        </div>


                        <label className="relative block w-full max-w-md">

                            <Search
                                className="pointer-events-none absolute left-3 top-1/2 h-4 w-4 -translate-y-1/2 text-slate-400"
                            />

                            <input
                                value={
                                    search
                                }
                                onChange={
                                    event =>
                                        setSearch(
                                            event.target.value
                                        )
                                }
                                placeholder="Search certificate, request, case, device, hash..."
                                className="w-full rounded-xl border border-slate-200 bg-slate-50 py-2.5 pl-9 pr-4 text-sm text-slate-800 outline-none transition placeholder:text-slate-400 focus:border-indigo-400 focus:bg-white focus:ring-2 focus:ring-indigo-100"
                            />

                        </label>

                    </div>

                </div>


                {loading ? (

                    <div className="flex min-h-[300px] items-center justify-center">

                        <div className="text-center">

                            <div className="mx-auto h-7 w-7 animate-spin rounded-full border-2 border-indigo-600 border-t-transparent" />

                            <p className="mt-4 text-sm font-medium text-slate-600">
                                Loading certificates...
                            </p>

                        </div>

                    </div>

                ) : filteredCertificates.length === 0 ? (

                    <div className="p-12 text-center">

                        <FileCheck2
                            className="mx-auto h-10 w-10 text-slate-300"
                        />

                        <h2 className="mt-4 text-base font-semibold text-slate-900">
                            No matching certificates
                        </h2>

                        <p className="mx-auto mt-2 max-w-md text-sm leading-6 text-slate-500">
                            Completed sanitization certificates and
                            forensic certificates will appear here
                            when they have been published and are
                            visible to your account.
                        </p>

                    </div>

                ) : (

                    <div className="overflow-x-auto">

                        <table className="min-w-[1120px] w-full">

                            <thead className="border-b border-slate-200 bg-slate-50">

                                <tr>

                                    <HeaderCell>
                                        Type
                                    </HeaderCell>

                                    <HeaderCell>
                                        Certificate
                                    </HeaderCell>

                                    <HeaderCell>
                                        Request / Case
                                    </HeaderCell>

                                    <HeaderCell>
                                        Source
                                    </HeaderCell>

                                    <HeaderCell>
                                        Status
                                    </HeaderCell>

                                    <HeaderCell>
                                        Integrity
                                    </HeaderCell>

                                    <HeaderCell>
                                        Generated
                                    </HeaderCell>

                                    <HeaderCell>
                                        Action
                                    </HeaderCell>

                                </tr>

                            </thead>


                            <tbody className="divide-y divide-slate-100">

                                {filteredCertificates.map(
                                    item => (

                                        <CertificateRow
                                            key={
                                                `${item.type}-${item.certificateId}-${item.requestId || item.caseId || ""}`
                                            }
                                            item={
                                                item
                                            }
                                        />
                                    )
                                )}

                            </tbody>

                        </table>

                    </div>
                )}

            </section>


            <div className="rounded-2xl border border-amber-200 bg-amber-50 p-5">

                <div className="flex items-start gap-3">

                    <FileCheck2
                        className="mt-0.5 h-5 w-5 shrink-0 text-amber-700"
                    />

                    <div>

                        <p className="text-sm font-semibold text-amber-900">
                            Forensic reports are separate from forensic certificates
                        </p>

                        <p className="mt-1 text-sm leading-6 text-amber-800">
                            A forensic report has its own report
                            SHA-256 hash. That report hash is shown
                            under Reports and is not treated as a
                            second certificate in this registry.
                        </p>

                    </div>

                </div>

            </div>

        </div>
    );
}


function CertificateRow({
    item
}) {

    const isSanitization =
        item.type ===
        "SANITIZATION";


    const detailsUrl =
        isSanitization

            ? `/certificates/sanitization/${encodeURIComponent(
                item.certificateId
            )}`

            : `/certificates/forensic/${encodeURIComponent(
                item.caseId
            )}`;


    const integrity =
        item.verificationPassed ===
            true ||

        item.integrityVerified ===
            true;


    return (
        <tr className="transition hover:bg-slate-50">

            <td className="px-4 py-4 align-top">

                <span
                    className={`inline-flex rounded-full px-2.5 py-1 text-xs font-semibold ${
                        isSanitization
                            ? "bg-blue-50 text-blue-700"
                            : "bg-violet-50 text-violet-700"
                    }`}
                >
                    {
                        isSanitization
                            ? "SANITIZATION"
                            : "FORENSIC"
                    }
                </span>


                {item.legacy && (

                    <span className="mt-2 inline-flex rounded-full border border-amber-200 bg-amber-50 px-2.5 py-1 text-[10px] font-semibold text-amber-700">
                        LEGACY
                    </span>

                )}

            </td>


            <td className="px-4 py-4 align-top">

                <p className="max-w-[250px] break-all font-mono text-xs font-semibold text-slate-800">
                    {
                        item.certificateId
                    }
                </p>


                {item.operationId && (

                    <p className="mt-1 text-[11px] text-slate-400">
                        Operation:
                        {" "}
                        {
                            item.operationId
                        }
                    </p>

                )}


                {item.runId && (

                    <p className="mt-1 text-[11px] text-slate-400">
                        Run:
                        {" "}
                        {
                            item.runId
                        }
                    </p>

                )}

            </td>


            <td className="px-4 py-4 align-top">

                <p className="font-mono text-xs font-medium text-slate-700">
                    {
                        item.requestId ||
                        item.caseId ||
                        "—"
                    }
                </p>

                <p className="mt-1 text-[11px] text-slate-400">
                    {
                        isSanitization
                            ? "Request ID"
                            : "Case ID"
                    }
                </p>

            </td>


            <td className="px-4 py-4 align-top">

                <p className="max-w-[220px] truncate text-sm font-medium text-slate-700">
                    {
                        item.model ||
                        item.sourceName ||
                        item.deviceId ||
                        "—"
                    }
                </p>


                <p className="mt-1 max-w-[220px] truncate font-mono text-[11px] text-slate-400">
                    {
                        item.serialNumber ||
                        item.sourceIdentifier ||
                        "—"
                    }
                </p>

            </td>


            <td className="px-4 py-4 align-top">

                <span className="inline-flex rounded-full bg-slate-100 px-2.5 py-1 text-xs font-semibold text-slate-700">
                    {
                        formatStatus(
                            item.status
                        )
                    }
                </span>


                {item.verificationStatus && (

                    <p className="mt-1 text-[11px] text-slate-400">
                        Verification:
                        {" "}
                        {
                            formatStatus(
                                item.verificationStatus
                            )
                        }
                    </p>

                )}

            </td>


            <td className="px-4 py-4 align-top">

                {item.legacy ? (

                    <span className="inline-flex rounded-full bg-amber-50 px-2.5 py-1 text-xs font-semibold text-amber-700">
                        Legacy
                    </span>

                ) : integrity ? (

                    <span className="inline-flex items-center gap-1.5 rounded-full bg-green-50 px-2.5 py-1 text-xs font-semibold text-green-700">

                        <CheckCircle2
                            className="h-3.5 w-3.5"
                        />

                        Verified

                    </span>

                ) : (

                    <span className="inline-flex rounded-full bg-slate-100 px-2.5 py-1 text-xs font-semibold text-slate-600">
                        Not verified
                    </span>

                )}

            </td>


            <td className="px-4 py-4 align-top whitespace-nowrap text-sm text-slate-600">

                {
                    formatDate(
                        item.generatedAt
                    )
                }

            </td>


            <td className="px-4 py-4 align-top">

                <Link
                    to={
                        detailsUrl
                    }
                    className="inline-flex items-center gap-2 rounded-lg bg-slate-900 px-3 py-2 text-sm font-medium text-white transition hover:bg-slate-800"
                >
                    Open

                    <span
                        aria-hidden="true"
                    >
                        →
                    </span>

                </Link>

            </td>

        </tr>
    );
}


function SummaryCard({
    icon,
    label,
    value
}) {

    return (
        <div className="rounded-2xl border border-slate-200 bg-white p-4 shadow-sm">

            <div className="flex items-center justify-between gap-3">

                <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-slate-100 text-slate-600">
                    {icon}
                </div>

                <p className="text-2xl font-semibold tracking-tight text-slate-900">
                    {value}
                </p>

            </div>


            <p className="mt-3 text-xs font-semibold uppercase tracking-[0.12em] text-slate-400">
                {label}
            </p>

        </div>
    );
}


function HeaderCell({
    children
}) {

    return (
        <th className="px-4 py-3 text-left text-[11px] font-semibold uppercase tracking-[0.08em] text-slate-500">
            {children}
        </th>
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
            character =>
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
                "short"
        }
    );
}


export default Certificates;