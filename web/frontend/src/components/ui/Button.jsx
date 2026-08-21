function Button({
    children,
    type = "button",
    onClick,
    disabled = false,
    variant = "primary",
    className = "",
}) {
    const base =
        "rounded-lg px-4 py-2 text-sm font-medium transition-colors focus:outline-none focus:ring-2 focus:ring-indigo-500 focus:ring-offset-1 disabled:cursor-not-allowed disabled:opacity-50";

    const variants = {
        primary:
            "bg-indigo-600 text-white hover:bg-indigo-700",

        secondary:
            "border border-slate-300 bg-white text-slate-700 hover:bg-slate-50",

        danger:
            "bg-red-600 text-white hover:bg-red-700",
    };

    return (
        <button
            type={type}
            onClick={onClick}
            disabled={disabled}
            className={`${base} ${variants[variant]} ${className}`}
        >
            {children}
        </button>
    );
}

export default Button;