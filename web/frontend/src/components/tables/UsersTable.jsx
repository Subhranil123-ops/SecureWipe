import Badge from "../ui/Badge";

function UsersTable({
    users = [],
    updatingUserId,
    onRoleChange,
}) {
    return (
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
                                        onRoleChange(
                                            user._id,
                                            event.target
                                                .value
                                        )
                                    }
                                    className="rounded-lg border border-slate-300 px-3 py-2 text-sm outline-none focus:border-indigo-500"
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
    );
}

export default UsersTable;