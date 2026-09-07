import { useEffect, useState } from "react";
import toast from "react-hot-toast";
import { useNavigate } from "react-router-dom";
import { useAuth } from "../../../context/AuthContext";

import {
    getHeadSanitizationRequests,
    getHeadApprovedSanitizationRequests,
    updateSanitizationRequestStatus,
    assignSanitizationRequest,
} from "../../../services/sanitizationRequestService";

import {
    getMyWorkstationCenter,
} from "../../../services/workstationCenterService";

function WorkstationHeadDashboard() {

    const { user } = useAuth();

    const navigate = useNavigate();

    const [pendingRequests, setPendingRequests] = useState([]);

    const [approvedRequests, setApprovedRequests] = useState([]);

    const [center, setCenter] = useState(null);

    const [loading, setLoading] = useState(true);

    const [actionLoading, setActionLoading] = useState(null);

    const [assignment, setAssignment] = useState({});


    const loadDashboard =
        async () => {

            try {
                setLoading(true);

                const [
                    pending,
                    approved,
                    centerResponse,
                ] = await Promise.all([
                    getHeadSanitizationRequests(),
                    getHeadApprovedSanitizationRequests(),
                    getMyWorkstationCenter(),
                ]);

                setPendingRequests(
                    pending || []
                );

                setApprovedRequests(
                    approved || []
                );

                setCenter(
                    centerResponse.data ||
                    centerResponse
                );

            } catch (error) {

                console.error(
                    "Failed to load dashboard:",
                    error
                );

                toast.error(
                    error.message ||
                    "Unable to load dashboard"
                );

            } finally {
                setLoading(false);
            }
        };


    useEffect(() => {
        loadDashboard();
    }, []);


    const handleApprove =
        async (requestId) => {

            if (
                !window.confirm(
                    "Are you sure you want to approve this request?"
                )
            ) {
                return;
            }

            try {

                setActionLoading(
                    requestId
                );

                await updateSanitizationRequestStatus(
                    requestId,
                    {
                        status:
                            "APPROVED",
                    }
                );

                toast.success(
                    "Request approved successfully"
                );

                await loadDashboard();

            } catch (error) {

                toast.error(
                    error.message ||
                    "Unable to approve request"
                );

            } finally {

                setActionLoading(null);
            }
        };


    const handleReject =
        async (requestId) => {

            const reason =
                window.prompt(
                    "Enter rejection reason:"
                );

            if (
                !reason ||
                !reason.trim()
            ) {
                return;
            }

            try {

                setActionLoading(
                    requestId
                );

                await updateSanitizationRequestStatus(
                    requestId,
                    {
                        status:
                            "REJECTED",

                        reason:
                            reason.trim(),
                    }
                );

                toast.success(
                    "Request rejected successfully"
                );

                await loadDashboard();

            } catch (error) {

                toast.error(
                    error.message ||
                    "Unable to reject request"
                );

            } finally {

                setActionLoading(null);
            }
        };


    const findEmployeeWorkstation = (employeeId) => {
        if (!employeeId) {
            return null;
        }

        return (center?.workstations || []).find(
            (workstation) =>
                String(
                    workstation.assignedEmployee?._id ||
                    workstation.assignedEmployee ||
                    ""
                ) === String(employeeId)
        ) || null;
    };

    const handleEmployeeSelection = (
        requestId,
        employeeId
    ) => {
        const employeeWorkstation =
            findEmployeeWorkstation(employeeId);

        setAssignment((previous) => ({
            ...previous,
            [requestId]: {
                ...previous[requestId],
                assignedEmployeeId: employeeId,
                assignedWorkstationId:
                    employeeWorkstation?.workstationId || "",
            },
        }));
    };

    const handleAssignmentChange =
        (
            requestId,
            field,
            value
        ) => {

            setAssignment(
                (previous) => ({
                    ...previous,

                    [requestId]: {
                        ...previous[requestId],
                        [field]: value,
                    },
                })
            );
        };


    const handleAssign =
        async (request) => {

            const selected =
                assignment[
                request.requestId
                ] || {};

            if (
                !selected.assignedEmployeeId
            ) {
                toast.error(
                    "Please select an employee"
                );

                return;
            }

            const employeeWorkstation =
                findEmployeeWorkstation(
                    selected.assignedEmployeeId
                );

            if (
                employeeWorkstation &&
                selected.assignedWorkstationId &&
                selected.assignedWorkstationId !==
                    employeeWorkstation.workstationId
            ) {
                toast.error(
                    `This employee is already assigned to ${employeeWorkstation.workstationId}.`
                );

                return;
            }

            if (
                !employeeWorkstation &&
                !selected.assignedWorkstationId
            ) {
                toast.error(
                    "Select an unassigned workstation to bind this employee before assigning the request."
                );

                return;
            }

            try {

                setActionLoading(
                    request.requestId
                );

                await assignSanitizationRequest(
                    request.requestId,
                    {
                        assignedEmployeeId:
                            selected.assignedEmployeeId,

                        assignedWorkstationId:
                            selected.assignedWorkstationId,
                    }
                );

                toast.success(
                    "Request assigned successfully"
                );

                setAssignment(
                    (previous) => {

                        const copy = {
                            ...previous
                        };

                        delete copy[
                            request.requestId
                        ];

                        return copy;
                    }
                );

                await loadDashboard();

            } catch (error) {

                toast.error(
                    error.message ||
                    "Unable to assign request"
                );

            } finally {

                setActionLoading(null);
            }
        };


    if (loading) {

        return (
            <div className="p-6">
                <p className="text-sm text-slate-500">
                    Loading workstation head dashboard...
                </p>
            </div>
        );
    }


    const employees =
        center?.employees || [];

    const workstations =
        center?.workstations || [];


    const availableEmployees =
        employees.filter(
            (employee) =>
                employee.role ===
                "WORKSTATION_EMPLOYEE" &&
                employee.status ===
                "ACTIVE"
        );


    const availableWorkstations =
        workstations.filter(
            (workstation) =>
                workstation.status ===
                "ACTIVE"
        );


    return (
        <div className="space-y-8">

            {/* HEADER */}

            <div>
                <h1 className="text-2xl font-semibold text-slate-900">
                    Workstation Head Dashboard
                </h1>

                <p className="mt-1 text-sm text-slate-500">
                    Welcome, {user?.name}.
                </p>

                <p className="mt-1 text-sm font-medium text-slate-700">
                    Centre ID:{" "}
                    <span className="font-mono text-slate-900">
                        {center?.centerId || "N/A"}
                    </span>
                </p>
            </div>


            {/* SUMMARY */}

            <div className="grid grid-cols-1 gap-4 md:grid-cols-3">

                <div className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">

                    <p className="text-sm text-slate-500">
                        Pending Review
                    </p>

                    <p className="mt-2 text-3xl font-semibold text-slate-900">
                        {pendingRequests.length}
                    </p>

                </div>


                <div className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">

                    <p className="text-sm text-slate-500">
                        Awaiting Assignment
                    </p>

                    <p className="mt-2 text-3xl font-semibold text-slate-900">
                        {approvedRequests.length}
                    </p>

                </div>


                <div
                    onClick={() =>
                        navigate(
                            "/workstation-head/workstations"
                        )
                    }
                    className="cursor-pointer rounded-lg border border-slate-200 bg-white p-5 shadow-sm transition hover:border-indigo-300 hover:shadow-md"
                >

                    <p className="text-sm text-slate-500">
                        Active Workstations
                    </p>

                    <p className="mt-2 text-3xl font-semibold text-slate-900">
                        {availableWorkstations.length}
                    </p>

                    <p className="mt-2 text-xs font-medium text-indigo-600">
                        View all workstations →
                    </p>

                </div>

            </div>


            {/* PENDING REQUESTS */}

            <div className="rounded-lg border border-slate-200 bg-white shadow-sm">

                <div className="border-b border-slate-200 p-5">

                    <h2 className="text-lg font-semibold text-slate-900">
                        Pending Sanitization Requests
                    </h2>

                    <p className="mt-1 text-sm text-slate-500">
                        Review requests submitted to your center.
                    </p>

                </div>


                {pendingRequests.length === 0 ? (

                    <div className="p-8 text-center text-sm text-slate-500">
                        No pending requests.
                    </div>

                ) : (

                    <div className="overflow-x-auto">

                        <table className="min-w-full">

                            <thead className="bg-slate-50">

                                <tr>

                                    <th className="px-4 py-3 text-left text-xs font-semibold uppercase text-slate-500">
                                        Request
                                    </th>

                                    <th className="px-4 py-3 text-left text-xs font-semibold uppercase text-slate-500">
                                        Customer
                                    </th>

                                    <th className="px-4 py-3 text-left text-xs font-semibold uppercase text-slate-500">
                                        Device
                                    </th>

                                    <th className="px-4 py-3 text-left text-xs font-semibold uppercase text-slate-500">
                                        Method
                                    </th>

                                    <th className="px-4 py-3 text-left text-xs font-semibold uppercase text-slate-500">
                                        Actions
                                    </th>

                                </tr>

                            </thead>


                            <tbody className="divide-y divide-slate-200">

                                {pendingRequests.map(
                                    (request) => (

                                        <tr
                                            key={
                                                request._id
                                            }
                                        >

                                            <td className="px-4 py-4 text-sm font-medium text-slate-900">
                                                {
                                                    request.requestId
                                                }
                                            </td>

                                            <td className="px-4 py-4 text-sm text-slate-700">
                                                {request.name}
                                            </td>

                                            <td className="px-4 py-4 text-sm text-slate-700">
                                                {
                                                    request.deviceType
                                                }
                                            </td>

                                            <td className="px-4 py-4 text-sm text-slate-700">
                                                {
                                                    request.sanitizationMethod
                                                }
                                            </td>

                                            <td className="px-4 py-4">

                                                <div className="flex gap-2">

                                                    <button
                                                        type="button"
                                                        onClick={() =>
                                                            handleApprove(
                                                                request.requestId
                                                            )
                                                        }
                                                        disabled={
                                                            actionLoading ===
                                                            request.requestId
                                                        }
                                                        className="rounded-md bg-green-600 px-3 py-2 text-xs font-medium text-white disabled:opacity-50"
                                                    >
                                                        Approve
                                                    </button>


                                                    <button
                                                        type="button"
                                                        onClick={() =>
                                                            handleReject(
                                                                request.requestId
                                                            )
                                                        }
                                                        disabled={
                                                            actionLoading ===
                                                            request.requestId
                                                        }
                                                        className="rounded-md bg-red-600 px-3 py-2 text-xs font-medium text-white disabled:opacity-50"
                                                    >
                                                        Reject
                                                    </button>

                                                </div>

                                            </td>

                                        </tr>

                                    )
                                )}

                            </tbody>

                        </table>

                    </div>

                )}

            </div>


            {/* APPROVED / ASSIGNMENT */}

            <div className="rounded-lg border border-slate-200 bg-white shadow-sm">

                <div className="border-b border-slate-200 p-5">

                    <h2 className="text-lg font-semibold text-slate-900">
                        Approved Requests — Assignment
                    </h2>

                    <p className="mt-1 text-sm text-slate-500">
                        Assign an active workstation and employee.
                    </p>

                </div>


                {approvedRequests.length === 0 ? (

                    <div className="p-8 text-center text-sm text-slate-500">
                        No approved requests waiting for assignment.
                    </div>

                ) : (

                    <div className="space-y-4 p-5">

                        {approvedRequests.map(
                            (request) => {

                                const selected =
                                    assignment[
                                    request.requestId
                                    ] || {};

                                return (

                                    <div
                                        key={
                                            request._id
                                        }
                                        className="rounded-lg border border-slate-200 p-5"
                                    >

                                        <div className="mb-4">

                                            <p className="font-semibold text-slate-900">
                                                {
                                                    request.requestId
                                                }
                                            </p>

                                            <p className="mt-1 text-sm text-slate-500">
                                                {
                                                    request.name
                                                }{" "}
                                                •{" "}
                                                {
                                                    request.deviceType
                                                }{" "}
                                                •{" "}
                                                {
                                                    request.sanitizationMethod
                                                }
                                            </p>

                                        </div>


                                        <div className="grid grid-cols-1 gap-4 md:grid-cols-2">

                                            {/* EMPLOYEE */}

                                            <div>

                                                <label className="mb-1 block text-sm font-medium text-slate-700">
                                                    Assign Employee
                                                </label>

                                                <select
                                                    value={
                                                        selected.assignedEmployeeId ||
                                                        ""
                                                    }
                                                    onChange={(event) =>
                                                        handleEmployeeSelection(
                                                            request.requestId,
                                                            event.target.value
                                                        )
                                                    }
                                                    className="w-full rounded-md border border-slate-300 px-3 py-2 text-sm"
                                                >

                                                    <option value="">
                                                        Select employee
                                                    </option>

                                                    {availableEmployees.map(
                                                        (
                                                            employee
                                                        ) => (

                                                            <option
                                                                key={
                                                                    employee._id
                                                                }
                                                                value={
                                                                    employee._id
                                                                }
                                                            >
                                                                {
                                                                    employee.name
                                                                }{" "}
                                                                (
                                                                {
                                                                    employee.email
                                                                }
                                                                )
                                                            </option>

                                                        )
                                                    )}

                                                </select>

                                            </div>


                                            {/* WORKSTATION */}

                                            <div>

                                                <label className="mb-1 block text-sm font-medium text-slate-700">
                                                    Assigned Workstation
                                                </label>

                                                {selected.assignedEmployeeId && findEmployeeWorkstation(selected.assignedEmployeeId) ? (

                                                    <div className="rounded-md border border-emerald-200 bg-emerald-50 px-3 py-3">

                                                        <p className="font-medium text-emerald-900">
                                                            {findEmployeeWorkstation(selected.assignedEmployeeId).workstationId}
                                                            {" — "}
                                                            {findEmployeeWorkstation(selected.assignedEmployeeId).name}
                                                        </p>

                                                        <p className="mt-1 text-xs text-emerald-700">
                                                            {findEmployeeWorkstation(selected.assignedEmployeeId).connectionStatus || "OFFLINE"}
                                                            {" • This employee is already bound to this workstation"}
                                                        </p>

                                                    </div>

                                                ) : (

                                                    <>
                                                        <select
                                                            value={
                                                                selected.assignedWorkstationId ||
                                                                ""
                                                            }
                                                            onChange={(event) =>
                                                                handleAssignmentChange(
                                                                    request.requestId,
                                                                    "assignedWorkstationId",
                                                                    event.target.value
                                                                )
                                                            }
                                                            disabled={!selected.assignedEmployeeId}
                                                            className="w-full rounded-md border border-slate-300 px-3 py-2 text-sm disabled:cursor-not-allowed disabled:bg-slate-50"
                                                        >
                                                            <option value="">
                                                                {selected.assignedEmployeeId
                                                                    ? "Select workstation to bind employee"
                                                                    : "Select employee first"}
                                                            </option>

                                                            {availableWorkstations
                                                                .filter(
                                                                    (workstation) =>
                                                                        !workstation.assignedEmployee
                                                                )
                                                                .map(
                                                                    (workstation) => (
                                                                        <option
                                                                            key={
                                                                                workstation._id
                                                                            }
                                                                            value={
                                                                                workstation.workstationId
                                                                            }
                                                                        >
                                                                            {workstation.workstationId}
                                                                            {" — "}
                                                                            {workstation.name}
                                                                            {" • "}
                                                                            {workstation.connectionStatus || "OFFLINE"}
                                                                        </option>
                                                                    )
                                                                )}
                                                        </select>

                                                        {selected.assignedEmployeeId && (
                                                            <p className="mt-2 text-xs text-amber-700">
                                                                This employee has no workstation yet. The selected workstation will become the employee's workstation for future jobs.
                                                            </p>
                                                        )}
                                                    </>

                                                )}

                                            </div>

                                        </div>

                                        <button
                                            type="button"
                                            onClick={() =>
                                                handleAssign(
                                                    request
                                                )
                                            }
                                            disabled={
                                                actionLoading ===
                                                request.requestId
                                            }
                                            className="mt-5 rounded-md bg-indigo-600 px-5 py-2 text-sm font-medium text-white hover:bg-indigo-700 disabled:opacity-50"
                                        >
                                            {actionLoading ===
                                                request.requestId
                                                ? "Assigning..."
                                                : "Assign Request"}
                                        </button>

                                    </div>

                                );
                            }
                        )}

                    </div>

                )}

            </div>

        </div>
    );
}

export default WorkstationHeadDashboard;