import { useEffect, useState } from "react";
import { useNavigate } from "react-router-dom";
import toast from "react-hot-toast";

import { createForensicCase } from "../../services/forensicService";
import { getActiveWorkstationCenters } from "../../services/workstationCenterService";

function ForensicNewCase() {
    const navigate = useNavigate();

    const [centers, setCenters] = useState([]);
    const [loadingCenters, setLoadingCenters] = useState(true);
    const [saving, setSaving] = useState(false);

    const [form, setForm] = useState({
        title: "",
        description: "",
        sourceType: "PHYSICAL_DEVICE",
        sourceName: "",
        sourceIdentifier: "",
        deviceType: "",
        capacity: "",
        assetIdentifier: "",
        workstationCenter: "",
    });

    useEffect(() => {
        loadCenters();
    }, []);

    const loadCenters = async () => {
        try {
            setLoadingCenters(true);

            const response = await getActiveWorkstationCenters();

            const loadedCenters = Array.isArray(response?.data)
                ? response.data
                : [];

            setCenters(loadedCenters);
        } catch (error) {
            toast.error(
                error.message ||
                "Unable to load workstation centers"
            );
        } finally {
            setLoadingCenters(false);
        }
    };

    const update = (field, value) => {
        setForm((previous) => ({
            ...previous,
            [field]: value,
        }));
    };

    const handleSubmit = async (event) => {
        event.preventDefault();

        if (!form.title.trim()) {
            return toast.error("Enter a case title");
        }

        if (!form.sourceName.trim()) {
            return toast.error("Enter a source name");
        }

        if (
            form.sourceType === "PHYSICAL_DEVICE" &&
            !form.sourceIdentifier.trim()
        ) {
            return toast.error(
                "Enter the physical device identifier"
            );
        }

        if (!form.workstationCenter) {
            return toast.error(
                "Select a workstation center"
            );
        }

        try {
            setSaving(true);

            const created = await createForensicCase({
                ...form,
                title: form.title.trim(),
                description: form.description.trim(),
                sourceName: form.sourceName.trim(),
                sourceIdentifier:
                    form.sourceIdentifier.trim(),
                deviceType: form.deviceType.trim(),
                capacity: form.capacity.trim(),
                assetIdentifier:
                    form.assetIdentifier.trim(),
            });

            toast.success(
                "Forensic case created successfully"
            );

            navigate(
                `/forensics/cases/${created.caseId}`
            );
        } catch (error) {
            toast.error(
                error.message ||
                "Unable to create forensic case"
            );
        } finally {
            setSaving(false);
        }
    };

    return (
        <div className="mx-auto max-w-4xl space-y-6">
            <div>
                <p className="text-xs font-semibold uppercase tracking-[0.18em] text-indigo-600">
                    New Investigation
                </p>

                <h1 className="mt-1 text-2xl font-semibold text-slate-900">
                    Create Forensic Case
                </h1>

                <p className="mt-1 text-sm text-slate-500">
                    Submit a source for controlled, read-only forensic acquisition.
                </p>
            </div>

            <form
                onSubmit={handleSubmit}
                className="space-y-6"
            >
                <div className="rounded-lg border border-slate-200 bg-white shadow-sm">
                    <div className="border-b border-slate-200 px-5 py-4">
                        <h2 className="text-base font-semibold text-slate-900">
                            Case information
                        </h2>

                        <p className="mt-1 text-sm text-slate-500">
                            Basic information used to identify the investigation.
                        </p>
                    </div>

                    <div className="grid grid-cols-1 gap-5 p-5 md:grid-cols-2">
                        <Field
                            label="Case title *"
                            value={form.title}
                            onChange={(value) =>
                                update("title", value)
                            }
                            placeholder="e.g. USB Evidence Recovery"
                        />

                        <Field
                            label="Asset identifier"
                            value={form.assetIdentifier}
                            onChange={(value) =>
                                update(
                                    "assetIdentifier",
                                    value
                                )
                            }
                            placeholder="Internal asset ID"
                        />

                        <div className="md:col-span-2">
                            <label className="text-sm font-medium text-slate-700">
                                Description
                            </label>

                            <textarea
                                value={form.description}
                                onChange={(event) =>
                                    update(
                                        "description",
                                        event.target.value
                                    )
                                }
                                rows={4}
                                maxLength={2000}
                                placeholder="Describe the investigation objective..."
                                className="mt-1.5 w-full rounded-lg border border-slate-300 px-3 py-2.5 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100"
                            />
                        </div>
                    </div>
                </div>

                <div className="rounded-lg border border-slate-200 bg-white shadow-sm">
                    <div className="border-b border-slate-200 px-5 py-4">
                        <h2 className="text-base font-semibold text-slate-900">
                            Evidence source
                        </h2>

                        <p className="mt-1 text-sm text-slate-500">
                            Identify the storage source that will be examined.
                        </p>
                    </div>

                    <div className="grid grid-cols-1 gap-5 p-5 md:grid-cols-2">
                        <SelectField
                            label="Source type *"
                            value={form.sourceType}
                            onChange={(value) =>
                                update(
                                    "sourceType",
                                    value
                                )
                            }
                            options={[
                                [
                                    "PHYSICAL_DEVICE",
                                    "Physical device",
                                ],
                                [
                                    "FORENSIC_IMAGE",
                                    "Forensic image",
                                ],
                            ]}
                        />

                        <Field
                            label="Source name *"
                            value={form.sourceName}
                            onChange={(value) =>
                                update(
                                    "sourceName",
                                    value
                                )
                            }
                            placeholder="e.g. Evidence USB Drive"
                        />

                        <Field
                            label={
                                form.sourceType ===
                                "PHYSICAL_DEVICE"
                                    ? "Device identifier *"
                                    : "Image identifier"
                            }
                            value={form.sourceIdentifier}
                            onChange={(value) =>
                                update(
                                    "sourceIdentifier",
                                    value
                                )
                            }
                            placeholder={
                                form.sourceType ===
                                "PHYSICAL_DEVICE"
                                    ? "e.g. physical serial number"
                                    : "Image filename or identifier"
                            }
                        />

                        <Field
                            label="Device type"
                            value={form.deviceType}
                            onChange={(value) =>
                                update(
                                    "deviceType",
                                    value
                                )
                            }
                            placeholder="USB / HDD / NVMe / SSD"
                        />

                        <Field
                            label="Capacity"
                            value={form.capacity}
                            onChange={(value) =>
                                update(
                                    "capacity",
                                    value
                                )
                            }
                            placeholder="e.g. 64 GB"
                        />

                        <SelectField
                            label="Workstation center *"
                            value={form.workstationCenter}
                            onChange={(value) =>
                                update(
                                    "workstationCenter",
                                    value
                                )
                            }
                            disabled={loadingCenters}
                            options={[
                                [
                                    "",
                                    loadingCenters
                                        ? "Loading centers..."
                                        : "Select a center",
                                ],

                                ...centers.map(
                                    (center) => [
                                        center.centerId,
                                        `${center.name} (${center.centerId}) — Head: ${center.head?.name || "Not assigned"}`,
                                    ]
                                ),
                            ]}
                        />
                    </div>
                </div>

                <div className="rounded-lg border border-green-200 bg-green-50 p-5">
                    <div className="flex gap-3">
                        <div className="flex h-9 w-9 shrink-0 items-center justify-center rounded-lg bg-green-100 text-green-700">
                            <svg
                                viewBox="0 0 24 24"
                                className="h-5 w-5"
                                fill="none"
                                stroke="currentColor"
                                strokeWidth="1.8"
                            >
                                <path d="M12 3 19 6v5c0 4.5-2.9 8-7 10-4.1-2-7-5.5-7-10V6l7-3Z" />
                                <path d="m9 12 2 2 4-4" />
                            </svg>
                        </div>

                        <div>
                            <p className="text-sm font-semibold text-green-900">
                                Read-only forensic acquisition
                            </p>

                            <p className="mt-1 text-sm leading-5 text-green-700">
                                SecureWipe records this case as read-only. The forensic engine is intended to acquire evidence without modifying the source.
                            </p>
                        </div>
                    </div>
                </div>

                <div className="flex flex-col-reverse gap-3 sm:flex-row sm:justify-end">
                    <button
                        type="button"
                        onClick={() =>
                            navigate("/forensics")
                        }
                        className="rounded-lg border border-slate-300 bg-white px-5 py-2.5 text-sm font-medium text-slate-700 hover:bg-slate-50"
                    >
                        Cancel
                    </button>

                    <button
                        type="submit"
                        disabled={
                            saving ||
                            loadingCenters
                        }
                        className="rounded-lg bg-indigo-600 px-5 py-2.5 text-sm font-medium text-white shadow-sm transition hover:bg-indigo-700 disabled:cursor-not-allowed disabled:opacity-50"
                    >
                        {saving
                            ? "Creating case..."
                            : "Create forensic case"}
                    </button>
                </div>
            </form>
        </div>
    );
}

function Field({
    label,
    value,
    onChange,
    placeholder,
}) {
    return (
        <div>
            <label className="text-sm font-medium text-slate-700">
                {label}
            </label>

            <input
                value={value}
                onChange={(event) =>
                    onChange(
                        event.target.value
                    )
                }
                placeholder={placeholder}
                className="mt-1.5 w-full rounded-lg border border-slate-300 px-3 py-2.5 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100"
            />
        </div>
    );
}

function SelectField({
    label,
    value,
    onChange,
    options,
    disabled = false,
}) {
    return (
        <div>
            <label className="text-sm font-medium text-slate-700">
                {label}
            </label>

            <select
                value={value}
                disabled={disabled}
                onChange={(event) =>
                    onChange(
                        event.target.value
                    )
                }
                className="mt-1.5 w-full rounded-lg border border-slate-300 bg-white px-3 py-2.5 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100 disabled:bg-slate-50"
            >
                {options.map(
                    ([
                        optionValue,
                        labelText,
                    ]) => (
                        <option
                            key={optionValue}
                            value={optionValue}
                        >
                            {labelText}
                        </option>
                    )
                )}
            </select>
        </div>
    );
}

export default ForensicNewCase;