import { ArrowDownToLine, CheckCircle2, ShieldCheck, MonitorDown } from "lucide-react";

const DOWNLOAD_URL = "/downloads/SecureWipe-1.0.0-Windows-x64.zip";

const requirements = [
    "Windows 10 or later",
    "64-bit Windows system",
    "Administrator access for storage operations",
];

export default function Download() {
    return (
        <main className="min-h-screen bg-slate-50 text-slate-900">
            <section className="mx-auto max-w-6xl px-6 py-16 sm:py-20">
                <div className="mx-auto max-w-3xl text-center">
                    <div className="mx-auto mb-5 flex h-14 w-14 items-center justify-center rounded-2xl border border-blue-100 bg-blue-50 text-blue-600">
                        <MonitorDown className="h-7 w-7" />
                    </div>

                    <div className="mb-4 inline-flex items-center gap-2 rounded-full border border-emerald-200 bg-emerald-50 px-3 py-1.5 text-xs font-semibold text-emerald-700">
                        <CheckCircle2 className="h-4 w-4" />
                        Windows desktop application
                    </div>

                    <h1 className="text-4xl font-bold tracking-tight text-slate-950 sm:text-5xl">
                        Download SecureWipe
                    </h1>

                    <p className="mt-5 text-base leading-7 text-slate-600 sm:text-lg">
                        Install the SecureWipe workstation console for controlled
                        storage sanitization, forensic recovery, verification, and
                        auditable operations.
                    </p>
                </div>

                <div className="mx-auto mt-12 max-w-4xl">
                    <div className="overflow-hidden rounded-3xl border border-slate-200 bg-white shadow-sm">
                        <div className="border-b border-slate-200 px-7 py-6 sm:px-9">
                            <div className="flex flex-col gap-5 sm:flex-row sm:items-center sm:justify-between">
                                <div>
                                    <p className="text-sm font-semibold text-blue-600">
                                        Latest release
                                    </p>
                                    <h2 className="mt-1 text-2xl font-bold text-slate-950">
                                        SecureWipe 1.0.0
                                    </h2>
                                    <p className="mt-2 text-sm text-slate-500">
                                        Windows x64 · ZIP package
                                    </p>
                                </div>

                                <a
                                    href={DOWNLOAD_URL}
                                    download
                                    className="inline-flex items-center justify-center gap-2 rounded-xl bg-blue-600 px-5 py-3 text-sm font-semibold text-white shadow-sm transition hover:bg-blue-700 focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2"
                                >
                                    <ArrowDownToLine className="h-4 w-4" />
                                    Download for Windows
                                </a>
                            </div>
                        </div>

                        <div className="grid gap-0 sm:grid-cols-3">
                            <div className="border-b border-slate-200 px-7 py-6 sm:border-b-0 sm:border-r sm:px-9">
                                <p className="text-xs font-semibold uppercase tracking-wide text-slate-400">
                                    Version
                                </p>
                                <p className="mt-2 text-base font-semibold text-slate-900">
                                    1.0.0
                                </p>
                            </div>

                            <div className="border-b border-slate-200 px-7 py-6 sm:border-b-0 sm:border-r sm:px-9">
                                <p className="text-xs font-semibold uppercase tracking-wide text-slate-400">
                                    Platform
                                </p>
                                <p className="mt-2 text-base font-semibold text-slate-900">
                                    Windows x64
                                </p>
                            </div>

                            <div className="px-7 py-6 sm:px-9">
                                <p className="text-xs font-semibold uppercase tracking-wide text-slate-400">
                                    Package
                                </p>
                                <p className="mt-2 text-base font-semibold text-slate-900">
                                    ZIP archive
                                </p>
                            </div>
                        </div>
                    </div>

                    <div className="mt-6 grid gap-6 md:grid-cols-2">
                        <div className="rounded-2xl border border-slate-200 bg-white p-6 shadow-sm">
                            <div className="flex items-center gap-3">
                                <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-blue-50 text-blue-600">
                                    <ShieldCheck className="h-5 w-5" />
                                </div>
                                <h3 className="text-base font-bold text-slate-950">
                                    Included in the package
                                </h3>
                            </div>

                            <div className="mt-5 space-y-3">
                                {[
                                    "SecureWipe workstation executable",
                                    "Required Qt runtime files",
                                    "Windows plugins and dependencies",
                                    "Ready-to-run packaged application",
                                ].map((item) => (
                                    <div
                                        key={item}
                                        className="flex items-start gap-3 text-sm text-slate-600"
                                    >
                                        <CheckCircle2 className="mt-0.5 h-4 w-4 shrink-0 text-emerald-500" />
                                        <span>{item}</span>
                                    </div>
                                ))}
                            </div>
                        </div>

                        <div className="rounded-2xl border border-slate-200 bg-white p-6 shadow-sm">
                            <h3 className="text-base font-bold text-slate-950">
                                Before you install
                            </h3>

                            <div className="mt-5 space-y-3">
                                {requirements.map((item) => (
                                    <div
                                        key={item}
                                        className="flex items-start gap-3 text-sm text-slate-600"
                                    >
                                        <CheckCircle2 className="mt-0.5 h-4 w-4 shrink-0 text-blue-500" />
                                        <span>{item}</span>
                                    </div>
                                ))}
                            </div>
                        </div>
                    </div>

                    <div className="mt-6 rounded-2xl border border-blue-100 bg-blue-50/70 px-6 py-5 text-sm leading-6 text-blue-900">
                        <span className="font-semibold">Installation:</span>{" "}
                        Download the ZIP, extract it to a local folder, and launch
                        <span className="mx-1 rounded bg-white px-1.5 py-0.5 font-mono text-xs text-blue-800">
                            SecureWipe.exe
                        </span>
                        from the extracted package.
                    </div>
                </div>
            </section>
        </main>
    );
}
