import React from "react";

function SanitizationPipeline({ status }) {
    const steps = [
        {
            key: "ASSIGNED",
            label: "Assigned",
            description: "Request assigned to workstation employee",
        },
        {
            key: "IN_PROGRESS",
            label: "Sanitizing",
            description: "Desktop workstation is performing sanitization",
        },
        {
            key: "VERIFYING",
            label: "Verifying",
            description: "Sanitization result is being validated",
        },
        {
            key: "COMPLETED",
            label: "Completed",
            description: "Sanitization and verification completed",
        },
    ];

    const statusOrder = {
        ASSIGNED: 0,
        IN_PROGRESS: 1,
        VERIFYING: 2,
        COMPLETED: 3,
    };

    const currentIndex =
        statusOrder[status] ??
        (status === "FAILED" ? 1 : 0);

    const isFailed = status === "FAILED";

    return (
        <section className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
            <div className="mb-5">
                <h2 className="text-base font-semibold text-slate-900">
                    Sanitization Pipeline
                </h2>

                <p className="mt-1 text-sm text-slate-500">
                    Track the current state of the assigned sanitization workflow.
                </p>
            </div>

            <div className="grid gap-3 md:grid-cols-4">
                {steps.map((step, index) => {
                    const completed =
                        !isFailed &&
                        index < currentIndex;

                    const active =
                        !isFailed &&
                        index === currentIndex;

                    const failed =
                        isFailed &&
                        index === 1;

                    let containerClass =
                        "border-slate-200 bg-white";

                    let numberClass =
                        "border-slate-300 bg-white text-slate-500";

                    let titleClass =
                        "text-slate-700";

                    let descriptionClass =
                        "text-slate-400";

                    if (completed) {
                        containerClass =
                            "border-green-200 bg-green-50";

                        numberClass =
                            "border-green-200 bg-green-100 text-green-700";

                        titleClass =
                            "text-green-800";

                        descriptionClass =
                            "text-green-700";
                    }

                    if (active) {
                        containerClass =
                            "border-indigo-200 bg-indigo-50";

                        numberClass =
                            "border-indigo-200 bg-indigo-100 text-indigo-700";

                        titleClass =
                            "text-indigo-900";

                        descriptionClass =
                            "text-indigo-700";
                    }

                    if (failed) {
                        containerClass =
                            "border-red-200 bg-red-50";

                        numberClass =
                            "border-red-200 bg-red-100 text-red-700";

                        titleClass =
                            "text-red-900";

                        descriptionClass =
                            "text-red-700";
                    }

                    return (
                        <div
                            key={step.key}
                            className={`rounded-lg border p-4 ${containerClass}`}
                        >
                            <div className="flex items-start gap-3">
                                <div
                                    className={`flex h-8 w-8 shrink-0 items-center justify-center rounded-full border text-xs font-semibold ${numberClass}`}
                                >
                                    {failed
                                        ? "!"
                                        : completed
                                            ? "✓"
                                            : index + 1}
                                </div>

                                <div className="min-w-0">
                                    <p
                                        className={`text-sm font-semibold ${titleClass}`}
                                    >
                                        {failed
                                            ? "Failed"
                                            : step.label}
                                    </p>

                                    <p
                                        className={`mt-1 text-xs leading-5 ${descriptionClass}`}
                                    >
                                        {failed
                                            ? "The desktop sanitization operation failed."
                                            : step.description}
                                    </p>
                                </div>
                            </div>
                        </div>
                    );
                })}
            </div>
        </section>
    );
}

export default SanitizationPipeline;