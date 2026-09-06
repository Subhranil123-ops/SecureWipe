import { Link } from "react-router-dom";

function SanitizationCertificateCard({
    certificate
}) {

    if (!certificate) {
        return null;
    }

    return (
        <div className="rounded-lg border border-green-200 bg-green-50 shadow-sm">

            <div className="border-b border-green-200 px-5 py-4">

                <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">

                    <div>
                        <h2 className="text-base font-semibold text-green-900">
                            Sanitization Certificate
                        </h2>

                        <p className="mt-1 text-sm text-green-700">
                            Evidence-backed sanitization record.
                        </p>
                    </div>

                    <span className="inline-flex w-fit rounded-full bg-green-100 px-3 py-1 text-xs font-semibold text-green-700">
                        VERIFIED
                    </span>

                </div>

            </div>

            <div className="grid gap-5 p-5 md:grid-cols-2 xl:grid-cols-3">

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
                    label="Device"
                    value={certificate.model}
                />

                <Meta
                    label="Serial Number"
                    value={certificate.serialNumber}
                    mono
                />

                <Meta
                    label="Sanitization Method"
                    value={formatMethod(certificate.method)}
                />

                <Meta
                    label="Verification"
                    value={certificate.verificationStatus}
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
                    label="Generated At"
                    value={formatDate(certificate.generatedAt)}
                />

            </div>

            <div className="border-t border-green-200 px-5 py-4">

                <p className="text-xs font-medium uppercase tracking-wide text-green-700">
                    Certificate Hash
                </p>

                <p className="mt-2 break-all font-mono text-xs leading-5 text-green-800">
                    {certificate.certificateHash || "—"}
                </p>

                <div className="mt-4">

                    <Link
                        to={`/workstation-employee/sanitization/certificate/${certificate.certificateId}`}
                        className="inline-flex rounded-lg bg-green-700 px-4 py-2 text-sm font-medium text-white hover:bg-green-800"
                    >
                        View & Verify Certificate
                    </Link>

                </div>

            </div>

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
            <p className="text-xs font-medium text-green-700">
                {label}
            </p>

            <p
                className={`mt-1 text-sm text-green-900 ${
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

export default SanitizationCertificateCard;