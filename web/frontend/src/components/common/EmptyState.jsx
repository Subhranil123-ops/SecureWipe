function EmptyState({
    title = "No data available",
    message = "There is nothing to display yet.",
}) {
    return (
        <div className="rounded-lg border border-slate-200 bg-white p-6 text-center">
            <h3 className="text-sm font-semibold text-slate-800">
                {title}
            </h3>

            <p className="mt-1 text-sm text-slate-500">
                {message}
            </p>
        </div>
    );
}

export default EmptyState;