import { useEffect, useState } from "react";
import { Link, useParams } from "react-router-dom";

import {
    getSanitizationCertificate,
    verifySanitizationCertificate,
} from "../../../services/sanitizationCertificateService";

function SanitizationCertificate() {

    const {
        certificateId
    } = useParams();

    const [certificate, setCertificate] =
        useState(null);

    const [verification, setVerification] =
        useState(null);

    const [loading, setLoading] =
        useState(true);

    const [verifyLoading, setVerifyLoading] =
        useState(false);

    const [error, setError] =
        useState("");

    const loadCertificate =
        async () => {

            try {

                setLoading(true);
                setError("");

                const data =
                    await getSanitizationCertificate(
                        certificateId
                    );

                setCertificate(
                    data
                );

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

    const handleVerify =
        async () => {

            try {

                setVerifyLoading(true);
                setError("");

                const data =
                    await verifySanitizationCertificate(
                        certificateId
                    );

                setVerification(
                    data
                );

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

    if (loading) {

        return (
            <div className="rounded-lg border border-slate-200 bg-white p-6 shadow-sm">
                <p className="text-sm text-slate-500">
                    Loading certificate...
                </p>
            </div>
        );
    }

    if (error && !certificate) {

        return (
            <div className="space-y-4">

                <div className="rounded-lg border border-red-200 bg-red-50 p-5">
                    <p className="text-sm text-red-700">
                        {error}
                    </p>
                </div>

                <Link
                    to="/workstation-employee/sanitization/history"
                    className="inline-flex rounded-lg border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 hover:bg-slate-50"
                >
                    Back to History
                </Link>

            </div>
        );
    }

    const integrityPassed =
        verification?.valid ??
        verification?.integrityVerified ??
        false;

    return (
        <div className="space-y-6">

            <div className="flex flex-col gap-3 sm:flex-row sm:items-end sm:justify-between">

                <div>
                    <p className="text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                        Certificate Verification
                    </p>

                    <h1 className="mt-1 text-2xl font-semibold text-slate-900">
                        {certificate.certificateId}
                    </h1>

                    <p className="mt-1 text-sm text-slate-500">
                        Cryptographic integrity verification for the sanitization record.
                    </p>
                </div>

                <Link
                    to="/workstation-employee/sanitization/history"
                    className="w-fit rounded-lg border border-slate-300 px-4 py-2 text-sm font-medium text-slate-700 hover:bg-slate-50"
                >
                    Back to History
                </Link>

            </div>

            {error && (
                <div className="rounded-lg border border-red-200 bg-red-50 p-4">
                    <p className="text-sm text-red-700">
                        {error}
                    </p>
                </div>
            )}

            {verification && (
                <div
                    className={`rounded-lg border p-5 ${
                        integrityPassed
                            ? "border-green-200 bg-green-50"
                            : "border-red-200 bg-red-50"
                    }`}
                >
                    <p
                        className={`text-sm font-semibold ${
                            integrityPassed
                                ? "text-green-900"
                                : "text-red-900"
                        }`}
                    >
                        {integrityPassed
                            ? "Certificate integrity verified"
                            : "Certificate integrity verification failed"}
                    </p>

                    <p
                        className={`mt-1 text-sm ${
                            integrityPassed
                                ? "text-green-700"
                                : "text-red-700"
                        }`}
                    >
                        {verification.message ||
                            "The stored certificate hash was checked against the canonical certificate data."}
                    </p>
                </div>
            )}

            <section className="rounded-lg border border-slate-200 bg-white shadow-sm">

                <div className="border-b border-slate-200 px-5 py-4">
                    <h2 className="text-base font-semibold text-slate-900">
                        Certificate Evidence
                    </h2>
                </div>

                <div className="grid gap-6 p-5 md:grid-cols-2">

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
                        value={formatBytes(certificate.capacityBytes)}
                    />

                    <Meta
                        label="Interface"
                        value={certificate.interfaceType}
                    />

                    <Meta
                        label="Method"
                        value={formatMethod(certificate.method)}
                    />

                    <Meta
                        label="Status"
                        value={certificate.status}
                    />

                    <Meta
                        label="Verification"
                        value={certificate.verificationStatus}
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
                        label="Bytes Verified"
                        value={formatBytes(certificate.bytesVerified)}
                    />

                    <Meta
                        label="Verification Samples"
                        value={certificate.verificationSamples}
                    />

                    <Meta
                        label="Hash Algorithm"
                        value={certificate.hashAlgorithm}
                    />

                    <Meta
                        label="Generated At"
                        value={formatDate(certificate.generatedAt)}
                    />

                </div>

                <div className="border-t border-slate-200 px-5 py-5">

                    <p className="text-xs font-medium uppercase tracking-wide text-slate-400">
                        Certificate SHA-256
                    </p>

                    <p className="mt-2 break-all rounded-lg bg-slate-50 p-4 font-mono text-xs leading-5 text-slate-700">
                        {certificate.certificateHash}
                    </p>

                </div>

            </section>

            <section className="rounded-lg border border-indigo-200 bg-indigo-50 p-5">

                <h2 className="text-base font-semibold text-indigo-900">
                    Verify Certificate Integrity
                </h2>

                <p className="mt-1 max-w-3xl text-sm leading-6 text-indigo-700">
                    SecureWipe recalculates the certificate hash from its
                    canonical evidence fields and compares it with the stored
                    SHA-256 value.
                </p>

                <button
                    type="button"
                    disabled={verifyLoading}
                    onClick={handleVerify}
                    className="mt-4 rounded-lg bg-indigo-600 px-4 py-2.5 text-sm font-medium text-white hover:bg-indigo-700 disabled:cursor-not-allowed disabled:opacity-50"
                >
                    {verifyLoading
                        ? "Verifying..."
                        : "Verify Integrity"}
                </button>

            </section>

        </div>
    );
}

function Meta({
    label,
    value,
    mono = false
}) {

    return (
        <div>
            <p className="text-xs font-medium text-slate-400">
                {label}
            </p>

            <p
                className={`mt-1 text-sm text-slate-700 ${
                    mono
                        ? "break-all font-mono text-xs"
                        : ""
                }`}
            >
                {value ?? "—"}
            </p>
        </div>
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

    const value =
        Number(bytes) || 0;

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

export default SanitizationCertificate;