import { useEffect, useState } from "react";

import toast from "react-hot-toast";

import {
    getEligibleCenterHeads,
} from "../../services/userService";

import {
    createWorkstationCenter,
} from "../../services/workstationCenterService";

import Button from "../ui/Button";
import Input from "../ui/Input";
import Loading from "../common/Loading";

function WorkstationCenterForm({
    onCreated,
}) {
    const [heads, setHeads] = useState([]);
    const [loadingHeads, setLoadingHeads] =
        useState(true);

    const [name, setName] = useState("");
    const [location, setLocation] =
        useState("");
    const [head, setHead] = useState("");

    const [submitting, setSubmitting] =
        useState(false);

    useEffect(() => {
        const loadHeads = async () => {
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
                toast.error(
                    error.message ||
                    "Unable to load eligible center heads."
                );
            } finally {
                setLoadingHeads(false);
            }
        };

        loadHeads();
    }, []);

    const handleSubmit = async (event) => {
        event.preventDefault();

        setSubmitting(true);

        try {
            const response =
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

            if (onCreated) {
                onCreated(response);
            }
        } catch (error) {
            toast.error(
                error.message ||
                "Unable to create workstation center."
            );
        } finally {
            setSubmitting(false);
        }
    };

    if (loadingHeads) {
        return (
            <Loading message="Loading center heads..." />
        );
    }

    return (
        <form
            onSubmit={handleSubmit}
            className="space-y-5"
        >
            <Input
                id="center-name"
                label="Center Name"
                value={name}
                onChange={(event) =>
                    setName(event.target.value)
                }
                required
            />

            <Input
                id="center-location"
                label="Location"
                value={location}
                onChange={(event) =>
                    setLocation(event.target.value)
                }
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
                        setHead(event.target.value)
                    }
                    required
                    className="w-full rounded-lg border border-slate-300 px-3 py-2.5 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100"
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
    );
}

export default WorkstationCenterForm;