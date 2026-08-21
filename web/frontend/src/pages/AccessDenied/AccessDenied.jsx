import { useNavigate } from "react-router-dom";
import { useAuth } from "../../context/AuthContext";
import Button from "../../components/ui/Button";

function AccessDenied() {
    const navigate = useNavigate();
    const { user } = useAuth();

    const handleBack = () => {
        if (user?.role === "ADMIN") {
            navigate("/admin/dashboard");
        } else if (
            user?.role === "WORKSTATION_HEAD"
        ) {
            navigate("/workstation-head/dashboard");
        } else if (
            user?.role === "WORKSTATION_EMPLOYEE"
        ) {
            navigate(
                "/workstation-employee/dashboard"
            );
        } else if (user?.role === "CUSTOMER") {
            navigate("/customer/dashboard");
        } else {
            navigate("/login");
        }
    };

    return (
        <div className="flex min-h-screen items-center justify-center bg-slate-50 px-4">
            <div className="w-full max-w-md rounded-lg border border-slate-200 bg-white p-8 text-center shadow-sm">
                <h1 className="text-2xl font-semibold text-slate-900">
                    Access Denied
                </h1>

                <p className="mt-2 text-sm text-slate-500">
                    You are not authorized to access this page.
                </p>

                <div className="mt-6">
                    <Button onClick={handleBack}>
                        Go to Dashboard
                    </Button>
                </div>
            </div>
        </div>
    );
}

export default AccessDenied;