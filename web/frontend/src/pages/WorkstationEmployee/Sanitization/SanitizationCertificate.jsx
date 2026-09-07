import { useEffect, useState } from "react";
import { Link, useParams } from "react-router-dom";

import {
    getSanitizationCertificate,
    verifySanitizationCertificate,
} from "../../../services/sanitizationCertificateService";

function SanitizationCertificate() {
    const { certificateId } = useParams();

    const [certificate, setCertificate] = useState(null);
    const [verification, setVerification] = useState(null);
    const [loading, setLoading] = useState(true);
    const [verifyLoading, setVerifyLoading] = useState(false);
    const [copyLabel, setCopyLabel] = useState("Copy ID");
    const [error, setError] = useState("");

    const loadCertificate = async () => {
        try {
            setLoading(true);
            setError("");

            const data = await getSanitizationCertificate(
                certificateId
            );

            setCertificate(data);
        } catch (err) {
            console.error(
                "Failed to load certificate:",
                err
            );

            setError(
                err.message ||
                    "Failed to load sanitization certificate."
            );
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        loadCertificate();
    }, [certificateId]);

    const handleVerify = async () => {
        try {
            setVerifyLoading(true);
            setError("");

            const data =
                await verifySanitizationCertificate(
                    certificateId
                );

            setVerification(data);
        } catch (err) {
            console.error(
                "Certificate verification failed:",
                err
            );

            setError(
                err.message ||
                    "Certificate verification failed."
            );
        } finally {
            setVerifyLoading(false);
        }
    };

    const handleCopyCertificateId = async () => {
        if (!certificate?.certificateId) {
            return;
        }

        try {
            await navigator.clipboard.writeText(
                certificate.certificateId
            );

            setCopyLabel("Copied");

            setTimeout(() => {
                setCopyLabel("Copy ID");
            }, 1600);
        } catch (err) {
            console.error(
                "Failed to copy certificate ID:",
                err
            );
        }
    };

    const integrityPassed =
        verification?.valid ??
        verification?.integrityVerified ??
        false;

    if (loading) {
        return (
            <div className="flex min-h-[420px] items-center justify-center">
                <div className="w-full max-w-md rounded-2xl border border-slate-200 bg-white p-8 text-center shadow-sm">
                    <div className="mx-auto flex h-14 w-14 items-center justify-center rounded-2xl bg-indigo-50">
                        <div className="h-6 w-6 animate-spin rounded-full border-2 border-indigo-600 border-t-transparent" />
                    </div>

                    <h2 className="mt-5 text-base font-semibold text-slate-900">
                        Loading certificate
                    </h2>

                    <p className="mt-2 text-sm leading-6 text-slate-500">
                        Retrieving the sanitization certificate and
                        associated evidence.
                    </p>
                </div>
            </div>
        );
    }

    if (error && !certificate) {
        return (
            <div className="space-y-4">
                <div className="rounded-2xl border border-red-200 bg-red-50 p-5">
                    <div className="flex items-start gap-3">
                        <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-full bg-red-100 font-bold text-red-700">
                            !
                        </div>

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
                    to="/workstation-employee/sanitization/history"
                    className="inline-flex rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                >
                    Back to History
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
                        Certificate Verification
                    </div>

                    <h1 className="mt-2 text-2xl font-semibold tracking-tight text-slate-900">
                        Sanitization Certificate
                    </h1>

                    <div className="mt-2 flex flex-wrap items-center gap-2">
                        <span className="break-all font-mono text-xs text-slate-500">
                            {certificate.certificateId}
                        </span>

                        <button
                            type="button"
                            onClick={handleCopyCertificateId}
                            className="rounded-md border border-slate-200 bg-white px-2 py-1 text-[11px] font-medium text-slate-600 transition hover:bg-slate-50"
                        >
                            {copyLabel}
                        </button>
                    </div>
                </div>

                <Link
                    to="/workstation-employee/sanitization/history"
                    className="inline-flex w-fit rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                >
                    Back to History
                </Link>
            </div>

            {error && (
                <div className="rounded-xl border border-red-200 bg-red-50 px-4 py-3">
                    <p className="text-sm text-red-700">
                        {error}
                    </p>
                </div>
            )}

            <section className="overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-sm">
                <div className="border-b border-slate-200 p-5">
                    <div className="flex flex-col gap-5 md:flex-row md:items-center md:justify-between">
                        <div className="flex items-center gap-4">
                            <div className="flex h-14 w-14 shrink-0 items-center justify-center rounded-2xl bg-green-50 text-2xl text-green-600">
                                ✓
                            </div>

                            <div>
                                <p className="text-xs font-semibold uppercase tracking-[0.12em] text-slate-400">
                                    Certificate Status
                                </p>

                                <h2 className="mt-1 text-lg font-semibold text-slate-900">
                                    {certificate.status === "COMPLETED"
                                        ? "Sanitization Completed"
                                        : certificate.status ||
                                          "Certificate Recorded"}
                                </h2>

                                <p className="mt-1 text-sm text-slate-500">
                                    This certificate contains the recorded
                                    evidence for the sanitization operation.
                                </p>
                            </div>
                        </div>

                        <div className="flex flex-wrap gap-2">
                            <StatusPill
                                label={
                                    certificate.status ||
                                    "UNKNOWN"
                                }
                                tone={
                                    certificate.status === "COMPLETED"
                                        ? "success"
                                        : "neutral"
                                }
                            />

                            <StatusPill
                                label={
                                    certificate.verificationStatus ||
                                    "NOT VERIFIED"
                                }
                                tone={
                                    certificate.verificationPassed
                                        ? "success"
                                        : "neutral"
                                }
                            />
                        </div>
                    </div>
                </div>

                <div className="grid gap-6 p-5 sm:grid-cols-2 xl:grid-cols-4">
                    <SummaryCard
                        label="Method"
                        value={formatMethod(
                            certificate.method
                        )}
                    />

                    <SummaryCard
                        label="Verification"
                        value={
                            certificate.verificationPassed
                                ? "Passed"
                                : certificate.verificationStatus ||
                                  "Not Passed"
                        }
                    />

                    <SummaryCard
                        label="Bytes Verified"
                        value={formatBytes(
                            certificate.bytesVerified
                        )}
                    />

                    <SummaryCard
                        label="Generated"
                        value={formatDate(
                            certificate.generatedAt
                        )}
                    />
                </div>
            </section>

            <section className="overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-sm">
                <div className="border-b border-slate-200 px-5 py-4">
                    <h2 className="text-base font-semibold text-slate-900">
                        Certificate Evidence
                    </h2>

                    <p className="mt-1 text-sm text-slate-500">
                        Recorded device, operation, sanitization, and
                        verification information.
                    </p>
                </div>

                <div className="grid gap-x-8 gap-y-6 p-5 md:grid-cols-2 xl:grid-cols-3">
                    <Meta
                        label="Certificate ID"
                        value={certificate.certificateId}
                        mono
                    />

                    <Meta
                        label="Request ID"
                        value={certificate.requestId}
                        mono
                    />

                    <Meta
                        label="Operation ID"
                        value={certificate.operationId}
                        mono
                    />

                    <Meta
                        label="Device ID"
                        value={certificate.deviceId}
                        mono
                    />

                    <Meta
                        label="Model"
                        value={certificate.model}
                    />

                    <Meta
                        label="Serial Number"
                        value={certificate.serialNumber}
                        mono
                    />

                    <Meta
                        label="Capacity"
                        value={formatBytes(
                            certificate.capacityBytes
                        )}
                    />

                    <Meta
                        label="Interface"
                        value={certificate.interfaceType}
                    />

                    <Meta
                        label="Method"
                        value={formatMethod(
                            certificate.method
                        )}
                    />

                    <Meta
                        label="Status"
                        value={certificate.status}
                    />

                    <Meta
                        label="Verification Status"
                        value={
                            certificate.verificationStatus
                        }
                    />

                    <Meta
                        label="Verification Passed"
                        value={
                            certificate.verificationPassed
                                ? "Yes"
                                : "No"
                        }
                    />

                    <Meta
                        label="Verification Samples"
                        value={
                            certificate.verificationSamples
                        }
                    />

                    <Meta
                        label="Bytes Verified"
                        value={formatBytes(
                            certificate.bytesVerified
                        )}
                    />

                    <Meta
                        label="Hash Algorithm"
                        value={certificate.hashAlgorithm}
                    />

                    <Meta
                        label="Generated At"
                        value={formatDate(
                            certificate.generatedAt
                        )}
                    />
                </div>

                <div className="border-t border-slate-200 p-5">
                    <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
                        <div>
                            <p className="text-xs font-semibold uppercase tracking-[0.12em] text-slate-400">
                                Certificate SHA-256
                            </p>

                            <p className="mt-1 text-xs text-slate-500">
                                Cryptographic fingerprint stored with the
                                certificate.
                            </p>
                        </div>

                        <StatusPill
                            label={
                                certificate.hashAlgorithm ||
                                "SHA-256"
                            }
                            tone="neutral"
                        />
                    </div>

                    <div className="mt-4 rounded-xl border border-slate-200 bg-slate-50 p-4">
                        <p className="break-all font-mono text-xs leading-6 text-slate-700">
                            {certificate.certificateHash ||
                                "Hash unavailable"}
                        </p>
                    </div>
                </div>
            </section>

            <section className="overflow-hidden rounded-2xl border border-indigo-200 bg-indigo-50">
                <div className="p-5">
                    <div className="flex flex-col gap-5 lg:flex-row lg:items-center lg:justify-between">
                        <div className="max-w-3xl">
                            <div className="flex items-center gap-2">
                                <div className="flex h-9 w-9 items-center justify-center rounded-lg bg-indigo-100 text-indigo-700">
                                    #
                                </div>

                                <h2 className="text-base font-semibold text-indigo-950">
                                    Verify Certificate Integrity
                                </h2>
                            </div>

                            <p className="mt-3 text-sm leading-6 text-indigo-800">
                                SecureWipe recalculates the certificate
                                fingerprint from the stored evidence and
                                checks it against the recorded SHA-256 hash.
                            </p>
                        </div>

                        <button
                            type="button"
                            disabled={verifyLoading}
                            onClick={handleVerify}
                            className="shrink-0 rounded-lg bg-indigo-600 px-5 py-2.5 text-sm font-medium text-white transition hover:bg-indigo-700 disabled:cursor-not-allowed disabled:opacity-50"
                        >
                            {verifyLoading
                                ? "Verifying..."
                                : "Verify Integrity"}
                        </button>
                    </div>
                </div>
            </section>

            {verification && (
                <section
                    className={`overflow-hidden rounded-2xl border ${
                        integrityPassed
                            ? "border-green-200 bg-green-50"
                            : "border-red-200 bg-red-50"
                    }`}
                >
                    <div className="p-5">
                        <div className="flex items-start gap-4">
                            <div
                                className={`flex h-11 w-11 shrink-0 items-center justify-center rounded-full text-lg font-bold ${
                                    integrityPassed
                                        ? "bg-green-100 text-green-700"
                                        : "bg-red-100 text-red-700"
                                }`}
                            >
                                {integrityPassed
                                    ? "✓"
                                    : "!"}
                            </div>

                            <div className="min-w-0">
                                <div className="flex flex-wrap items-center gap-2">
                                    <h2
                                        className={`text-base font-semibold ${
                                            integrityPassed
                                                ? "text-green-950"
                                                : "text-red-950"
                                        }`}
                                    >
                                        {integrityPassed
                                            ? "Certificate integrity verified"
                                            : "Certificate integrity verification failed"}
                                    </h2>

                                    <StatusPill
                                        label={
                                            integrityPassed
                                                ? "VALID"
                                                : "INVALID"
                                        }
                                        tone={
                                            integrityPassed
                                                ? "success"
                                                : "danger"
                                        }
                                    />
                                </div>

                                <p
                                    className={`mt-2 text-sm leading-6 ${
                                        integrityPassed
                                            ? "text-green-800"
                                            : "text-red-800"
                                    }`}
                                >
                                    {verification.message ||
                                        "The stored certificate hash was checked against the canonical certificate data."}
                                </p>
                            </div>
                        </div>
                    </div>
                </section>
            )}

            <div className="flex flex-wrap gap-3">
                <Link
                    to={`/workstation-employee/sanitization/${certificate.requestId}`}
                    className="inline-flex rounded-lg bg-slate-900 px-4 py-2.5 text-sm font-medium text-white transition hover:bg-slate-800"
                >
                    View Sanitization Job
                </Link>

                <Link
                    to="/workstation-employee/sanitization/history"
                    className="inline-flex rounded-lg border border-slate-300 bg-white px-4 py-2.5 text-sm font-medium text-slate-700 transition hover:bg-slate-50"
                >
                    Back to History
                </Link>
            </div>
        </div>
    );
}

function SummaryCard({ label, value }) {
    return (
        <div className="rounded-xl border border-slate-200 bg-slate-50 p-4">
            <p className="text-xs font-medium uppercase tracking-wide text-slate-400">
                {label}
            </p>

            <p className="mt-2 break-words text-sm font-semibold text-slate-800">
                {value ?? "—"}
            </p>
        </div>
    );
}

function Meta({
    label,
    value,
    mono = false,
}) {
    return (
        <div className="min-w-0">
            <p className="text-xs font-medium uppercase tracking-wide text-slate-400">
                {label}
            </p>

            <p
                className={`mt-1 break-words text-sm text-slate-700 ${
                    mono
                        ? "font-mono text-xs"
                        : ""
                }`}
            >
                {value ?? "—"}
            </p>
        </div>
    );
}

function StatusPill({
    label,
    tone = "neutral",
}) {
    const styles = {
        success:
            "border-green-200 bg-green-100 text-green-700",
        danger:
            "border-red-200 bg-red-100 text-red-700",
        neutral:
            "border-slate-200 bg-slate-100 text-slate-600",
    };

    return (
        <span
            className={`inline-flex items-center rounded-full border px-2.5 py-1 text-[11px] font-semibold tracking-wide ${
                styles[tone] ||
                styles.neutral
            }`}
        >
            {label || "UNKNOWN"}
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

function formatBytes(bytes) {
    const value = Number(bytes) || 0;

    if (value < 1024) {
        return `${value} B`;
    }

    if (value < 1024 ** 2) {
        return `${(
            value / 1024
        ).toFixed(1)} KB`;
    }

    if (value < 1024 ** 3) {
        return `${(
            value / 1024 ** 2
        ).toFixed(1)} MB`;
    }

    return `${(
        value / 1024 ** 3
    ).toFixed(2)} GB`;
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

export default SanitizationCertificate;