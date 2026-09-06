import { NavLink, useNavigate } from "react-router-dom";
import { useAuth } from "../../context/AuthContext";

function Sidebar() {
    const { user, logout } = useAuth();
    const navigate = useNavigate();
    const role = user?.role;

    const getLinkClass = ({ isActive }) =>
        `block rounded-lg px-3 py-2 text-sm font-medium transition-colors ${
            isActive
                ? "bg-indigo-50 text-indigo-700"
                : "text-slate-600 hover:bg-slate-100 hover:text-slate-900"
        }`;

    const handleLogout = () => {
        logout();
        navigate("/login", { replace: true });
    };

    return (
        <aside className="hidden w-64 shrink-0 border-r border-slate-200 bg-white md:flex md:flex-col">
            <div className="border-b border-slate-200 px-5 py-5">
                <h1 className="text-lg font-bold text-indigo-600">
                    SecureWipe
                </h1>

                <p className="mt-1 text-xs text-slate-500">
                    {role?.replaceAll("_", " ")}
                </p>
            </div>

            <nav className="flex-1 space-y-1 overflow-y-auto p-4">

                {role === "ADMIN" && (
                    <>
                        <NavLink
                            to="/admin/dashboard"
                            className={getLinkClass}
                        >
                            Dashboard
                        </NavLink>

                        <NavLink
                            to="/admin/users"
                            className={getLinkClass}
                        >
                            Users
                        </NavLink>

                        <NavLink
                            to="/admin/workstation-centers"
                            className={getLinkClass}
                        >
                            Workstation Centers
                        </NavLink>
                    </>
                )}

                {role === "WORKSTATION_HEAD" && (
                    <>
                        <NavLink
                            to="/workstation-head/dashboard"
                            className={getLinkClass}
                        >
                            Dashboard
                        </NavLink>

                        <NavLink
                            to="/workstation-head/sanitization-requests"
                            className={getLinkClass}
                        >
                            Sanitization Requests
                        </NavLink>

                        <NavLink
                            to="/workstation-head/workstations"
                            className={getLinkClass}
                        >
                            Workstations
                        </NavLink>
                    </>
                )}

                {role === "WORKSTATION_EMPLOYEE" && (
                    <>
                        <NavLink
                            to="/workstation-employee/dashboard"
                            end
                            className={getLinkClass}
                        >
                            Dashboard
                        </NavLink>

                        <NavLink
                            to="/workstation-employee/sanitization/history"
                            className={getLinkClass}
                        >
                            Sanitization History
                        </NavLink>
                    </>
                )}

                {role === "CUSTOMER" && (
                    <>
                        <NavLink
                            to="/customer/dashboard"
                            className={getLinkClass}
                        >
                            Dashboard
                        </NavLink>

                        <NavLink
                            to="/customer/sanitization-request"
                            className={getLinkClass}
                        >
                            Sanitization Request
                        </NavLink>
                    </>
                )}

                {role && (
                    <>
                        <div className="px-3 pb-1 pt-6 text-[10px] font-semibold uppercase tracking-[0.18em] text-slate-400">
                            Forensics
                        </div>

                        <NavLink
                            to="/forensics"
                            end
                            className={getLinkClass}
                        >
                            <span className="flex items-center gap-2">
                                <IconGrid />
                                Overview
                            </span>
                        </NavLink>

                        <NavLink
                            to="/forensics/cases"
                            className={getLinkClass}
                        >
                            <span className="flex items-center gap-2">
                                <IconFolder />
                                Cases
                            </span>
                        </NavLink>

                        <NavLink
                            to="/forensics/evidence"
                            className={getLinkClass}
                        >
                            <span className="flex items-center gap-2">
                                <IconEvidence />
                                Evidence
                            </span>
                        </NavLink>

                        <NavLink
                            to="/forensics/reports"
                            className={getLinkClass}
                        >
                            <span className="flex items-center gap-2">
                                <IconReport />
                                Reports
                            </span>
                        </NavLink>

                        {role === "CUSTOMER" && (
                            <NavLink
                                to="/customer/forensics/new"
                                className={getLinkClass}
                            >
                                <span className="flex items-center gap-2">
                                    <IconPlus />
                                    New Forensic Case
                                </span>
                            </NavLink>
                        )}
                    </>
                )}
            </nav>

            <div className="border-t border-slate-200 p-4">
                <button
                    type="button"
                    onClick={handleLogout}
                    className="w-full rounded-lg border border-slate-300 px-3 py-2 text-sm font-medium text-slate-700 transition-colors hover:bg-slate-50"
                >
                    Logout
                </button>
            </div>
        </aside>
    );
}

function IconGrid() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <rect x="4" y="4" width="6" height="6" rx="1" />
            <rect x="14" y="4" width="6" height="6" rx="1" />
            <rect x="4" y="14" width="6" height="6" rx="1" />
            <rect x="14" y="14" width="6" height="6" rx="1" />
        </svg>
    );
}

function IconFolder() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <path d="M3 6h7l2 2h9v10H3V6Z" />
        </svg>
    );
}

function IconEvidence() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <path d="M6 4h12v16H6z" />
            <path d="M9 8h6M9 12h6M9 16h3" />
        </svg>
    );
}

function IconReport() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <path d="M6 3h9l4 4v14H6V3Z" />
            <path d="M14 3v5h5M9 13h6M9 17h6" />
        </svg>
    );
}

function IconPlus() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <path d="M12 5v14M5 12h14" />
        </svg>
    );
}

export default Sidebar;