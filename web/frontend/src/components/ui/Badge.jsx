function Badge({ children, variant = "default" }) {
    const styles = {
        default:
            "bg-slate-100 text-slate-700",

        success:
            "bg-green-100 text-green-700",

        warning:
            "bg-yellow-100 text-yellow-700",

        danger:
            "bg-red-100 text-red-700",

        primary:
            "bg-indigo-100 text-indigo-700",
    };

    return (
        <span
            className={`inline-flex rounded-full px-2.5 py-1 text-xs font-medium ${styles[variant]}`}
        >
            {children}
        </span>
    );
}

export default Badge;