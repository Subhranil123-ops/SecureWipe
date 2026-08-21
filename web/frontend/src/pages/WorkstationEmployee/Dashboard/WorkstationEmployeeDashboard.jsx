import { useAuth } from "../../../context/AuthContext";

function WorkstationEmployeeDashboard() {
    const { user } = useAuth();

    return (
        <div className="space-y-6">
            <div>
                <h1 className="text-2xl font-semibold text-slate-900">
                    Workstation Employee Dashboard
                </h1>

                <p className="mt-1 text-sm text-slate-500">
                    Welcome, {user?.name}.
                </p>
            </div>

            <div className="rounded-lg border border-slate-200 bg-white p-6 shadow-sm">
                <p className="text-sm text-slate-500">
                    No employee-specific backend functionality is
                    available yet.
                </p>
            </div>
        </div>
    );
}

export default WorkstationEmployeeDashboard;