import {
    useEffect,
    useState
} from "react";

import {
    useParams
} from "react-router-dom";

import {
    getForensicArtifactBlob
} from "../../services/forensicService";

function ForensicEvidenceDrawer({
    artifact,
    onClose
}) {
    const {
        caseId: routeCaseId
    } = useParams();

    const [
        previewUrl,
        setPreviewUrl
    ] = useState("");

    const [
        previewLoading,
        setPreviewLoading
    ] = useState(false);

    const [
        previewError,
        setPreviewError
    ] = useState("");

    const caseId =
        routeCaseId ||
        artifact?.caseId ||
        "";

    useEffect(() => {
        let cancelled =
            false;

        let objectUrl =
            "";

        const loadContent =
            async () => {
                setPreviewUrl("");
                setPreviewError("");

                if (
                    !artifact ||
                    !caseId ||
                    !artifact.artifactId ||
                    !artifact.contentAvailable
                ) {
                    setPreviewLoading(
                        false
                    );
                    return;
                }

                setPreviewLoading(
                    true
                );

                try {
                    const blob =
                        await getForensicArtifactBlob(
                            caseId,
                            artifact.artifactId
                        );

                    if (
                        cancelled
                    ) {
                        return;
                    }

                    objectUrl =
                        URL.createObjectURL(
                            blob
                        );

                    setPreviewUrl(
                        objectUrl
                    );
                } catch (
                    error
                ) {
                    if (
                        cancelled
                    ) {
                        return;
                    }

                    setPreviewError(
                        error?.message ||
                            "Unable to load recovered artifact content."
                    );
                } finally {
                    if (
                        !cancelled
                    ) {
                        setPreviewLoading(
                            false
                        );
                    }
                }
            };

        loadContent();

        return () => {
            cancelled =
                true;

            if (
                objectUrl
            ) {
                URL.revokeObjectURL(
                    objectUrl
                );
            }
        };
    }, [
        artifact,
        caseId
    ]);

    if (
        !artifact
    ) {
        return null;
    }

    const fileType =
        String(
            artifact.fileType ||
                ""
        ).toUpperCase();

    const contentType =
        String(
            artifact.contentMimeType ||
                ""
        ).toLowerCase();

    const isImage =
        contentType.startsWith(
            "image/"
        ) ||
        [
            "JPEG",
            "JPG",
            "PNG"
        ].includes(
            fileType
        );

    return (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-slate-950/50 px-4 py-6 backdrop-blur-sm">
            <div className="max-h-[92vh] w-full max-w-5xl overflow-hidden rounded-2xl border border-slate-200 bg-white shadow-2xl">
                <div className="flex items-start justify-between border-b border-slate-200 px-5 py-4">
                    <div className="min-w-0">
                        <p className="text-xs font-semibold uppercase tracking-[0.18em] text-indigo-600">
                            Recovered Evidence
                        </p>

                        <h2 className="mt-1 truncate text-lg font-semibold text-slate-900">
                            {artifact.fileName ||
                                "Recovered artifact"}
                        </h2>

                        <p className="mt-1 font-mono text-xs text-slate-500">
                            {artifact.artifactId ||
                                "—"}
                        </p>
                    </div>

                    <button
                        type="button"
                        onClick={
                            onClose
                        }
                        className="ml-4 rounded-lg px-3 py-1 text-2xl leading-none text-slate-400 hover:bg-slate-100 hover:text-slate-700"
                        aria-label="Close evidence viewer"
                    >
                        ×
                    </button>
                </div>

                <div className="max-h-[calc(92vh-90px)] overflow-y-auto p-5">
                    <section className="rounded-xl border border-slate-200 bg-slate-50 p-4">
                        <div className="flex flex-col gap-3 sm:flex-row sm:items-center sm:justify-between">
                            <div>
                                <p className="text-sm font-semibold text-slate-900">
                                    Recovered file content
                                </p>

                                <p className="mt-1 text-xs text-slate-500">
                                    Content is loaded from the server-side forensic evidence store and independently served through the protected artifact-content endpoint.
                                </p>
                            </div>

                            {previewUrl && (
                                <a
                                    href={
                                        previewUrl
                                    }
                                    download={
                                        artifact.fileName ||
                                        "recovered-artifact"
                                    }
                                    className="inline-flex items-center justify-center rounded-lg bg-indigo-600 px-4 py-2 text-sm font-medium text-white hover:bg-indigo-700"
                                >
                                    Download
                                </a>
                            )}
                        </div>
                    </section>

                    {previewLoading && (
                        <div className="mt-5 rounded-xl border border-slate-200 bg-white p-8 text-center">
                            <div className="mx-auto h-7 w-7 animate-spin rounded-full border-2 border-indigo-600 border-t-transparent" />

                            <p className="mt-3 text-sm text-slate-600">
                                Loading recovered artifact...
                            </p>
                        </div>
                    )}

                    {!previewLoading &&
                        previewError && (
                            <div className="mt-5 rounded-xl border border-red-200 bg-red-50 p-5">
                                <p className="text-sm font-semibold text-red-900">
                                    Preview unavailable
                                </p>

                                <p className="mt-1 text-sm text-red-700">
                                    {
                                        previewError
                                    }
                                </p>
                            </div>
                        )}

                    {!previewLoading &&
                        !previewError &&
                        previewUrl &&
                        isImage && (
                            <div className="mt-5 overflow-hidden rounded-xl border border-slate-200 bg-slate-950 p-3">
                                <img
                                    src={
                                        previewUrl
                                    }
                                    alt={
                                        artifact.fileName ||
                                        "Recovered forensic evidence"
                                    }
                                    className="mx-auto max-h-[58vh] max-w-full object-contain"
                                />
                            </div>
                        )}

                    {!previewLoading &&
                        !previewError &&
                        previewUrl &&
                        !isImage && (
                            <div className="mt-5 rounded-xl border border-slate-200 bg-white p-6">
                                <p className="text-sm font-semibold text-slate-900">
                                    File content stored successfully
                                </p>

                                <p className="mt-1 text-sm text-slate-500">
                                    Inline preview is not available for this file type, but the verified recovered bytes are available through the Download button.
                                </p>
                            </div>
                        )}

                    {!artifact.contentAvailable &&
                        !previewLoading && (
                            <div className="mt-5 rounded-xl border border-dashed border-slate-300 bg-slate-50 px-6 py-12 text-center">
                                <p className="text-sm font-semibold text-slate-900">
                                    Evidence content is not stored
                                </p>

                                <p className="mt-2 text-xs leading-5 text-slate-500">
                                    The case currently contains artifact metadata only. The production native evidence-package submission must be used for server-side content storage.
                                </p>
                            </div>
                        )}

                    <div className="mt-5 grid grid-cols-1 gap-4 sm:grid-cols-2 lg:grid-cols-3">
                        <Meta
                            label="Case ID"
                            value={
                                caseId
                            }
                        />

                        <Meta
                            label="File type"
                            value={
                                artifact.fileType
                            }
                        />

                        <Meta
                            label="Content type"
                            value={
                                artifact.contentMimeType ||
                                "—"
                            }
                        />

                        <Meta
                            label="Size"
                            value={formatBytes(
                                artifact.size
                            )}
                        />

                        <Meta
                            label="Stored bytes"
                            value={formatBytes(
                                artifact.contentLength
                            )}
                        />

                        <Meta
                            label="Offset"
                            value={formatOffset(
                                artifact.offset
                            )}
                        />

                        <Meta
                            label="Validation"
                            value={
                                artifact.validated
                                    ? "VALIDATED"
                                    : "NOT VALIDATED"
                            }
                        />

                        <Meta
                            label="Confidence"
                            value={`${artifact.confidenceLevel || "LOW"} (${artifact.confidenceScore ?? 0}%)`}
                        />

                        <Meta
                            label="Server content"
                            value={
                                artifact.contentAvailable
                                    ? "AVAILABLE"
                                    : "NOT STORED"
                            }
                        />
                    </div>

                    <div className="mt-5 rounded-xl border border-slate-200 bg-slate-50 p-4">
                        <p className="text-xs font-semibold uppercase tracking-wide text-slate-500">
                            Artifact SHA-256
                        </p>

                        <p className="mt-2 break-all font-mono text-xs leading-5 text-slate-700">
                            {artifact.sha256 ||
                                "Not available"}
                        </p>

                        {artifact.contentSha256 && (
                            <>
                                <p className="mt-4 text-xs font-semibold uppercase tracking-wide text-slate-500">
                                    Stored Content SHA-256
                                </p>

                                <p className="mt-2 break-all font-mono text-xs leading-5 text-slate-700">
                                    {
                                        artifact.contentSha256
                                    }
                                </p>
                            </>
                        )}
                    </div>

                    {Array.isArray(
                        artifact.confidenceReasons
                    ) &&
                        artifact
                            .confidenceReasons
                            .length >
                            0 && (
                            <div className="mt-5 rounded-xl border border-slate-200 bg-white p-4">
                                <p className="text-sm font-semibold text-slate-900">
                                    Confidence reasons
                                </p>

                                <div className="mt-3 space-y-2">
                                    {artifact.confidenceReasons.map(
                                        (
                                            reason,
                                            index
                                        ) => (
                                            <p
                                                key={`${index}-${reason}`}
                                                className="text-xs leading-5 text-slate-600"
                                            >
                                                •{" "}
                                                {
                                                    reason
                                                }
                                            </p>
                                        )
                                    )}
                                </div>
                            </div>
                        )}
                </div>
            </div>
        </div>
    );
}

function Meta({
    label,
    value
}) {
    return (
        <div className="rounded-lg border border-slate-200 bg-white p-3">
            <p className="text-[11px] font-semibold uppercase tracking-wide text-slate-400">
                {label}
            </p>

            <p className="mt-1 break-words text-sm text-slate-700">
                {value || "—"}
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
            value /
            1024 ** 2
        ).toFixed(1)} MB`;
    }

    return `${(
        value /
        1024 ** 3
    ).toFixed(2)} GB`;
}

function formatOffset(
    offset
) {
    const value =
        Number(offset) || 0;

    return `0x${value
        .toString(16)
        .toUpperCase()}`;
}

export default ForensicEvidenceDrawer;