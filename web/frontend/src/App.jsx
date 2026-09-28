import {
    BrowserRouter,
    Navigate,
    Route,
    Routes,
    useLocation,
    Link,
} from "react-router-dom";

import {
    Download as DownloadIcon
} from "lucide-react";


import Login from "./pages/Login/Login";
import Register from "./pages/Register/Register";
import AccessDenied from "./pages/AccessDenied/AccessDenied";
import Download from "./pages/Download/Download";


import ProtectedRoute
    from "./components/auth/ProtectedRoute";

import RoleRoute
    from "./components/auth/RoleRoute";

import DashboardLayout
    from "./components/layout/DashboardLayout";


import AdminDashboard
    from "./pages/Admin/Dashboard/AdminDashboard";

import AdminUsers
    from "./pages/Admin/Users/AdminUsers";

import AdminWorkstationCenters
    from "./pages/Admin/WorkstationCenters/AdminWorkstationCenters";

import AdminWorkstations
    from "./pages/Admin/Workstations/AdminWorkstations";


import WorkstationHeadDashboard
    from "./pages/WorkstationHead/Dashboard/WorkstationHeadDashboard";

import WorkstationCenter
    from "./pages/WorkstationHead/Center/WorkstationCenter";

import WorkstationHeadSanitizationRequests
    from "./pages/WorkstationHead/SanitizationRequests/WorkstationHeadSanitizationRequests";

import WorkstationHeadWorkstations
    from "./pages/WorkstationHead/Workstations/WorkstationHeadWorkstations";


import WorkstationEmployeeDashboard
    from "./pages/WorkstationEmployee/Dashboard/WorkstationEmployeeDashboard";

import SanitizationExecution
    from "./pages/WorkstationEmployee/Sanitization/SanitizationExecution";

import SanitizationHistory
    from "./pages/WorkstationEmployee/Sanitization/SanitizationHistory";

import SanitizationCertificate
    from "./pages/WorkstationEmployee/Sanitization/SanitizationCertificate";


import CustomerDashboard
    from "./pages/Customer/Dashboard/CustomerDashboard";

import CustomerSanitizationRequest
    from "./pages/Customer/SanitizationRequest/CustomerSanitizationRequest";

import CustomerSanitizationRequests
    from "./pages/Customer/SanitizationRequest/CustomerSanitizationRequests";

import CustomerSanitizationDetails
    from "./pages/Customer/SanitizationRequest/CustomerSanitizationDetails";


import ForensicDashboard
    from "./pages/Forensics/ForensicDashboard";

import ForensicCases
    from "./pages/Forensics/ForensicCases";

import ForensicCaseDetails
    from "./pages/Forensics/ForensicCaseDetails";

import ForensicEvidence
    from "./pages/Forensics/ForensicEvidence";

import ForensicNewCase
    from "./pages/Forensics/ForensicNewCase";

import ForensicReports
    from "./pages/Forensics/ForensicReports";


/*
 * ============================================================
 * CENTRAL CERTIFICATE UI
 * ============================================================
 */

import Certificates
    from "./pages/Certificates/Certificates";

import CertificateDetails
    from "./pages/Certificates/CertificateDetails";


function FloatingDownloadButton() {

    const location =
        useLocation();


    if (
        location.pathname ===
            "/downloads" ||

        location.pathname ===
            "/login" ||

        location.pathname ===
            "/register"
    ) {
        return null;
    }


    return (
        <Link
            to="/downloads"
            className="
                fixed
                bottom-6
                right-6
                z-[100]
                inline-flex
                items-center
                gap-2
                rounded-xl
                bg-blue-600
                px-4
                py-3
                text-sm
                font-semibold
                text-white
                shadow-lg
                shadow-blue-600/20
                transition
                hover:bg-blue-700
                hover:-translate-y-0.5
                focus:outline-none
                focus:ring-2
                focus:ring-blue-500
                focus:ring-offset-2
            "
        >

            <DownloadIcon
                className="h-4 w-4"
            />

            Download App

        </Link>
    );
}


function AppContent() {

    return (
        <>

            <Routes>

                {/* PUBLIC */}

                <Route
                    path="/login"
                    element={
                        <Login />
                    }
                />

                <Route
                    path="/register"
                    element={
                        <Register />
                    }
                />

                <Route
                    path="/access-denied"
                    element={
                        <AccessDenied />
                    }
                />

                <Route
                    path="/downloads"
                    element={
                        <Download />
                    }
                />


                {/* PROTECTED */}

                <Route
                    element={
                        <ProtectedRoute />
                    }
                >

                    {/* ==================================================
                        ADMIN
                       ================================================== */}

                    <Route
                        element={
                            <RoleRoute
                                allowedRoles={[
                                    "ADMIN"
                                ]}
                            />
                        }
                    >

                        <Route
                            path="/admin"
                            element={
                                <DashboardLayout />
                            }
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

                            <Route
                                path="workstations"
                                element={
                                    <AdminWorkstations />
                                }
                            />

                        </Route>

                    </Route>


                    {/* ==================================================
                        WORKSTATION HEAD
                       ================================================== */}

                    <Route
                        element={
                            <RoleRoute
                                allowedRoles={[
                                    "WORKSTATION_HEAD"
                                ]}
                            />
                        }
                    >

                        <Route
                            path="/workstation-head"
                            element={
                                <DashboardLayout />
                            }
                        >

                            <Route
                                path="dashboard"
                                element={
                                    <WorkstationHeadDashboard />
                                }
                            />

                            <Route
                                path="sanitization-requests"
                                element={
                                    <WorkstationHeadSanitizationRequests />
                                }
                            />

                            <Route
                                path="workstations"
                                element={
                                    <WorkstationHeadWorkstations />
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


                    {/* ==================================================
                        WORKSTATION EMPLOYEE
                       ================================================== */}

                    <Route
                        element={
                            <RoleRoute
                                allowedRoles={[
                                    "WORKSTATION_EMPLOYEE"
                                ]}
                            />
                        }
                    >

                        <Route
                            path="/workstation-employee"
                            element={
                                <DashboardLayout />
                            }
                        >

                            <Route
                                path="dashboard"
                                element={
                                    <WorkstationEmployeeDashboard />
                                }
                            />

                            <Route
                                path="sanitization/history"
                                element={
                                    <SanitizationHistory />
                                }
                            />

                            <Route
                                path="sanitization/:requestId"
                                element={
                                    <SanitizationExecution />
                                }
                            />

                            <Route
                                path="sanitization/certificate/:certificateId"
                                element={
                                    <SanitizationCertificate />
                                }
                            />

                        </Route>

                    </Route>


                    {/* ==================================================
                        CUSTOMER
                       ================================================== */}

                    <Route
                        element={
                            <RoleRoute
                                allowedRoles={[
                                    "CUSTOMER"
                                ]}
                            />
                        }
                    >

                        <Route
                            path="/customer"
                            element={
                                <DashboardLayout />
                            }
                        >

                            {/* Customer Dashboard */}

                            <Route
                                path="dashboard"
                                element={
                                    <CustomerDashboard />
                                }
                            />


                            {/* Existing request creation page */}

                            <Route
                                path="sanitization-request"
                                element={
                                    <CustomerSanitizationRequest />
                                }
                            />


                            {/* Customer's own sanitization requests */}

                            <Route
                                path="sanitization-requests"
                                element={
                                    <CustomerSanitizationRequests />
                                }
                            />


                            {/* Individual customer sanitization job */}

                            <Route
                                path="sanitization-requests/:requestId"
                                element={
                                    <CustomerSanitizationDetails />
                                }
                            />


                            {/* New forensic case */}

                            <Route
                                path="forensics/new"
                                element={
                                    <ForensicNewCase />
                                }
                            />

                        </Route>

                    </Route>


                    {/* ==================================================
                        CERTIFICATES & EVIDENCE
                       ================================================== */}

                    <Route
                        element={
                            <RoleRoute
                                allowedRoles={[
                                    "ADMIN",
                                    "CUSTOMER",
                                    "WORKSTATION_HEAD",
                                    "WORKSTATION_EMPLOYEE"
                                ]}
                            />
                        }
                    >

                        <Route
                            path="/certificates"
                            element={
                                <DashboardLayout />
                            }
                        >

                            {/* Certificate Registry */}

                            <Route
                                index
                                element={
                                    <Certificates />
                                }
                            />


                            {/* Sanitization Certificate */}

                            <Route
                                path="sanitization/:id"
                                element={
                                    <CertificateDetails />
                                }
                            />


                            {/* Forensic Certificate */}

                            <Route
                                path="forensic/:id"
                                element={
                                    <CertificateDetails />
                                }
                            />

                        </Route>

                    </Route>


                    {/* ==================================================
                        FORENSICS
                       ================================================== */}

                    <Route
                        element={
                            <RoleRoute
                                allowedRoles={[
                                    "ADMIN",
                                    "CUSTOMER",
                                    "WORKSTATION_HEAD",
                                    "WORKSTATION_EMPLOYEE"
                                ]}
                            />
                        }
                    >

                        <Route
                            path="/forensics"
                            element={
                                <DashboardLayout />
                            }
                        >

                            {/* Forensics Overview */}

                            <Route
                                index
                                element={
                                    <ForensicDashboard />
                                }
                            />


                            {/* Cases */}

                            <Route
                                path="cases"
                                element={
                                    <ForensicCases />
                                }
                            />


                            {/* Individual Case */}

                            <Route
                                path="cases/:caseId"
                                element={
                                    <ForensicCaseDetails />
                                }
                            />


                            {/* Evidence */}

                            <Route
                                path="evidence"
                                element={
                                    <ForensicEvidence />
                                }
                            />


                            {/* Reports */}

                            <Route
                                path="reports"
                                element={
                                    <ForensicReports />
                                }
                            />

                        </Route>

                    </Route>

                </Route>


                {/* ==================================================
                    DEFAULT ROUTES
                   ================================================== */}

                <Route
                    path="/"
                    element={
                        <Navigate
                            to="/login"
                            replace
                        />
                    }
                />


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


            <FloatingDownloadButton />

        </>
    );
}


function App() {

    return (
        <BrowserRouter>

            <AppContent />

        </BrowserRouter>
    );
}


export default App;