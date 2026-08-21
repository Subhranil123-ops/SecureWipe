import {
    NavLink,
    useNavigate,
} from "react-router-dom";

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

            <nav className="flex-1 space-y-1 p-4">
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
                    </>
                )}

                {role === "WORKSTATION_EMPLOYEE" && (
                    <>
                        <NavLink
                            to="/workstation-employee/dashboard"
                            className={getLinkClass}
                        >
                            Dashboard
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

export default Sidebar;