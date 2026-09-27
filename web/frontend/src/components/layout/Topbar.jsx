import {
    useLocation
} from "react-router-dom";

import {
    useAuth
} from "../../context/AuthContext";


function Topbar() {

    const {
        user
    } =
        useAuth();


    const location =
        useLocation();


    const getPageTitle =
        () => {

            const path =
                location.pathname;


            if (
                path.includes(
                    "/forensics/cases/"
                )
            ) {
                return "Forensic Case";
            }


            if (
                path ===
                "/forensics/cases"
            ) {
                return "Forensic Cases";
            }


            if (
                path ===
                "/forensics/evidence"
            ) {
                return "Evidence";
            }


            if (
                path ===
                "/forensics/reports"
            ) {
                return "Forensic Reports";
            }


            if (
                path ===
                "/forensics"
            ) {
                return "Forensics";
            }


            if (
                path.includes(
                    "/forensics/new"
                )
            ) {
                return "New Forensic Case";
            }


            if (
                path ===
                "/certificates"
            ) {
                return "Certificates & Evidence";
            }


            if (
                path.startsWith(
                    "/certificates/sanitization/"
                )
            ) {
                return "Sanitization Certificate";
            }


            if (
                path.startsWith(
                    "/certificates/forensic/"
                )
            ) {
                return "Forensic Certificate";
            }


            if (
                path.includes(
                    "/users"
                )
            ) {
                return "Users";
            }


            if (
                path.includes(
                    "/workstation-centers"
                )
            ) {
                return "Workstation Centers";
            }


            if (
                path.includes(
                    "/workstation-head/center"
                )
            ) {
                return "My Center";
            }


            if (
                path.includes(
                    "/workstation-head/dashboard"
                )
            ) {
                return "Dashboard";
            }


            if (
                path.includes(
                    "/workstation-employee"
                )
            ) {
                return "Dashboard";
            }


            if (
                path.includes(
                    "/customer"
                )
            ) {
                return "Dashboard";
            }


            if (
                path.includes(
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
                    {
                        getPageTitle()
                    }
                </h2>

            </div>


            <div className="text-right">

                <p className="text-sm font-medium text-slate-800">
                    {
                        user?.name
                    }
                </p>


                <p className="text-xs text-slate-500">
                    {
                        user?.role?.replaceAll(
                            "_",
                            " "
                        )
                    }
                </p>

            </div>

        </header>
    );
}


export default Topbar;