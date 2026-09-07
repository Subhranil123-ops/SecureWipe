import { NavLink, useNavigate } from "react-router-dom";
import { useAuth } from "../../context/AuthContext";

function Sidebar() {
    const { user, logout } = useAuth();
    const navigate = useNavigate();
    const role = user?.role;

    const getLinkClass = ({ isActive }) =>
        `group flex items-center gap-3 rounded-xl px-3 py-2.5 text-sm font-medium transition-all ${
            isActive
                ? "bg-indigo-50 text-indigo-700 shadow-sm"
                : "text-slate-600 hover:bg-slate-100 hover:text-slate-900"
        }`;

    const getIconClass = ({ isActive }) =>
        `flex h-8 w-8 shrink-0 items-center justify-center rounded-lg transition ${
            isActive
                ? "bg-white text-indigo-600 shadow-sm"
                : "bg-transparent text-slate-400 group-hover:text-slate-600"
        }`;

    const handleLogout = () => {
        logout();
        navigate("/login", { replace: true });
    };

    const roleLabel =
        role
            ? role
                  .replaceAll("_", " ")
                  .replace(
                      /\b\w/g,
                      (character) =>
                          character.toUpperCase()
                  )
            : "";

    return (
        <aside className="hidden w-72 shrink-0 border-r border-slate-200 bg-white md:flex md:flex-col">
            <div className="border-b border-slate-200 px-5 py-5">
                <div className="flex items-center gap-3">
                    <div className="flex h-10 w-10 items-center justify-center rounded-xl bg-indigo-600 text-sm font-bold text-white shadow-sm">
                        SW
                    </div>

                    <div className="min-w-0">
                        <h1 className="text-base font-bold tracking-tight text-slate-900">
                            SecureWipe
                        </h1>

                        <p className="mt-0.5 text-xs text-slate-400">
                            Secure media operations
                        </p>
                    </div>
                </div>

                {roleLabel && (
                    <div className="mt-4 rounded-xl border border-slate-200 bg-slate-50 px-3 py-2.5">
                        <p className="text-[10px] font-semibold uppercase tracking-[0.16em] text-slate-400">
                            Current Role
                        </p>

                        <p className="mt-1 text-xs font-semibold text-slate-700">
                            {roleLabel}
                        </p>
                    </div>
                )}
            </div>

            <nav className="flex-1 overflow-y-auto p-4">
                {role === "ADMIN" && (
                    <NavSection title="Administration">
                        <SidebarLink
                            to="/admin/dashboard"
                            icon={<IconHome />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Dashboard
                        </SidebarLink>

                        <SidebarLink
                            to="/admin/users"
                            icon={<IconUsers />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Users
                        </SidebarLink>

                        <SidebarLink
                            to="/admin/workstation-centers"
                            icon={<IconBuilding />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Workstation Centers
                        </SidebarLink>
                    </NavSection>
                )}

                {role === "WORKSTATION_HEAD" && (
                    <NavSection title="Operations">
                        <SidebarLink
                            to="/workstation-head/dashboard"
                            icon={<IconHome />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Dashboard
                        </SidebarLink>

                        <SidebarLink
                            to="/workstation-head/sanitization-requests"
                            icon={<IconShield />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Sanitization Requests
                        </SidebarLink>

                        <SidebarLink
                            to="/workstation-head/workstations"
                            icon={<IconMonitor />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Workstations
                        </SidebarLink>
                    </NavSection>
                )}

                {role === "WORKSTATION_EMPLOYEE" && (
                    <NavSection title="Operations">
                        <SidebarLink
                            to="/workstation-employee/dashboard"
                            end
                            icon={<IconHome />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Dashboard
                        </SidebarLink>

                        <SidebarLink
                            to="/workstation-employee/sanitization/history"
                            icon={<IconShield />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Sanitization History
                        </SidebarLink>
                    </NavSection>
                )}

                {role === "CUSTOMER" && (
                    <NavSection title="Service">
                        <SidebarLink
                            to="/customer/dashboard"
                            icon={<IconHome />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Dashboard
                        </SidebarLink>

                        <SidebarLink
                            to="/customer/sanitization-request"
                            icon={<IconShield />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Sanitization Request
                        </SidebarLink>
                    </NavSection>
                )}

                {role && (
                    <NavSection title="Forensics">
                        <SidebarLink
                            to="/forensics"
                            end
                            icon={<IconGrid />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Overview
                        </SidebarLink>

                        <SidebarLink
                            to="/forensics/cases"
                            icon={<IconFolder />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Cases
                        </SidebarLink>

                        <SidebarLink
                            to="/forensics/evidence"
                            icon={<IconEvidence />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Evidence
                        </SidebarLink>

                        <SidebarLink
                            to="/forensics/reports"
                            icon={<IconReport />}
                            getClass={getLinkClass}
                            getIconClass={getIconClass}
                        >
                            Reports
                        </SidebarLink>

                        {role === "CUSTOMER" && (
                            <SidebarLink
                                to="/customer/forensics/new"
                                icon={<IconPlus />}
                                getClass={getLinkClass}
                                getIconClass={getIconClass}
                            >
                                New Forensic Case
                            </SidebarLink>
                        )}
                    </NavSection>
                )}
            </nav>

            <div className="border-t border-slate-200 p-4">
                {user?.name && (
                    <div className="mb-3 flex items-center gap-3 rounded-xl bg-slate-50 px-3 py-3">
                        <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-full bg-indigo-100 text-xs font-semibold text-indigo-700">
                            {getInitials(user.name)}
                        </div>

                        <div className="min-w-0">
                            <p className="truncate text-sm font-semibold text-slate-800">
                                {user.name}
                            </p>

                            <p className="truncate text-xs text-slate-400">
                                {user.email || roleLabel}
                            </p>
                        </div>
                    </div>
                )}

                <button
                    type="button"
                    onClick={handleLogout}
                    className="flex w-full items-center justify-center gap-2 rounded-xl border border-slate-300 bg-white px-3 py-2.5 text-sm font-medium text-slate-700 transition hover:border-red-200 hover:bg-red-50 hover:text-red-700"
                >
                    <IconLogout />
                    Logout
                </button>
            </div>
        </aside>
    );
}

function NavSection({ title, children }) {
    return (
        <div className="mb-6">
            <p className="mb-2 px-3 text-[10px] font-semibold uppercase tracking-[0.18em] text-slate-400">
                {title}
            </p>

            <div className="space-y-1">
                {children}
            </div>
        </div>
    );
}

function SidebarLink({
    to,
    end = false,
    icon,
    children,
    getClass,
    getIconClass,
}) {
    return (
        <NavLink
            to={to}
            end={end}
            className={getClass}
        >
            {({ isActive }) => (
                <>
                    <span className={getIconClass({ isActive })}>
                        {icon}
                    </span>

                    <span className="truncate">
                        {children}
                    </span>
                </>
            )}
        </NavLink>
    );
}

function getInitials(name) {
    const parts = String(name)
        .trim()
        .split(/\s+/)
        .filter(Boolean);

    if (parts.length === 0) {
        return "U";
    }

    if (parts.length === 1) {
        return parts[0]
            .slice(0, 2)
            .toUpperCase();
    }

    return `${parts[0][0]}${parts[parts.length - 1][0]}`.toUpperCase();
}

function IconHome() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <path d="m3 10 9-7 9 7" />
            <path d="M5 9v11h14V9" />
            <path d="M9 20v-6h6v6" />
        </svg>
    );
}

function IconUsers() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <circle cx="9" cy="8" r="3" />
            <path d="M3.5 20a5.5 5.5 0 0 1 11 0" />
            <path d="M16 11a3 3 0 1 0 0-6" />
            <path d="M16 14a5.5 5.5 0 0 1 4.5 6" />
        </svg>
    );
}

function IconBuilding() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <path d="M4 21V5l8-2 8 2v16" />
            <path d="M8 8h2M14 8h2M8 12h2M14 12h2M8 16h2M14 16h2" />
        </svg>
    );
}

function IconMonitor() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <rect x="3" y="4" width="18" height="13" rx="2" />
            <path d="M8 21h8M12 17v4" />
        </svg>
    );
}

function IconShield() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <path d="M12 3 19 6v5c0 5-3 8-7 10-4-2-7-5-7-10V6l7-3Z" />
            <path d="m9 12 2 2 4-4" />
        </svg>
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

function IconLogout() {
    return (
        <svg
            viewBox="0 0 24 24"
            className="h-4 w-4"
            fill="none"
            stroke="currentColor"
            strokeWidth="1.8"
        >
            <path d="M10 5H5v14h5" />
            <path d="M14 8l4 4-4 4" />
            <path d="M18 12H9" />
        </svg>
    );
}

export default Sidebar;