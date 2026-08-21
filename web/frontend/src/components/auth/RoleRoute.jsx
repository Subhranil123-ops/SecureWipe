import {
    Navigate,
    Outlet,
} from "react-router-dom";

import { useAuth } from "../../context/AuthContext";

function RoleRoute({ allowedRoles = [] }) {
    const { user } = useAuth();

    if (!user) {
        return (
            <Navigate
                to="/login"
                replace
            />
        );
    }

    if (!allowedRoles.includes(user.role)) {
        return (
            <Navigate
                to="/access-denied"
                replace
            />
        );
    }

    return <Outlet />;
}

export default RoleRoute;