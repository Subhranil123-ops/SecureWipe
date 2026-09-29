import { Outlet } from "react-router-dom";

import Sidebar from "./Sidebar";
import Topbar from "./Topbar";
import ProjectResources from "./ProjectResources";

function DashboardLayout() {
    return (
        <div className="flex min-h-screen bg-slate-50">
            <Sidebar />

            <div className="flex min-w-0 flex-1 flex-col">
                <Topbar />

                <main className="flex-1 p-4 sm:p-6">
                    <ProjectResources />

                    <Outlet />
                </main>
            </div>
        </div>
    );
}

export default DashboardLayout;