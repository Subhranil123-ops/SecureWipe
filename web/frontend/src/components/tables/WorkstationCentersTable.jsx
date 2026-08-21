function WorkstationCentersTable({
    centers = [],
}) {
    if (!centers.length) {
        return null;
    }

    return (
        <div className="overflow-x-auto rounded-lg border border-slate-200 bg-white shadow-sm">
            <table className="min-w-full text-left text-sm">
                <thead className="border-b border-slate-200 bg-slate-50">
                    <tr>
                        <th className="px-4 py-3 font-medium text-slate-600">
                            Name
                        </th>

                        <th className="px-4 py-3 font-medium text-slate-600">
                            Location
                        </th>

                        <th className="px-4 py-3 font-medium text-slate-600">
                            Head
                        </th>
                    </tr>
                </thead>

                <tbody>
                    {centers.map((center) => (
                        <tr
                            key={center._id}
                            className="border-b border-slate-100 last:border-0"
                        >
                            <td className="px-4 py-3 text-slate-800">
                                {center.name}
                            </td>

                            <td className="px-4 py-3 text-slate-600">
                                {center.location || "-"}
                            </td>

                            <td className="px-4 py-3 text-slate-600">
                                {typeof center.head ===
                                "object"
                                    ? center.head?.name ||
                                      center.head?.email ||
                                      center.head?._id
                                    : center.head || "-"}
                            </td>
                        </tr>
                    ))}
                </tbody>
            </table>
        </div>
    );
}

export default WorkstationCentersTable;