import {
    useEffect,
    useMemo,
    useState
} from "react";

import {
    Link,
    useParams
} from "react-router-dom";

import {
    ArrowLeft,
    CheckCircle2,
    FileCheck2,
    Fingerprint,
    Hash,
    RefreshCw,
    ShieldCheck,
    ShieldX
} from "lucide-react";


import {
    getForensicCertificateDetails,
    getSanitizationCertificateDetails
} from "../../services/certificateService";


import {
    getSanitizationAuditChain,
    verifySanitizationAuditChain,
    verifySanitizationCertificate
} from "../../services/sanitizationCertificateService";


import {
    getForensicIntegrity,
    verifyForensicIntegrity
} from "../../services/forensicService";


function CertificateDetails() {

    const {
        kind,
        id
    } = useParams();


    const isForensic =
        kind === "forensic";


    const [
        data,
        setData
    ] = useState(null);


    const [
        audit,
        setAudit
    ] = useState(null);


    const [
        verification,
        setVerification
    ] = useState(null);


    const [
        auditVerification,
        setAuditVerification
    ] = useState(null);


    const [
        loading,
        setLoading
    ] = useState(true);


    const [
        verifyLoading,
        setVerifyLoading
    ] = useState(false);


    const [
        auditLoading,
        setAuditLoading
    ] = useState(false);


    const [
        auditVerifyLoading,
        setAuditVerifyLoading
    ] = useState(false);


    const [
        error,
        setError
    ] = useState("");


    const [
        auditError,
        setAuditError
    ] = useState("");


    const load =
        async () => {

            try {

                setLoading(
                    true
                );

                setError(
                    ""
                );

                setAuditError(
                    ""
                );

                setVerification(
                    null
                );

                setAuditVerification(
                    null
                );


                if (
                    isForensic
                ) {

                    const forensicData =
                        await getForensicCertificateDetails(
                            id
                        );


                    setData(
                        forensicData
                    );


                    try {

                        setAuditLoading(
                            true
                        );


                        const integrity =
                            await getForensicIntegrity(
                                id
                            );


                        setAudit(
                            integrity
                        );

                    } catch (
                        err
                    ) {

                        console.error(
                            "Failed to load forensic integrity:",
                            err
                        );


                        setAudit(
                            null
                        );


                        setAuditError(
                            err.message ||
                            "Forensic integrity evidence is not available."
                        );

                    } finally {

                        setAuditLoading(
                            false
                        );
                    }

                } else {

                    const sanitizationData =
                        await getSanitizationCertificateDetails(
                            id
                        );


                    setData(
                        sanitizationData
                    );


                    if (
                        sanitizationData?.requestId
                    ) {

                        try {

                            setAuditLoading(
                                true
                            );


                            const auditData =
                                await getSanitizationAuditChain(
                                    sanitizationData.requestId
                                );


                            setAudit(
                                auditData
                            );

                        } catch (
                            err
                        ) {

                            console.error(
                                "Failed to load sanitization audit chain:",
                                err
                            );


                            setAudit(
                                null
                            );


                            setAuditError(
                                err.message ||
                                "Sanitization audit evidence is not available."
                            );

                        } finally {

                            setAuditLoading(
                                false
                            );
                        }
                    }
                }

            } catch (
                err
            ) {

                console.error(
                    "Failed to load certificate details:",
                    err
                );


                setError(
                    err.message ||
                    "Unable to load certificate details."
                );

            } finally {

                setLoading(
                    false
                );
            }
        };


    useEffect(
        () => {
            load();
        },
        [
            id,
            isForensic
        ]
    );


    const handleVerifyCertificate =
        async () => {

            if (
                isForensic
            ) {

                try {

                    setVerifyLoading(
                        true
                    );

                    setError(
                        ""
                    );


                    const result =
                        await verifyForensicIntegrity(
                            id
                        );


                    setVerification(
                        result
                    );

                } catch (
                    err
                ) {

                    console.error(
                        "Forensic certificate verification failed:",
                        err
                    );


                    setError(
                        err.message ||
                        "Forensic certificate verification failed."
                    );

                } finally {

                    setVerifyLoading(
                        false
                    );
                }

                return;
            }


            try {

                setVerifyLoading(
                    true
                );

                setError(
                    ""
                );


                const result =
                    await verifySanitizationCertificate(
                        id
                    );


                setVerification(
                    result
                );

            } catch (
                err
            ) {

                console.error(
                    "Sanitization certificate verification failed:",
                    err
                );


                setError(
                    err.message ||
                    "Sanitization certificate verification failed."
                );

            } finally {

                setVerifyLoading(
                    false
                );
            }
        };


    const handleVerifyAudit =
        async () => {

            try {

                setAuditVerifyLoading(
                    true
                );

                setAuditError(
                    ""
                );


                const result =
                    isForensic

                        ? await verifyForensicIntegrity(
                            id
                        )

                        : await verifySanitizationAuditChain(
                            data.requestId
                        );


                setAuditVerification(
                    result
                );


                if (
                    isForensic
                ) {

                    setAudit(
                        previous => ({
                            ...(previous || {}),
                            ...result
                        })
                    );
                }

            } catch (
                err
            ) {

                console.error(
                    "Audit integrity verification failed:",
                    err
                );


                setAuditError(
                    err.message ||
                    "Audit integrity verification failed."
                );

            } finally {

                setAuditVerifyLoading(
                    false
                );
            }
        };


    const certificate =
        isForensic
            ? data?.certificate
            : data;


    const certificateIntegrityPassed =
        isForensic

            ? (
                verification?.certificateValid === true ||
                verification?.valid === true ||
                certificate?.integrityVerified === true
            )

            : (
                verification?.valid === true ||
                data?.integrityVerified === true
            );


    const auditIntegrityPassed =
        isForensic

            ? (
                auditVerification?.chainValid === true ||
                auditVerification?.valid === true ||
                audit?.chainValid === true
            )

            : (
                auditVerification?.valid === true ||
                audit?.chainValid === true
            );


    const artifacts =
        useMemo(
            () => {

                if (
                    isForensic
                ) {

                    return Array.isArray(
                        data?.artifacts
                    )
                        ? data.artifacts
                        : [];
                }


                return [];

            },
            [
                data,
                isForensic
            ]
        );


    if (
        loading
    ) {

        return (
            <div className="flex min-h-[430px] items-center justify-center">

                <div className="rounded-2xl border border-slate-200 bg-white p-8 text-center shadow-sm">

                    <RefreshCw
                        className="mx-auto h-7 w-7 animate-spin text-indigo-600"
                    />

                    <h2 className="mt-4 text-base font-semibold text-slate-900">
                        Loading certificate
                    </h2>

                    <p className="mt-2 text-sm text-slate-500">
                        Retrieving the certificate and its integrity evidence.
                    </p>

                </div>

            </div>
        );
    }


    if (
        error &&
        !data
    ) {

        return (
            <div className="space-y-4">

                <div className="rounded-2xl border border-red-200 bg-red-50 p-5">

                    <div className="flex items-start gap-3">

                        <ShieldX
                            className="mt-0.5 h-5 w-5 shrink-0 text-red-600"
                        />

                        <div>

                            <p className="text-sm font-semibold text-red-900">
                                Unable to load certificate
                            </p>

                            <p className="mt-1 text-sm leading-6 text-red-700">
                                {error}
                            </p>

                        </div>

                    </div>

                </div>


                <Link
                    to="/certificates"
                    className="inline-flex items-center gap-2 rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 hover:bg-slate-50"
                >

                    <ArrowLeft
                        className="h-4 w-4"
                    />

                    Back to Certificates

                </Link>

            </div>
        );
    }


    return (
        <div className="space-y-6">

            <div className="flex flex-col gap-4 lg:flex-row lg:items-end lg:justify-between">

                <div>

                    <div className="flex items-center gap-2 text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">

                        <span className="h-1.5 w-1.5 rounded-full bg-indigo-600" />

                        {
                            isForensic
                                ? "Forensic Certificate"
                                : "Sanitization Certificate"
                        }

                    </div>


                    <h1 className="mt-2 break-words text-2xl font-semibold tracking-tight text-slate-900">
                        {
                            certificate?.certificateId ||
                            "Certificate"
                        }
                    </h1>

                </div>


                <div className="flex flex-wrap gap-2">

                    <button
                        type="button"
                        onClick={
                            handleVerifyCertificate
                        }
                        disabled={
                            verifyLoading ||
                            Boolean(
                                data?.certificate?.legacy
                            )
                        }
                        className="inline-flex items-center gap-2 rounded-lg bg-slate-900 px-4 py-2.5 text-sm font-medium text-white hover:bg-slate-800 disabled:cursor-not-allowed disabled:opacity-50"
                    >

                        <ShieldCheck
                            className="h-4 w-4"
                        />

                        {
                            verifyLoading
                                ? "Verifying..."
                                : "Verify Certificate"
                        }

                    </button>


                    <Link
                        to="/certificates"
                        className="inline-flex items-center gap-2 rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 hover:bg-slate-50"
                    >

                        <ArrowLeft
                            className="h-4 w-4"
                        />

                        Back

                    </Link>

                </div>

            </div>


            {error && (

                <div className="rounded-xl border border-red-200 bg-red-50 px-4 py-3 text-sm text-red-700">
                    {error}
                </div>

            )}


            <section className="rounded-2xl border border-slate-200 bg-white shadow-sm">

                <div className="border-b border-slate-200 p-5">

                    <div className="flex flex-wrap items-center gap-2">

                        <span
                            className={`rounded-full px-2.5 py-1 text-xs font-semibold ${
                                isForensic
                                    ? "bg-violet-50 text-violet-700"
                                    : "bg-blue-50 text-blue-700"
                            }`}
                        >
                            {
                                isForensic
                                    ? "FORENSIC"
                                    : "SANITIZATION"
                            }
                        </span>


                        <span className="rounded-full bg-slate-100 px-2.5 py-1 text-xs font-semibold text-slate-700">
                            {
                                formatStatus(
                                    certificate?.status
                                )
                            }
                        </span>


                        {certificateIntegrityPassed ? (

                            <span className="inline-flex items-center gap-1.5 rounded-full bg-green-50 px-2.5 py-1 text-xs font-semibold text-green-700">

                                <CheckCircle2
                                    className="h-3.5 w-3.5"
                                />

                                Integrity verified

                            </span>

                        ) : (

                            <span className="rounded-full bg-slate-100 px-2.5 py-1 text-xs font-semibold text-slate-600">
                                Verification pending
                            </span>

                        )}

                    </div>


                    <p className="mt-3 max-w-4xl text-sm leading-6 text-slate-500">

                        {isForensic

                            ? "This record represents the native forensic certificate produced for a forensic acquisition and evidence-recovery run."

                            : "This record represents the certificate produced after a sanitization operation and its recorded post-write verification."

                        }

                    </p>

                </div>


                <div className="grid gap-5 p-5 sm:grid-cols-2 xl:grid-cols-4">

                    <Meta
                        label={
                            isForensic
                                ? "Case ID"
                                : "Request ID"
                        }
                        value={
                            isForensic
                                ? data?.case?.caseId
                                : data?.requestId
                        }
                    />


                    <Meta
                        label={
                            isForensic
                                ? "Run ID"
                                : "Operation ID"
                        }
                        value={
                            isForensic
                                ? certificate?.runId
                                : data?.operationId
                        }
                    />


                    <Meta
                        label="Workstation"
                        value={
                            certificate?.workstationId
                        }
                    />


                    <Meta
                        label={
                            isForensic
                                ? "Source"
                                : "Device"
                        }
                        value={
                            isForensic
                                ? certificate?.sourceName
                                : data?.deviceId
                        }
                    />


                    <Meta
                        label="Model"
                        value={
                            certificate?.model
                        }
                    />


                    <Meta
                        label="Serial Number"
                        value={
                            certificate?.serialNumber
                        }
                    />


                    <Meta
                        label={
                            isForensic
                                ? "Interface"
                                : "Method"
                        }
                        value={
                            isForensic
                                ? certificate?.interfaceType
                                : formatStatus(
                                    data?.method
                                )
                        }
                    />


                    <Meta
                        label="Generated At"
                        value={
                            formatDate(
                                certificate?.generatedAt
                            )
                        }
                    />

                </div>

            </section>


            {isForensic && (

                <section className="grid gap-4 sm:grid-cols-2 xl:grid-cols-4">

                    <Metric
                        label="Bytes scanned"
                        value={
                            number(
                                certificate?.bytesScanned
                            )
                        }
                    />


                    <Metric
                        label="Candidates found"
                        value={
                            number(
                                certificate?.candidatesFound
                            )
                        }
                    />


                    <Metric
                        label="Recovered artifacts"
                        value={
                            number(
                                certificate?.recoveredArtifacts
                            )
                        }
                    />


                    <Metric
                        label="Validated artifacts"
                        value={
                            number(
                                certificate?.validatedArtifacts
                            )
                        }
                    />

                </section>

            )}


            <section className="rounded-2xl border border-slate-200 bg-white shadow-sm">

                <div className="flex flex-col gap-4 border-b border-slate-200 p-5 lg:flex-row lg:items-center lg:justify-between">

                    <div>

                        <div className="flex items-center gap-2">

                            <Hash
                                className="h-5 w-5 text-indigo-600"
                            />

                            <h2 className="text-base font-semibold text-slate-900">
                                Certificate SHA-256
                            </h2>

                        </div>


                        <p className="mt-1 text-sm text-slate-500">
                            The hash stored with the certificate
                            can be recalculated and checked independently.
                        </p>

                    </div>


                    <span
                        className={`inline-flex w-fit rounded-full px-3 py-1.5 text-xs font-semibold ${
                            certificateIntegrityPassed
                                ? "bg-green-50 text-green-700"
                                : "bg-slate-100 text-slate-600"
                        }`}
                    >
                        {
                            certificateIntegrityPassed
                                ? "VALID"
                                : "NOT VERIFIED"
                        }
                    </span>

                </div>


                <div className="space-y-4 p-5">

                    <HashBlock
                        label="Stored certificate hash"
                        value={
                            verification?.storedCertificateHash ||
                            verification?.storedHash ||
                            certificate?.certificateHash
                        }
                    />


                    <HashBlock
                        label="Server calculated hash"
                        value={
                            verification?.calculatedCertificateHash ||
                            verification?.calculatedHash ||
                            certificate?.serverCalculatedCertificateHash
                        }
                    />


                    {verification?.message && (

                        <div className="rounded-xl border border-slate-200 bg-slate-50 p-4 text-sm text-slate-600">
                            {
                                verification.message
                            }
                        </div>

                    )}

                </div>

            </section>


            <section className="rounded-2xl border border-slate-200 bg-white shadow-sm">

                <div className="flex flex-col gap-4 border-b border-slate-200 p-5 lg:flex-row lg:items-center lg:justify-between">

                    <div>

                        <div className="flex items-center gap-2">

                            <Fingerprint
                                className="h-5 w-5 text-indigo-600"
                            />

                            <h2 className="text-base font-semibold text-slate-900">
                                Tamper-Evident Audit Chain
                            </h2>

                        </div>


                        <p className="mt-1 text-sm text-slate-500">

                            {isForensic

                                ? "Native forensic audit events are stored and re-verified as part of the evidence package."

                                : "The sanitization audit ledger is stored separately and can be re-verified against its hash chain."

                            }

                        </p>

                    </div>


                    <button
                        type="button"
                        onClick={
                            handleVerifyAudit
                        }
                        disabled={
                            auditVerifyLoading ||
                            Boolean(
                                data?.certificate?.legacy
                            )
                        }
                        className="inline-flex w-fit items-center gap-2 rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 hover:bg-slate-50 disabled:cursor-not-allowed disabled:opacity-50"
                    >

                        <ShieldCheck
                            className="h-4 w-4"
                        />

                        {
                            auditVerifyLoading
                                ? "Verifying chain..."
                                : "Verify Audit Chain"
                        }

                    </button>

                </div>


                <div className="space-y-4 p-5">

                    {auditError && (

                        <div className="rounded-xl border border-red-200 bg-red-50 p-4 text-sm text-red-700">
                            {
                                auditError
                            }
                        </div>

                    )}


                    {auditLoading ? (

                        <div className="rounded-xl border border-slate-200 bg-slate-50 p-6 text-center text-sm text-slate-500">
                            Loading audit evidence...
                        </div>

                    ) : (

                        <>

                            <div className="grid gap-3 sm:grid-cols-3">

                                <Meta
                                    label="Events"
                                    value={
                                        audit?.eventCount ??
                                        audit?.ledgerEventCount ??
                                        0
                                    }
                                />


                                <Meta
                                    label="Verified events"
                                    value={
                                        audit?.verifiedEventCount ??
                                        audit?.ledgerVerifiedEventCount ??
                                        0
                                    }
                                />


                                <Meta
                                    label="Final event hash"
                                    value={
                                        audit?.finalEventHash ||
                                        certificate?.auditAnchorHash
                                    }
                                />

                            </div>


                            <AuditStatus
                                passed={
                                    auditIntegrityPassed
                                }
                                message={
                                    auditVerification?.message ||
                                    audit?.message ||
                                    audit?.verificationMessage ||
                                    (
                                        auditIntegrityPassed

                                            ? "Audit chain integrity verified."

                                            : "Audit chain has not been independently verified in this view."
                                    )
                                }
                            />


                            {isForensic &&
                                certificate?.auditAnchorHash && (

                                <HashBlock
                                    label="Certificate audit anchor hash"
                                    value={
                                        certificate.auditAnchorHash
                                    }
                                />

                            )}


                            {Array.isArray(
                                audit?.events
                            ) &&
                                audit.events.length >
                                    0 && (

                                <div className="space-y-3">

                                    {audit.events.map(
                                        (
                                            event,
                                            index
                                        ) => (

                                            <AuditEvent
                                                key={
                                                    `${event.sequence}-${event.eventHash || index}`
                                                }
                                                event={
                                                    event
                                                }
                                                index={
                                                    index
                                                }
                                            />

                                        )
                                    )}

                                </div>

                            )}

                        </>

                    )}

                </div>

            </section>


            {isForensic && (

                <section className="rounded-2xl border border-slate-200 bg-white shadow-sm">

                    <div className="border-b border-slate-200 p-5">

                        <div className="flex items-center gap-2">

                            <FileCheck2
                                className="h-5 w-5 text-indigo-600"
                            />

                            <h2 className="text-base font-semibold text-slate-900">
                                Recovered Evidence Bound to Certificate
                            </h2>

                        </div>


                        <p className="mt-1 text-sm text-slate-500">
                            The server verifies recovered artifact
                            bytes against their declared SHA-256 values
                            before the forensic certificate is accepted.
                        </p>

                    </div>


                    <div className="overflow-x-auto">

                        <table className="min-w-[950px] w-full">

                            <thead className="border-b border-slate-200 bg-slate-50">

                                <tr>

                                    <HeaderCell>
                                        Artifact
                                    </HeaderCell>

                                    <HeaderCell>
                                        Type
                                    </HeaderCell>

                                    <HeaderCell>
                                        Offset
                                    </HeaderCell>

                                    <HeaderCell>
                                        Size
                                    </HeaderCell>

                                    <HeaderCell>
                                        Confidence
                                    </HeaderCell>

                                    <HeaderCell>
                                        Validated
                                    </HeaderCell>

                                    <HeaderCell>
                                        SHA-256
                                    </HeaderCell>

                                </tr>

                            </thead>


                            <tbody className="divide-y divide-slate-100">

                                {artifacts.length ===
                                0 ? (

                                    <tr>

                                        <td
                                            colSpan="7"
                                            className="px-5 py-10 text-center text-sm text-slate-500"
                                        >
                                            No recovered artifact metadata
                                            is stored with this certificate.
                                        </td>

                                    </tr>

                                ) : (

                                    artifacts.map(
                                        artifact => (

                                            <tr
                                                key={
                                                    artifact.artifactId
                                                }
                                            >

                                                <td className="px-4 py-4">

                                                    <p className="font-mono text-xs font-semibold text-slate-800">
                                                        {
                                                            artifact.artifactId
                                                        }
                                                    </p>

                                                    <p className="mt-1 text-xs text-slate-500">
                                                        {
                                                            artifact.fileName ||
                                                            "Unnamed artifact"
                                                        }
                                                    </p>

                                                </td>


                                                <td className="px-4 py-4 text-sm text-slate-600">
                                                    {
                                                        artifact.fileType ||
                                                        "—"
                                                    }
                                                </td>


                                                <td className="px-4 py-4 font-mono text-xs text-slate-600">
                                                    {
                                                        number(
                                                            artifact.offset
                                                        )
                                                    }
                                                </td>


                                                <td className="px-4 py-4 text-sm text-slate-600">
                                                    {
                                                        formatBytes(
                                                            artifact.size
                                                        )
                                                    }
                                                </td>


                                                <td className="px-4 py-4">

                                                    <span className="rounded-full bg-slate-100 px-2.5 py-1 text-xs font-semibold text-slate-700">

                                                        {
                                                            artifact.confidenceLevel ||
                                                            "UNKNOWN"
                                                        }

                                                        {" "}

                                                        {
                                                            artifact.confidenceScore ??
                                                            0
                                                        }

                                                    </span>

                                                </td>


                                                <td className="px-4 py-4">

                                                    {artifact.validated ? (

                                                        <CheckCircle2
                                                            className="h-5 w-5 text-green-600"
                                                        />

                                                    ) : (

                                                        <ShieldX
                                                            className="h-5 w-5 text-slate-300"
                                                        />

                                                    )}

                                                </td>


                                                <td className="max-w-[300px] break-all px-4 py-4 font-mono text-[11px] text-slate-500">
                                                    {
                                                        artifact.sha256 ||
                                                        "—"
                                                    }
                                                </td>

                                            </tr>

                                        )
                                    )

                                )}

                            </tbody>

                        </table>

                    </div>

                </section>

            )}


            {isForensic &&
                data?.report?.reportHash && (

                <section className="rounded-2xl border border-amber-200 bg-amber-50 p-5">

                    <p className="text-sm font-semibold text-amber-900">
                        Forensic report integrity is separate
                    </p>


                    <p className="mt-1 text-sm leading-6 text-amber-800">
                        This case also has a generated forensic report.
                        Its report hash identifies the report payload
                        and is not a second forensic certificate.
                    </p>


                    <p className="mt-3 break-all font-mono text-xs text-amber-900">
                        Report SHA-256:
                        {" "}
                        {
                            data.report.reportHash
                        }
                    </p>


                    <Link
                        to="/forensics/reports"
                        className="mt-4 inline-flex items-center rounded-lg border border-amber-200 bg-white px-3 py-2 text-sm font-medium text-amber-900 hover:bg-amber-100"
                    >
                        Open Forensic Reports
                    </Link>

                </section>

            )}


            {isForensic &&
                data?.case?.caseId && (

                <Link
                    to={`/forensics/cases/${encodeURIComponent(
                        data.case.caseId
                    )}`}
                    className="inline-flex items-center gap-2 rounded-lg bg-slate-900 px-4 py-2.5 text-sm font-medium text-white hover:bg-slate-800"
                >
                    Open Related Forensic Case
                </Link>

            )}

        </div>
    );
}


function Meta({
    label,
    value
}) {

    return (
        <div className="min-w-0">

            <p className="text-[11px] font-semibold uppercase tracking-[0.08em] text-slate-400">
                {label}
            </p>


            <p className="mt-1 break-all text-sm font-medium text-slate-700">

                {
                    value ===
                        null ||
                    value ===
                        undefined ||
                    value ===
                        ""
                        ? "—"
                        : value
                }

            </p>

        </div>
    );
}


function Metric({
    label,
    value
}) {

    return (
        <div className="rounded-2xl border border-slate-200 bg-white p-4 shadow-sm">

            <p className="text-[11px] font-semibold uppercase tracking-[0.08em] text-slate-400">
                {label}
            </p>

            <p className="mt-2 text-xl font-semibold text-slate-900">
                {value}
            </p>

        </div>
    );
}


function HashBlock({
    label,
    value
}) {

    return (
        <div>

            <p className="text-[11px] font-semibold uppercase tracking-[0.08em] text-slate-400">
                {label}
            </p>


            <div className="mt-2 rounded-xl border border-slate-200 bg-slate-50 p-4">

                <p className="break-all font-mono text-xs leading-6 text-slate-700">
                    {
                        value ||
                        "Not available"
                    }
                </p>

            </div>

        </div>
    );
}


function AuditStatus({
    passed,
    message
}) {

    return (
        <div
            className={`rounded-xl border p-4 ${
                passed
                    ? "border-green-200 bg-green-50"
                    : "border-slate-200 bg-slate-50"
            }`}
        >

            <div className="flex items-start gap-3">

                {passed ? (

                    <CheckCircle2
                        className="mt-0.5 h-5 w-5 shrink-0 text-green-600"
                    />

                ) : (

                    <ShieldX
                        className="mt-0.5 h-5 w-5 shrink-0 text-slate-400"
                    />

                )}


                <div>

                    <p
                        className={`text-sm font-semibold ${
                            passed
                                ? "text-green-900"
                                : "text-slate-700"
                        }`}
                    >
                        {
                            passed
                                ? "Audit chain verified"
                                : "Audit chain not verified"
                        }
                    </p>


                    <p
                        className={`mt-1 text-sm leading-6 ${
                            passed
                                ? "text-green-800"
                                : "text-slate-500"
                        }`}
                    >
                        {message}
                    </p>

                </div>

            </div>

        </div>
    );
}


function AuditEvent({
    event,
    index
}) {

    const hashValid =
        event.hashValid !==
            false &&

        event.valid !==
            false &&

        event.chainValid !==
            false;


    return (
        <div className="rounded-xl border border-slate-200 bg-white p-4">

            <div className="flex gap-4">

                <div
                    className={`flex h-9 w-9 shrink-0 items-center justify-center rounded-full text-xs font-semibold ${
                        hashValid
                            ? "bg-green-50 text-green-700"
                            : "bg-red-50 text-red-700"
                    }`}
                >
                    {
                        index +
                        1
                    }
                </div>


                <div className="min-w-0 flex-1">

                    <div className="flex flex-col gap-1 sm:flex-row sm:items-center sm:justify-between">

                        <p className="text-sm font-semibold text-slate-900">
                            {
                                event.eventType ||
                                event.action ||
                                "Audit Event"
                            }
                        </p>


                        <p className="text-xs text-slate-400">
                            {
                                formatDate(
                                    event.timestampUtc ||
                                    event.timestamp ||
                                    event.createdAt
                                )
                            }
                        </p>

                    </div>


                    {event.details && (

                        <p className="mt-2 break-words text-sm leading-6 text-slate-600">
                            {
                                event.details
                            }
                        </p>

                    )}


                    <div className="mt-3 grid gap-3 sm:grid-cols-2">

                        <Meta
                            label="Previous hash"
                            value={
                                event.previousEventHash
                            }
                        />


                        <Meta
                            label="Event hash"
                            value={
                                event.eventHash
                            }
                        />

                    </div>

                </div>

            </div>

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


function formatBytes(
    value
) {

    const bytes =
        Number(
            value
        ) || 0;


    if (
        bytes <
        1024
    ) {
        return `${bytes} B`;
    }


    if (
        bytes <
        1024 ** 2
    ) {
        return `${(
            bytes /
            1024
        ).toFixed(1)} KB`;
    }


    if (
        bytes <
        1024 ** 3
    ) {
        return `${(
            bytes /
            1024 ** 2
        ).toFixed(1)} MB`;
    }


    return `${(
        bytes /
        1024 ** 3
    ).toFixed(1)} GB`;
}


function number(
    value
) {

    return new Intl.NumberFormat()
        .format(
            Number(
                value
            ) || 0
        );
}


export default CertificateDetails;