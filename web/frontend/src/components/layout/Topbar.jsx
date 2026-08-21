import { useLocation } from "react-router-dom";

import { useAuth } from "../../context/AuthContext";

function Topbar() {
    const { user } = useAuth();
    const location = useLocation();

    const getPageTitle = () => {
        if (location.pathname.includes("/users")) {
            return "Users";
        }

        if (
            location.pathname.includes(
                "/workstation-centers"
            )
        ) {
            return "Workstation Centers";
        }

        if (
            location.pathname.includes(
                "/workstation-head/center"
            )
        ) {
            return "My Center";
        }

        if (
            location.pathname.includes(
                "/workstation-head/dashboard"
            )
        ) {
            return "Dashboard";
        }

        if (
            location.pathname.includes(
                "/workstation-employee"
            )
        ) {
            return "Dashboard";
        }

        if (
            location.pathname.includes(
                "/customer"
            )
        ) {
            return "Dashboard";
        }

        if (
            location.pathname.includes(
                "/admin/dashboard"
            )
        ) {
            return "Dashboard";
        }

        return "Dashboard";
    };

    return (
        <header className="flex min-h-16 items-center justify-between border-b border-slate-200 bg-white px-4 sm:px-6">
            <div>
                <h2 className="text-lg font-semibold text-slate-900">
                    {getPageTitle()}
                </h2>
            </div>

            <div className="text-right">
                <p className="text-sm font-medium text-slate-800">
                    {user?.name}
                </p>

                <p className="text-xs text-slate-500">
                    {user?.role?.replaceAll("_", " ")}
                </p>
            </div>
        </header>
    );
}

export default Topbar;