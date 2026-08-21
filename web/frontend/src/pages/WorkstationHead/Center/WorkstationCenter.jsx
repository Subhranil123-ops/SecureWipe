import { useEffect, useState } from "react";
import { useParams } from "react-router-dom";

import {
    getWorkstationCenter,
} from "../../../services/workstationCenterService";

import Loading from "../../../components/common/Loading";
import ErrorMessage from "../../../components/common/ErrorMessage";
import EmptyState from "../../../components/common/EmptyState";
import PageHeader from "../../../components/ui/PageHeader";

function WorkstationCenter() {
    const { centerId } = useParams();

    const [center, setCenter] =
        useState(null);

    const [loading, setLoading] =
        useState(true);

    const [error, setError] =
        useState("");

    useEffect(() => {
        const loadCenter = async () => {
            setLoading(true);
            setError("");

            try {
                const response =
                    await getWorkstationCenter(
                        centerId
                    );

                setCenter(
                    response.center ||
                    response.data ||
                    response
                );
            } catch (error) {
                setError(
                    error.message ||
                    "Unable to load workstation center."
                );
            } finally {
                setLoading(false);
            }
        };

        if (centerId) {
            loadCenter();
        }
    }, [centerId]);

    if (loading) {
        return (
            <Loading message="Loading workstation center..." />
        );
    }

    if (error) {
        return (
            <ErrorMessage message={error} />
        );
    }

    if (!center) {
        return (
            <EmptyState
                title="Center not found"
                message="No workstation center data is available."
            />
        );
    }

    return (
        <div className="space-y-6">
            <PageHeader
                title={center.name || "Workstation Center"}
                description="Workstation center details."
            />

            <div className="rounded-lg border border-slate-200 bg-white p-6 shadow-sm">
                <dl className="space-y-4">
                    {center.name && (
                        <div>
                            <dt className="text-xs font-medium uppercase text-slate-400">
                                Name
                            </dt>

                            <dd className="mt-1 text-sm text-slate-800">
                                {center.name}
                            </dd>
                        </div>
                    )}

                    {center.location && (
                        <div>
                            <dt className="text-xs font-medium uppercase text-slate-400">
                                Location
                            </dt>

                            <dd className="mt-1 text-sm text-slate-800">
                                {center.location}
                            </dd>
                        </div>
                    )}

                    {center.head && (
                        <div>
                            <dt className="text-xs font-medium uppercase text-slate-400">
                                Head
                            </dt>

                            <dd className="mt-1 text-sm text-slate-800">
                                {typeof center.head ===
                                "object"
                                    ? center.head.name ||
                                      center.head.email ||
                                      center.head._id
                                    : center.head}
                            </dd>
                        </div>
                    )}
                </dl>
            </div>
        </div>
    );
}

export default WorkstationCenter;