import {
    useEffect,
    useRef,
    useState
} from "react";

import {
    getSanitizationLiveProgress
} from "../../services/liveProgressService";

function SanitizationLiveProgress({
    requestId,
    requestStatus
}) {
    const [progressData, setProgressData] =
        useState(null);

    const [loading, setLoading] =
        useState(true);

    const [error, setError] =
        useState("");

    const timerRef =
        useRef(null);

    const cancelledRef =
        useRef(false);

    useEffect(() => {
        cancelledRef.current =
            false;

        const clearTimer = () => {
            if (
                timerRef.current !==
                null
            ) {
                window.clearTimeout(
                    timerRef.current
                );

                timerRef.current =
                    null;
            }
        };

        clearTimer();

        if (
            !requestId
        ) {
            setProgressData(null);
            setLoading(false);
            return () => {
                clearTimer();
                cancelledRef.current =
                    true;
            };
        }

        const poll = async () => {
            if (
                cancelledRef.current
            ) {
                return;
            }

            try {
                const result =
                    await getSanitizationLiveProgress(
                        requestId
                    );

                if (
                    cancelledRef.current
                ) {
                    return;
                }

                setProgressData(
                    result
                );

                setError("");
                setLoading(false);

                const terminal =
                    [
                        "COMPLETED",
                        "FAILED",
                        "CANCELLED"
                    ].includes(
                        String(
                            result?.status ||
                                ""
                        ).toUpperCase()
                    );

                const requestTerminal =
                    [
                        "COMPLETED",
                        "FAILED",
                        "CANCELLED"
                    ].includes(
                        String(
                            requestStatus ||
                                ""
                        ).toUpperCase()
                    );

                if (
                    !terminal &&
                    !requestTerminal
                ) {
                    timerRef.current =
                        window.setTimeout(
                            poll,
                            1000
                        );
                }
            } catch (pollError) {
                if (
                    cancelledRef.current
                ) {
                    return;
                }

                setError(
                    pollError?.message ||
                        "Live progress is temporarily unavailable."
                );

                setLoading(false);

                timerRef.current =
                    window.setTimeout(
                        poll,
                        2000
                    );
            }
        };

        poll();

        return () => {
            cancelledRef.current =
                true;

            clearTimer();
        };
    }, [
        requestId,
        requestStatus
    ]);

    if (
        !progressData &&
        loading
    ) {
        return (
            <section className="rounded-2xl border border-slate-200 bg-white p-5 shadow-sm">
                <div className="flex items-center gap-3">
                    <div className="h-5 w-5 animate-spin rounded-full border-2 border-indigo-600 border-t-transparent" />

                    <div>
                        <h2 className="text-base font-semibold text-slate-900">
                            Live sanitization progress
                        </h2>

                        <p className="mt-1 text-sm text-slate-500">
                            Waiting for the desktop workstation to publish live progress.
                        </p>
                    </div>
                </div>
            </section>
        );
    }

    const progressKnown =
        progressData?.progressKnown !==
        false;

    const rawProgress =
        Number(
            progressData?.progress
        );

    const progress =
        Number.isFinite(
            rawProgress
        )
            ? Math.min(
                  100,
                  Math.max(
                      0,
                      rawProgress
                  )
              )
            : 0;

    const processed =
        Number(
            progressData?.processedBytes
        ) || 0;

    const total =
        Number(
            progressData?.totalBytes
        ) || 0;

    const remaining =
        Number(
            progressData?.remainingBytes
        ) ||
        (
            total > processed
                ? total - processed
                : 0
        );

    const status =
        String(
            progressData?.status ||
                requestStatus ||
                "UNKNOWN"
        ).toUpperCase();

    return (
        <section className="overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-sm">
            <div className="border-b border-slate-200 px-5 py-4">
                <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
                    <div>
                        <p className="text-xs font-semibold uppercase tracking-[0.16em] text-indigo-600">
                            Live Operation
                        </p>

                        <h2 className="mt-1 text-base font-semibold text-slate-900">
                            Sanitization Progress
                        </h2>
                    </div>

                    <span
                        className={`inline-flex w-fit items-center rounded-full border px-3 py-1 text-xs font-semibold ${
                            status ===
                            "COMPLETED"
                                ? "border-green-200 bg-green-50 text-green-700"
                                : status ===
                                  "FAILED"
                                ? "border-red-200 bg-red-50 text-red-700"
                                : "border-indigo-200 bg-indigo-50 text-indigo-700"
                        }`}
                    >
                        {status}
                    </span>
                </div>
            </div>

            <div className="p-5">
                <div className="flex items-end justify-between gap-4">
                    <div>
                        <p className="text-xs font-medium uppercase tracking-wide text-slate-400">
                            Progress
                        </p>

                        <p className="mt-1 text-3xl font-semibold text-slate-900">
                            {progressKnown
                                ? `${progress}%`
                                : "—"}
                        </p>
                    </div>

                    <div className="text-right text-xs text-slate-500">
                        <p>
                            Phase:{" "}
                            <span className="font-semibold text-slate-700">
                                {progressData?.phase ||
                                    "UNKNOWN"}
                            </span>
                        </p>

                        <p className="mt-1">
                            Last update:{" "}
                            <span className="font-semibold text-slate-700">
                                {formatDate(
                                    progressData?.lastProgressAt
                                )}
                            </span>
                        </p>
                    </div>
                </div>

                <div className="mt-4 h-3 overflow-hidden rounded-full bg-slate-100">
                    {progressKnown ? (
                        <div
                            className="h-full rounded-full bg-indigo-600 transition-all duration-500"
                            style={{
                                width: `${progress}%`
                            }}
                        />
                    ) : (
                        <div className="h-full w-1/3 animate-pulse rounded-full bg-indigo-400" />
                    )}
                </div>

                <div className="mt-5 grid grid-cols-1 gap-3 sm:grid-cols-3">
                    <Metric
                        label="Processed"
                        value={formatBytes(
                            processed
                        )}
                    />

                    <Metric
                        label="Total"
                        value={
                            progressKnown
                                ? formatBytes(
                                      total
                                  )
                                : "Unknown"
                        }
                    />

                    <Metric
                        label="Remaining"
                        value={
                            progressKnown
                                ? formatBytes(
                                      remaining
                                  )
                                : "Unknown"
                        }
                    />
                </div>

                <div className="mt-5 rounded-xl border border-slate-200 bg-slate-50 p-4">
                    <p className="text-xs font-semibold uppercase tracking-wide text-slate-400">
                        Operation ID
                    </p>

                    <p className="mt-1 break-all font-mono text-xs text-slate-700">
                        {progressData?.operationId ||
                            "Waiting for native operation ID"}
                    </p>

                    <p className="mt-4 text-xs font-semibold uppercase tracking-wide text-slate-400">
                        Current message
                    </p>

                    <p className="mt-1 text-sm leading-6 text-slate-700">
                        {progressData?.message ||
                            "The desktop workstation has not published a detailed message yet."}
                    </p>
                </div>

                {error && (
                    <p className="mt-3 text-xs text-amber-700">
                        {error}
                    </p>
                )}
            </div>
        </section>
    );
}

function Metric({
    label,
    value
}) {
    return (
        <div className="rounded-xl border border-slate-200 bg-slate-50 p-3">
            <p className="text-[11px] font-semibold uppercase tracking-wide text-slate-400">
                {label}
            </p>

            <p className="mt-1 text-sm font-semibold text-slate-800">
                {value}
            </p>
        </div>
    );
}

function formatBytes(
    bytes
) {
    const value =
        Number(bytes) || 0;

    if (
        value <
        1024
    ) {
        return `${value} B`;
    }

    if (
        value <
        1024 ** 2
    ) {
        return `${(
            value / 1024
        ).toFixed(1)} KB`;
    }

    if (
        value <
        1024 ** 3
    ) {
        return `${(
            value / 1024 ** 2
        ).toFixed(1)} MB`;
    }

    return `${(
        value / 1024 ** 3
    ).toFixed(2)} GB`;
}

function formatDate(
    value
) {
    if (!value) {
        return "—";
    }

    const date =
        new Date(value);

    if (
        Number.isNaN(
            date.getTime()
        )
    ) {
        return "—";
    }

    return date.toLocaleString();
}

export default SanitizationLiveProgress;