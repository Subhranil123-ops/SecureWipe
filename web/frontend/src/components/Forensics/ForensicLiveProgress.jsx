import {
    useEffect,
    useRef,
    useState
} from "react";

import {
    getForensicLiveProgress
} from "../../services/liveProgressService";

function ForensicLiveProgress({
    caseId
}) {
    const [data, setData] =
        useState(null);

    const [error, setError] =
        useState("");

    const [loading, setLoading] =
        useState(true);

    const timerRef =
        useRef(null);

    const cancelledRef =
        useRef(false);

    useEffect(() => {
        cancelledRef.current =
            false;

        if (
            !caseId
        ) {
            setLoading(false);
            return () => {
                cancelledRef.current =
                    true;
            };
        }

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

        const poll = async () => {
            if (
                cancelledRef.current
            ) {
                return;
            }

            try {
                const result =
                    await getForensicLiveProgress(
                        caseId
                    );

                if (
                    cancelledRef.current
                ) {
                    return;
                }

                setData(result);
                setError("");
                setLoading(false);

                const status =
                    String(
                        result?.status ||
                            ""
                    ).toUpperCase();

                if (
                    ![
                        "COMPLETED",
                        "FAILED",
                        "CANCELLED"
                    ].includes(
                        status
                    )
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
                        "Live forensic progress is temporarily unavailable."
                );

                setLoading(false);

                timerRef.current =
                    window.setTimeout(
                        poll,
                        2000
                    );
            }
        };

        clearTimer();
        poll();

        return () => {
            cancelledRef.current =
                true;

            clearTimer();
        };
    }, [caseId]);

    if (
        loading &&
        !data
    ) {
        return (
            <div className="rounded-xl border border-slate-200 bg-white p-4">
                <p className="text-sm text-slate-500">
                    Loading live forensic progress...
                </p>
            </div>
        );
    }

    const progressKnown =
        data?.progressKnown !==
        false;

    const progressValue =
        Number(data?.progress);

    const progress =
        Number.isFinite(
            progressValue
        )
            ? Math.min(
                  100,
                  Math.max(
                      0,
                      progressValue
                  )
              )
            : 0;

    const scanned =
        Number(
            data?.bytesScanned
        ) || 0;

    const total =
        Number(
            data?.totalBytes
        ) || 0;

    const remaining =
        Number(
            data?.remainingBytes
        ) ||
        (
            total > scanned
                ? total - scanned
                : 0
        );

    const status =
        String(
            data?.status ||
                "UNKNOWN"
        ).toUpperCase();

    return (
        <section className="rounded-xl border border-slate-200 bg-white shadow-sm">
            <div className="border-b border-slate-200 px-5 py-4">
                <div className="flex flex-col gap-2 sm:flex-row sm:items-center sm:justify-between">
                    <div>
                        <p className="text-xs font-semibold uppercase tracking-[0.15em] text-indigo-600">
                            Live acquisition
                        </p>

                        <h3 className="mt-1 text-base font-semibold text-slate-900">
                            Forensic Scan Progress
                        </h3>
                    </div>

                    <span className="inline-flex w-fit rounded-full border border-indigo-200 bg-indigo-50 px-3 py-1 text-xs font-semibold text-indigo-700">
                        {status}
                    </span>
                </div>
            </div>

            <div className="p-5">
                <div className="flex items-end justify-between">
                    <p className="text-3xl font-semibold text-slate-900">
                        {progressKnown
                            ? `${progress}%`
                            : "—"}
                    </p>

                    <p className="text-xs text-slate-500">
                        {data?.phase ||
                            "UNKNOWN"}
                    </p>
                </div>

                <div className="mt-3 h-2.5 overflow-hidden rounded-full bg-slate-100">
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

                <div className="mt-4 grid grid-cols-1 gap-3 sm:grid-cols-3">
                    <Metric
                        label="Scanned"
                        value={formatBytes(scanned)}
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

                <div className="mt-4 grid grid-cols-2 gap-3 md:grid-cols-5">
                    <Metric
                        label="Candidates"
                        value={
                            data?.candidatesFound ??
                            0
                        }
                    />

                    <Metric
                        label="Recovered"
                        value={
                            data?.recoveredArtifacts ??
                            0
                        }
                    />

                    <Metric
                        label="Validated"
                        value={
                            data?.validatedArtifacts ??
                            0
                        }
                    />

                    <Metric
                        label="Rejected"
                        value={
                            data?.rejectedArtifacts ??
                            0
                        }
                    />

                    <Metric
                        label="High confidence"
                        value={
                            data?.highConfidenceArtifacts ??
                            0
                        }
                    />
                </div>

                <div className="mt-4 rounded-lg bg-slate-50 p-4">
                    <p className="text-xs font-semibold uppercase tracking-wide text-slate-400">
                        Current activity
                    </p>

                    <p className="mt-1 text-sm text-slate-700">
                        {data?.message ||
                            "Waiting for forensic engine progress."}
                    </p>

                    <p className="mt-3 text-xs font-semibold uppercase tracking-wide text-slate-400">
                        Native operation ID
                    </p>

                    <p className="mt-1 break-all font-mono text-xs text-slate-700">
                        {data?.operationId ||
                            "Not available"}
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
        <div className="rounded-lg border border-slate-200 bg-slate-50 p-3">
            <p className="text-[10px] font-semibold uppercase tracking-wide text-slate-400">
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

export default ForensicLiveProgress;