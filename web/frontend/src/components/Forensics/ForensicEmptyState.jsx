function ForensicEmptyState({
    title = "No data available",
    description =
        "There is nothing to display yet.",
    action = null
}) {
    return (
        <div className="flex min-h-[220px] items-center justify-center rounded-xl border border-dashed border-slate-300 bg-white p-8">
            <div className="max-w-md text-center">
                <div className="mx-auto flex h-12 w-12 items-center justify-center rounded-2xl bg-slate-100 text-slate-500">
                    <svg
                        viewBox="0 0 24 24"
                        className="h-6 w-6"
                        fill="none"
                        stroke="currentColor"
                        strokeWidth="1.7"
                    >
                        <path d="M7 4h10a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H7a2 2 0 0 1-2-2V6a2 2 0 0 1 2-2Z" />
                        <path d="M9 9h6M9 13h6M9 17h3" />
                    </svg>
                </div>

                <h3 className="mt-4 text-sm font-semibold text-slate-900">
                    {title}
                </h3>

                <p className="mt-2 text-sm leading-6 text-slate-500">
                    {description}
                </p>

                {action ? (
                    <div className="mt-5">
                        {action}
                    </div>
                ) : null}
            </div>
        </div>
    );
}

export default ForensicEmptyState;