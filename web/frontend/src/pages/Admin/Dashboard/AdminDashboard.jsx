import { useEffect, useState } from "react";

import {
    getAllUsers,
} from "../../../services/userService";

import Loading from "../../../components/common/Loading";
import ErrorMessage from "../../../components/common/ErrorMessage";
import StatCard from "../../../components/cards/StatCard";
import PageHeader from "../../../components/ui/PageHeader";

function AdminDashboard() {
    const [users, setUsers] = useState([]);
    const [loading, setLoading] =
        useState(true);
    const [error, setError] =
        useState("");

    useEffect(() => {
        const loadUsers = async () => {
            try {
                const response =
                    await getAllUsers();

                const userList =
                    Array.isArray(response)
                        ? response
                        : response.users ||
                          response.data ||
                          [];

                setUsers(userList);
            } catch (error) {
                setError(
                    error.message ||
                    "Unable to load dashboard data."
                );
            } finally {
                setLoading(false);
            }
        };

        loadUsers();
    }, []);

    if (loading) {
        return (
            <Loading message="Loading dashboard..." />
        );
    }

    if (error) {
        return (
            <ErrorMessage message={error} />
        );
    }

    const totalUsers = users.length;

    const activeUsers = users.filter(
        (user) =>
            user.status === "ACTIVE"
    ).length;

    const workstationHeads =
        users.filter(
            (user) =>
                user.role ===
                "WORKSTATION_HEAD"
        ).length;

    const workstationEmployees =
        users.filter(
            (user) =>
                user.role ===
                "WORKSTATION_EMPLOYEE"
        ).length;

    const customers =
        users.filter(
            (user) =>
                user.role === "CUSTOMER"
        ).length;

    return (
        <div className="space-y-6">
            <PageHeader
                title="Admin Dashboard"
                description="Overview based on current backend user data."
            />

            <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
                <StatCard
                    title="Total Users"
                    value={totalUsers}
                />

                <StatCard
                    title="Active Users"
                    value={activeUsers}
                />

                <StatCard
                    title="Workstation Heads"
                    value={workstationHeads}
                />

                <StatCard
                    title="Customers"
                    value={customers}
                />
            </div>

            <div className="rounded-lg border border-slate-200 bg-white p-5 shadow-sm">
                <h2 className="text-base font-semibold text-slate-900">
                    User Overview
                </h2>

                <div className="mt-4 space-y-2 text-sm text-slate-600">
                    <p>
                        Workstation Employees:{" "}
                        <span className="font-medium text-slate-900">
                            {workstationEmployees}
                        </span>
                    </p>

                    <p>
                        Data shown above is calculated from the
                        backend users endpoint.
                    </p>
                </div>
            </div>
        </div>
    );
}

export default AdminDashboard;