import {
    BrowserRouter,
    Navigate,
    Route,
    Routes,
} from "react-router-dom";

import Login from "./pages/Login/Login";
import Register from "./pages/Register/Register";
import AccessDenied from "./pages/AccessDenied/AccessDenied";

import ProtectedRoute from "./components/auth/ProtectedRoute";
import RoleRoute from "./components/auth/RoleRoute";

import DashboardLayout from "./components/layout/DashboardLayout";

import AdminDashboard from "./pages/Admin/Dashboard/AdminDashboard";
import AdminUsers from "./pages/Admin/Users/AdminUsers";
import AdminWorkstationCenters from "./pages/Admin/WorkstationCenters/AdminWorkstationCenters";

import WorkstationHeadDashboard from "./pages/WorkstationHead/Dashboard/WorkstationHeadDashboard";
import WorkstationCenter from "./pages/WorkstationHead/Center/WorkstationCenter";

import WorkstationEmployeeDashboard from "./pages/WorkstationEmployee/Dashboard/WorkstationEmployeeDashboard";

import CustomerDashboard from "./pages/Customer/Dashboard/CustomerDashboard";

function App() {
    return (
        <BrowserRouter>
            <Routes>

                {/* Public Routes */}
                <Route
                    path="/login"
                    element={<Login />}
                />

                <Route
                    path="/register"
                    element={<Register />}
                />

                <Route
                    path="/access-denied"
                    element={<AccessDenied />}
                />

                {/* Protected Routes */}
                <Route element={<ProtectedRoute />}>

                    {/* ADMIN */}
                    <Route
                        element={
                            <RoleRoute
                                allowedRoles={["ADMIN"]}
                            />
                        }
                    >
                        <Route
                            path="/admin"
                            element={<DashboardLayout />}
                        >
                            <Route
                                path="dashboard"
                                element={
                                    <AdminDashboard />
                                }
                            />

                            <Route
                                path="users"
                                element={
                                    <AdminUsers />
                                }
                            />

                            <Route
                                path="workstation-centers"
                                element={
                                    <AdminWorkstationCenters />
                                }
                            />
                        </Route>
                    </Route>

                    {/* WORKSTATION HEAD */}
                    <Route
                        element={
                            <RoleRoute
                                allowedRoles={[
                                    "WORKSTATION_HEAD",
                                ]}
                            />
                        }
                    >
                        <Route
                            path="/workstation-head"
                            element={<DashboardLayout />}
                        >
                            <Route
                                path="dashboard"
                                element={
                                    <WorkstationHeadDashboard />
                                }
                            />

                            <Route
                                path="center/:centerId"
                                element={
                                    <WorkstationCenter />
                                }
                            />
                        </Route>
                    </Route>

                    {/* WORKSTATION EMPLOYEE */}
                    <Route
                        element={
                            <RoleRoute
                                allowedRoles={[
                                    "WORKSTATION_EMPLOYEE",
                                ]}
                            />
                        }
                    >
                        <Route
                            path="/workstation-employee"
                            element={<DashboardLayout />}
                        >
                            <Route
                                path="dashboard"
                                element={
                                    <WorkstationEmployeeDashboard />
                                }
                            />
                        </Route>
                    </Route>

                    {/* CUSTOMER */}
                    <Route
                        element={
                            <RoleRoute
                                allowedRoles={[
                                    "CUSTOMER",
                                ]}
                            />
                        }
                    >
                        <Route
                            path="/customer"
                            element={<DashboardLayout />}
                        >
                            <Route
                                path="dashboard"
                                element={
                                    <CustomerDashboard />
                                }
                            />
                        </Route>
                    </Route>

                </Route>

                {/* Default */}
                <Route
                    path="/"
                    element={
                        <Navigate
                            to="/login"
                            replace
                        />
                    }
                />

                {/* Unknown route */}
                <Route
                    path="*"
                    element={
                        <Navigate
                            to="/login"
                            replace
                        />
                    }
                />

            </Routes>
        </BrowserRouter>
    );
}

export default App;