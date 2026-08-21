import { useEffect, useState } from "react";

import toast from "react-hot-toast";

import {
    getAllUsers,
    updateUserRole,
} from "../../../services/userService";

import Loading from "../../../components/common/Loading";
import ErrorMessage from "../../../components/common/ErrorMessage";
import EmptyState from "../../../components/common/EmptyState";
import Badge from "../../../components/ui/Badge";
import PageHeader from "../../../components/ui/PageHeader";

function AdminUsers() {
    const [users, setUsers] = useState([]);
    const [loading, setLoading] = useState(true);
    const [error, setError] = useState("");

    const [updatingUserId, setUpdatingUserId] =
        useState(null);

    const loadUsers = async () => {
        setLoading(true);
        setError("");

        try {
            const response = await getAllUsers();

            setUsers(
                Array.isArray(response)
                    ? response
                    : response.users || response.data || []
            );
        } catch (error) {
            setError(
                error.message ||
                "Unable to load users."
            );
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        loadUsers();
    }, []);

    const handleRoleChange = async (
        userId,
        role
    ) => {
        setUpdatingUserId(userId);

        try {
            await updateUserRole(
                userId,
                role
            );

            toast.success(
                "User role updated successfully"
            );

            await loadUsers();
        } catch (error) {
            toast.error(
                error.message ||
                "Unable to update user role"
            );
        } finally {
            setUpdatingUserId(null);
        }
    };

    if (loading) {
        return (
            <Loading message="Loading users..." />
        );
    }

    return (
        <div className="space-y-6">
            <PageHeader
                title="Users"
                description="View and manage registered users."
            />

            {error && (
                <ErrorMessage message={error} />
            )}

            {!error && users.length === 0 && (
                <EmptyState
                    title="No users found"
                    message="There are no users available."
                />
            )}

            {users.length > 0 && (
                <div className="overflow-x-auto rounded-lg border border-slate-200 bg-white shadow-sm">
                    <table className="min-w-full text-left text-sm">
                        <thead className="border-b border-slate-200 bg-slate-50">
                            <tr>
                                <th className="px-4 py-3 font-medium text-slate-600">
                                    Name
                                </th>

                                <th className="px-4 py-3 font-medium text-slate-600">
                                    Email
                                </th>

                                <th className="px-4 py-3 font-medium text-slate-600">
                                    Role
                                </th>

                                <th className="px-4 py-3 font-medium text-slate-600">
                                    Status
                                </th>

                                <th className="px-4 py-3 font-medium text-slate-600">
                                    Action
                                </th>
                            </tr>
                        </thead>

                        <tbody>
                            {users.map((user) => (
                                <tr
                                    key={user._id}
                                    className="border-b border-slate-100 last:border-0"
                                >
                                    <td className="px-4 py-3 text-slate-800">
                                        {user.name}
                                    </td>

                                    <td className="px-4 py-3 text-slate-600">
                                        {user.email}
                                    </td>

                                    <td className="px-4 py-3">
                                        <Badge variant="primary">
                                            {user.role}
                                        </Badge>
                                    </td>

                                    <td className="px-4 py-3">
                                        <Badge
                                            variant={
                                                user.status ===
                                                "ACTIVE"
                                                    ? "success"
                                                    : "default"
                                            }
                                        >
                                            {user.status}
                                        </Badge>
                                    </td>

                                    <td className="px-4 py-3">
                                        <select
                                            value={
                                                user.role || ""
                                            }
                                            disabled={
                                                updatingUserId ===
                                                user._id
                                            }
                                            onChange={(event) =>
                                                handleRoleChange(
                                                    user._id,
                                                    event.target
                                                        .value
                                                )
                                            }
                                            className="rounded-lg border border-slate-300 bg-white px-3 py-2 text-sm outline-none focus:border-indigo-500"
                                        >
                                            <option value="ADMIN">
                                                ADMIN
                                            </option>

                                            <option value="WORKSTATION_HEAD">
                                                WORKSTATION_HEAD
                                            </option>

                                            <option value="WORKSTATION_EMPLOYEE">
                                                WORKSTATION_EMPLOYEE
                                            </option>

                                            <option value="CUSTOMER">
                                                CUSTOMER
                                            </option>
                                        </select>
                                    </td>
                                </tr>
                            ))}
                        </tbody>
                    </table>
                </div>
            )}
        </div>
    );
}

export default AdminUsers;