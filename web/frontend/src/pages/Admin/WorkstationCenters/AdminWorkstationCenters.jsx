import { useEffect, useState } from "react";

import toast from "react-hot-toast";

import {
    getEligibleCenterHeads,
} from "../../../services/userService";

import {
    createWorkstationCenter,
} from "../../../services/workstationCenterService";

import Loading from "../../../components/common/Loading";
import ErrorMessage from "../../../components/common/ErrorMessage";
import PageHeader from "../../../components/ui/PageHeader";
import Button from "../../../components/ui/Button";
import Input from "../../../components/ui/Input";

function AdminWorkstationCenters() {
    const [heads, setHeads] = useState([]);

    const [loadingHeads, setLoadingHeads] =
        useState(true);

    const [submitting, setSubmitting] =
        useState(false);

    const [error, setError] = useState("");

    const [name, setName] = useState("");
    const [location, setLocation] =
        useState("");
    const [head, setHead] = useState("");

    useEffect(() => {
        const loadHeads = async () => {
            setLoadingHeads(true);
            setError("");

            try {
                const response =
                    await getEligibleCenterHeads();

                setHeads(
                    Array.isArray(response)
                        ? response
                        : response.users ||
                              response.data ||
                              []
                );
            } catch (error) {
                setError(
                    error.message ||
                    "Unable to load eligible workstation heads."
                );
            } finally {
                setLoadingHeads(false);
            }
        };

        loadHeads();
    }, []);

    const handleSubmit = async (event) => {
        event.preventDefault();

        if (!head) {
            toast.error(
                "Please select a workstation head"
            );
            return;
        }

        setSubmitting(true);

        try {
            await createWorkstationCenter({
                name,
                location,
                head,
            });

            toast.success(
                "Workstation center created successfully"
            );

            setName("");
            setLocation("");
            setHead("");
        } catch (error) {
            toast.error(
                error.message ||
                "Unable to create workstation center"
            );
        } finally {
            setSubmitting(false);
        }
    };

    if (loadingHeads) {
        return (
            <Loading message="Loading workstation heads..." />
        );
    }

    return (
        <div className="space-y-6">
            <PageHeader
                title="Workstation Centers"
                description="Create a workstation center using an eligible center head."
            />

            {error && (
                <ErrorMessage message={error} />
            )}

            <div className="max-w-xl rounded-lg border border-slate-200 bg-white p-6 shadow-sm">
                <form
                    onSubmit={handleSubmit}
                    className="space-y-5"
                >
                    <Input
                        id="center-name"
                        label="Center Name"
                        value={name}
                        onChange={(event) =>
                            setName(
                                event.target.value
                            )
                        }
                        placeholder="Enter center name"
                        required
                    />

                    <Input
                        id="center-location"
                        label="Location"
                        value={location}
                        onChange={(event) =>
                            setLocation(
                                event.target.value
                            )
                        }
                        placeholder="Enter center location"
                        required
                    />

                    <div className="space-y-2">
                        <label
                            htmlFor="center-head"
                            className="block text-sm font-medium text-slate-700"
                        >
                            Workstation Head
                        </label>

                        <select
                            id="center-head"
                            value={head}
                            onChange={(event) =>
                                setHead(
                                    event.target.value
                                )
                            }
                            required
                            className="w-full rounded-lg border border-slate-300 bg-white px-3 py-2.5 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100"
                        >
                            <option value="">
                                Select workstation head
                            </option>

                            {heads.map((item) => (
                                <option
                                    key={item._id}
                                    value={item._id}
                                >
                                    {item.name} -{" "}
                                    {item.email}
                                </option>
                            ))}
                        </select>
                    </div>

                    <Button
                        type="submit"
                        disabled={submitting}
                    >
                        {submitting
                            ? "Creating..."
                            : "Create Center"}
                    </Button>
                </form>
            </div>
        </div>
    );
}

export default AdminWorkstationCenters;