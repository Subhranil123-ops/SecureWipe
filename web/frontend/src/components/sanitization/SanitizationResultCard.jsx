function SanitizationResultCard({
    result
}) {

    if (!result) {
        return (
            <div className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
                <h2 className="text-base font-semibold text-slate-900">
                    Sanitization Result
                </h2>

                <p className="mt-2 text-sm text-slate-500">
                    No sanitization result has been received yet.
                </p>
            </div>
        );
    }

    const verificationPassed =
        result.verificationPassed === true ||
        result.verificationStatus === "PASSED";

    return (
        <div className="rounded-lg border border-slate-200 bg-white shadow-sm">

            <div className="border-b border-slate-200 px-5 py-4">
                <h2 className="text-base font-semibold text-slate-900">
                    Sanitization Result
                </h2>

                <p className="mt-1 text-sm text-slate-500">
                    Structured evidence received from the sanitization engine.
                </p>
            </div>

            <div className="grid gap-5 p-5 md:grid-cols-2 xl:grid-cols-3">

                <Meta
                    label="Operation ID"
                    value={result.operationId}
                    mono
                />

                <Meta
                    label="Method"
                    value={formatMethod(result.method)}
                />

                <Meta
                    label="Status"
                    value={result.status}
                />

                <Meta
                    label="Bytes Processed"
                    value={formatBytes(result.bytesProcessed)}
                />

                <Meta
                    label="Duration"
                    value={`${result.operationDurationMs || 0} ms`}
                />

                <Meta
                    label="Verification"
                    value={result.verificationStatus}
                />

                <Meta
                    label="Verification Performed"
                    value={result.verificationPerformed ? "Yes" : "No"}
                />

                <Meta
                    label="Verification Passed"
                    value={verificationPassed ? "Yes" : "No"}
                />

                <Meta
                    label="Bytes Verified"
                    value={formatBytes(result.bytesVerified)}
                />

                <Meta
                    label="Verification Samples"
                    value={result.verificationSamples ?? 0}
                />

                <Meta
                    label="Native Error Code"
                    value={result.nativeErrorCode ?? 0}
                    mono
                />

                <Meta
                    label="Device Reported Success"
                    value={
                        result.deviceReportedSuccess
                            ? "Yes"
                            : "No"
                    }
                />

            </div>

            <div className="border-t border-slate-200 px-5 py-4">

                <p className="text-xs font-medium uppercase tracking-wide text-slate-400">
                    Verification Evidence
                </p>

                <p
                    className={`mt-2 text-sm ${
                        verificationPassed
                            ? "text-green-700"
                            : "text-red-600"
                    }`}
                >
                    {result.verificationMessage ||
                        "No verification message provided."}
                </p>

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

export default SanitizationResultCard;