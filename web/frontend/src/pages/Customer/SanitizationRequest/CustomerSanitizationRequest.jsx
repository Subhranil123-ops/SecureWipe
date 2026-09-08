import { useMemo, useState, useEffect } from "react";
import { apiRequest } from "../../../services/api";
import { getActiveWorkstationCenters, } from "../../../services/workstationCenterService";


import {
    DEVICE_TYPES,
    CAPACITIES,
    validateSanitizationForm,
} from "../../../utils/sanitizationValidation";

const INITIAL_FORM_DATA = {
    name: "",
    email: "",
    phone: "",
    workstationCenter: "",
    deviceType: "",
    capacity: "",
    deviceCount: 1,
    assetIdentifier: "",
    serialNumber: "",
    additionalRequirements: "",
    preferredDate: "",
    notes: "",
    consent: false,
};

function FieldError({ message, id }) {
    if (!message) {
        return null;
    }

    return (
        <p
            id={id}
            className="mt-1 text-sm text-red-600"
            role="alert"
        >
            {message}
        </p>
    );
}

function InputField({
    label,
    name,
    type = "text",
    value,
    onChange,
    onBlur,
    error,
    placeholder,
    required = false,
    min,
}) {
    const errorId = `${name}-error`;

    return (
        <div>
            <label
                htmlFor={name}
                className="mb-1 block text-sm font-medium text-gray-700"
            >
                {label}

                {required && (
                    <span
                        className="ml-1 text-red-600"
                        aria-hidden="true"
                    >
                        *
                    </span>
                )}
            </label>

            <input
                id={name}
                name={name}
                type={type}
                value={value}
                onChange={onChange}
                onBlur={onBlur}
                placeholder={placeholder}
                min={min}
                required={required}
                aria-invalid={Boolean(error)}
                aria-describedby={
                    error ? errorId : undefined
                }
                className={`w-full rounded-lg border px-3 py-2 text-sm outline-none transition focus:ring-2 ${error
                    ? "border-red-500 focus:ring-red-200"
                    : "border-gray-300 focus:border-blue-500 focus:ring-blue-100"
                    }`}
            />

            <FieldError
                message={error}
                id={errorId}
            />
        </div>
    );
}

function SelectField({
    label,
    name,
    value,
    onChange,
    onBlur,
    options,
    error,
    required = false,
}) {
    const errorId = `${name}-error`;

    return (
        <div>
            <label
                htmlFor={name}
                className="mb-1 block text-sm font-medium text-gray-700"
            >
                {label}

                {required && (
                    <span
                        className="ml-1 text-red-600"
                        aria-hidden="true"
                    >
                        *
                    </span>
                )}
            </label>

            <select
                id={name}
                name={name}
                value={value}
                onChange={onChange}
                onBlur={onBlur}
                required={required}
                aria-invalid={Boolean(error)}
                aria-describedby={
                    error ? errorId : undefined
                }
                className={`w-full rounded-lg border px-3 py-2 text-sm outline-none transition focus:ring-2 ${error
                    ? "border-red-500 focus:ring-red-200"
                    : "border-gray-300 focus:border-blue-500 focus:ring-blue-100"
                    }`}
            >
                <option value="">
                    Select {label.toLowerCase()}
                </option>

                {options.map((option) => (
                    <option
                        key={option}
                        value={option}
                    >
                        {option}
                    </option>
                ))}
            </select>

            <FieldError
                message={error}
                id={errorId}
            />
        </div>
    );
}

function TextAreaField({
    label,
    name,
    value,
    onChange,
    onBlur,
    error,
    placeholder,
    maxLength,
}) {
    const errorId = `${name}-error`;

    return (
        <div>
            <label
                htmlFor={name}
                className="mb-1 block text-sm font-medium text-gray-700"
            >
                {label}
            </label>

            <textarea
                id={name}
                name={name}
                value={value}
                onChange={onChange}
                onBlur={onBlur}
                placeholder={placeholder}
                maxLength={maxLength}
                aria-invalid={Boolean(error)}
                aria-describedby={
                    error ? errorId : undefined
                }
                rows={4}
                className={`w-full rounded-lg border px-3 py-2 text-sm outline-none transition focus:ring-2 ${error
                    ? "border-red-500 focus:ring-red-200"
                    : "border-gray-300 focus:border-blue-500 focus:ring-blue-100"
                    }`}
            />

            <FieldError
                message={error}
                id={errorId}
            />
        </div>
    );
}

export default function CustomerSanitizationRequest() {
    const [formData, setFormData] = useState(INITIAL_FORM_DATA);

    const [workstationCenters, setWorkstationCenters,] = useState([]);
    const [isLoadingCenters, setIsLoadingCenters,] = useState(true);
    const [centerLoadError, setCenterLoadError,] = useState("");

    const [errors, setErrors] = useState({});
    const [touched, setTouched] = useState({});

    const [isSubmitting, setIsSubmitting] = useState(false);
    const [submitError, setSubmitError] = useState("");
    const [success, setSuccess] = useState(null);

    const today = new Date().toISOString().split("T")[0];

    useEffect(() => {
        const loadWorkstationCenters = async () => {
            setIsLoadingCenters(true);
            setCenterLoadError("");

            try {
                const response =
                    await getActiveWorkstationCenters();

                setWorkstationCenters(
                    response?.data || []
                );
            } catch (error) {
                console.error(
                    "Failed to load workstation centers:",
                    error
                );

                setCenterLoadError(
                    error?.message ||
                    "Unable to load workstation centers."
                );
            } finally {
                setIsLoadingCenters(false);
            }
        };

        loadWorkstationCenters();
    }, []);

    const summary = useMemo(
        () => ({
            customer: formData.name || "—",
            deviceType:
                formData.deviceType || "—",
            capacity:
                formData.capacity || "—",
            deviceCount:
                formData.deviceCount || "—",
            serialNumber:
                formData.serialNumber ||
                "—",
            preferredDate:
                formData.preferredDate || "—",
        }),
        [formData]
    );

    const handleChange = (event) => {
        const {
            name,
            value,
            type,
            checked,
        } = event.target;

        const updatedValue =
            type === "checkbox"
                ? checked
                : value;

        const updatedFormData = {
            ...formData,
            [name]: updatedValue,
        };

        setFormData(updatedFormData);
        setSubmitError("");

        if (touched[name]) {
            const validationErrors =
                validateSanitizationForm(
                    updatedFormData
                );

            setErrors(
                (previousErrors) => {
                    const nextErrors = {
                        ...previousErrors,
                    };

                    if (
                        validationErrors[name]
                    ) {
                        nextErrors[name] =
                            validationErrors[name];
                    } else {
                        delete nextErrors[name];
                    }

                    return nextErrors;
                }
            );
        }
    };

    const handleBlur = (event) => {
        const { name } = event.target;

        const nextTouched = {
            ...touched,
            [name]: true,
        };

        setTouched(nextTouched);

        const validationErrors =
            validateSanitizationForm(
                formData
            );

        setErrors(
            (previousErrors) => {
                const nextErrors = {
                    ...previousErrors,
                };

                if (validationErrors[name]) {
                    nextErrors[name] =
                        validationErrors[name];
                } else {
                    delete nextErrors[name];
                }

                return nextErrors;
            }
        );
    };

    const handleSubmit = async (event) => {
        event.preventDefault();

        if (isSubmitting) {
            return;
        }

        setSubmitError("");
        setSuccess(null);

        const validationErrors =
            validateSanitizationForm(
                formData
            );

        setErrors(validationErrors);

        const allTouched =
            Object.keys(formData).reduce(
                (result, fieldName) => {
                    result[fieldName] = true;
                    return result;
                },
                {}
            );

        setTouched(allTouched);

        if (
            Object.keys(validationErrors)
                .length > 0
        ) {
            return;
        }

        setIsSubmitting(true);

        try {
            const response =
                await apiRequest(
                    "/api/sanitization-requests",
                    {
                        method: "POST",

                        body: JSON.stringify({
                            name: formData.name,

                            email: formData.email,

                            phone: formData.phone,

                            workstationCenter: formData.workstationCenter,

                            deviceType: formData.deviceType,

                            capacity: formData.capacity,

                            deviceCount: Number(formData.deviceCount),

                            assetIdentifier:
                                formData.assetIdentifier,

                            serialNumber:
                                formData.serialNumber.trim(),

                            additionalRequirements:
                                formData.additionalRequirements,

                            preferredDate:
                                formData.preferredDate ||
                                null,

                            notes:
                                formData.notes,

                            consent:
                                formData.consent,
                        }),
                    }
                );

            setSuccess({
                requestId:
                    response.data.requestId,
            });
        } catch {
            setSubmitError(
                "Unable to submit the request. Please try again."
            );
        } finally {
            setIsSubmitting(false);
        }
    };

    if (success) {
        return (
            <div className="mx-auto max-w-4xl px-4 py-8">
                <div className="rounded-xl border border-green-200 bg-green-50 p-6">
                    <h1 className="text-2xl font-semibold text-green-800">
                        Sanitization request submitted successfully.
                    </h1>

                    <p className="mt-3 text-sm text-green-700">
                        Request ID:

                        <span className="ml-2 font-semibold">
                            {success.requestId}
                        </span>
                    </p>

                    <p className="mt-2 text-sm text-green-700">
                        Your request has been recorded and is currently pending.
                    </p>

                    <button
                        type="button"
                        onClick={() => {
                            setSuccess(null);
                            setFormData(
                                INITIAL_FORM_DATA
                            );
                            setErrors({});
                            setTouched({});
                        }}
                        className="mt-5 rounded-lg bg-blue-600 px-4 py-2 text-sm font-medium text-white hover:bg-blue-700"
                    >
                        Submit Another Request
                    </button>
                </div>
            </div>
        );
    }

    return (
        <div className="mx-auto max-w-5xl px-4 py-8">
            <div className="mb-8">
                <h1 className="text-3xl font-bold text-gray-900">
                    Request Sanitization
                </h1>

                <p className="mt-2 text-gray-600">
                    Submit a request to securely sanitize your
                    storage device or data.
                </p>
            </div>

            <form
                onSubmit={handleSubmit}
                noValidate
            >
                <div className="space-y-6">

                    <section className="rounded-xl border border-gray-200 bg-white p-6 shadow-sm">
                        <h2 className="text-xl font-semibold text-gray-900">
                            1. Customer Information
                        </h2>

                        <div className="mt-5 grid gap-5 md:grid-cols-2">

                            <InputField
                                label="Full Name"
                                name="name"
                                value={formData.name}
                                onChange={handleChange}
                                onBlur={handleBlur}
                                error={errors.name}
                                placeholder="Enter your full name"
                                required
                            />

                            <InputField
                                label="Email"
                                name="email"
                                type="email"
                                value={formData.email}
                                onChange={handleChange}
                                onBlur={handleBlur}
                                error={errors.email}
                                placeholder="user@example.com"
                                required
                            />

                            <InputField
                                label="Phone Number"
                                name="phone"
                                type="tel"
                                value={formData.phone}
                                onChange={handleChange}
                                onBlur={handleBlur}
                                error={errors.phone}
                                placeholder="+91 9876543210"
                                required
                            />

                            <div>
                                <label
                                    htmlFor="workstationCenter"
                                    className="mb-1 block text-sm font-medium text-gray-700"
                                >
                                    Workstation Center

                                    <span
                                        className="ml-1 text-red-600"
                                        aria-hidden="true"
                                    >
                                        *
                                    </span>
                                </label>

                                <select
                                    id="workstationCenter"
                                    name="workstationCenter"
                                    value={formData.workstationCenter}
                                    onChange={handleChange}
                                    onBlur={handleBlur}
                                    disabled={isLoadingCenters}
                                    required
                                    aria-invalid={Boolean(
                                        errors.workstationCenter
                                    )}
                                    aria-describedby={
                                        errors.workstationCenter
                                            ? "workstationCenter-error"
                                            : undefined
                                    }
                                    className={`w-full rounded-lg border px-3 py-2 text-sm outline-none transition focus:ring-2 ${errors.workstationCenter
                                            ? "border-red-500 focus:ring-red-200"
                                            : "border-gray-300 focus:border-blue-500 focus:ring-blue-100"
                                        }`}
                                >
                                    <option value="">
                                        {isLoadingCenters
                                            ? "Loading workstation centers..."
                                            : "Select workstation center"}
                                    </option>

                                    {workstationCenters.map((center) => {
                                        const centerName =
                                            center.name ||
                                            center.centerId ||
                                            "Unnamed workstation center";

                                        const centerCode =
                                            center.centerId ||
                                            "N/A";

                                        const city =
                                            center.location?.city ||
                                            center.location?.state ||
                                            "";

                                        const headName =
                                            center.head?.name ||
                                            "Head not assigned";

                                        const headEmail =
                                            center.head?.email ||
                                            "";

                                        return (
                                            <option
                                                key={center.centerId}
                                                value={center.centerId}
                                            >
                                                {centerCode} — {centerName}
                                                {city ? ` — ${city}` : ""}
                                                {headEmail
                                                    ? ` — Head: ${headName}`
                                                    : ` — Head: ${headName}`}
                                            </option>
                                        );
                                    })}
                                </select>

                                <FieldError
                                    message={
                                        errors.workstationCenter
                                    }
                                    id="workstationCenter-error"
                                />

                                {centerLoadError && (
                                    <p
                                        className="mt-1 text-sm text-red-600"
                                        role="alert"
                                    >
                                        {centerLoadError}
                                    </p>
                                )}
                            </div>

                        </div>
                    </section>

                    <section className="rounded-xl border border-gray-200 bg-white p-6 shadow-sm">
                        <h2 className="text-xl font-semibold text-gray-900">
                            2. Device Information
                        </h2>

                        <div className="mt-5 grid gap-5 md:grid-cols-2">

                            <SelectField
                                label="Storage Device Type"
                                name="deviceType"
                                value={formData.deviceType}
                                onChange={handleChange}
                                onBlur={handleBlur}
                                options={DEVICE_TYPES}
                                error={errors.deviceType}
                                required
                            />

                            <SelectField
                                label="Approximate Storage Capacity"
                                name="capacity"
                                value={formData.capacity}
                                onChange={handleChange}
                                onBlur={handleBlur}
                                options={CAPACITIES}
                                error={errors.capacity}
                                required
                            />

                            <InputField
                                label="Number of Devices"
                                name="deviceCount"
                                type="number"
                                value={formData.deviceCount}
                                onChange={handleChange}
                                onBlur={handleBlur}
                                error={errors.deviceCount}
                                min="1"
                                required
                            />

                            <InputField
                                label="Physical Device Serial Number"
                                name="serialNumber"
                                value={formData.serialNumber}
                                onChange={handleChange}
                                onBlur={handleBlur}
                                error={errors.serialNumber}
                                placeholder="Enter the exact device serial number"
                                required
                            />

                            <InputField
                                label="Device / Asset Identifier"
                                name="assetIdentifier"
                                value={formData.assetIdentifier}
                                onChange={handleChange}
                                onBlur={handleBlur}
                                error={errors.assetIdentifier}
                                placeholder="Optional"
                            />

                        </div>
                    </section>

                    <section className="rounded-xl border border-gray-200 bg-white p-6 shadow-sm">
                        <h2 className="text-xl font-semibold text-gray-900">
                            3. Sanitization Requirements
                        </h2>

                        <div className="mt-5 space-y-5">

                            <div className="rounded-lg border border-blue-100 bg-blue-50 p-4 text-sm text-blue-800">
                                The sanitization method is selected automatically by the workstation capability engine after the authorized physical device is detected and safety-validated.
                            </div>

                            <TextAreaField
                                label="Additional Requirements"
                                name="additionalRequirements"
                                value={
                                    formData.additionalRequirements
                                }
                                onChange={handleChange}
                                onBlur={handleBlur}
                                error={
                                    errors.additionalRequirements
                                }
                                placeholder="Optional additional requirements"
                                maxLength={1000}
                            />

                        </div>
                    </section>

                    <section className="rounded-xl border border-gray-200 bg-white p-6 shadow-sm">
                        <h2 className="text-xl font-semibold text-gray-900">
                            4. Request Details
                        </h2>

                        <div className="mt-5 space-y-5">

                            <InputField
                                label="Preferred Service Date"
                                name="preferredDate"
                                type="date"
                                value={
                                    formData.preferredDate
                                }
                                onChange={handleChange}
                                onBlur={handleBlur}
                                error={
                                    errors.preferredDate
                                }
                                min={today}
                            />

                            <TextAreaField
                                label="Additional Notes"
                                name="notes"
                                value={formData.notes}
                                onChange={handleChange}
                                onBlur={handleBlur}
                                error={errors.notes}
                                placeholder="Optional notes"
                                maxLength={2000}
                            />

                        </div>
                    </section>

                    <section className="rounded-xl border border-red-200 bg-red-50 p-6">
                        <h2 className="text-xl font-semibold text-red-900">
                            5. Safety / Consent
                        </h2>

                        <div className="mt-4 rounded-lg border border-red-200 bg-white p-4">
                            <p className="text-sm leading-6 text-red-800">
                                Sanitization is a destructive operation and
                                may permanently remove data. Make sure you
                                have backed up any data you need before
                                requesting sanitization.
                            </p>
                        </div>

                        <div className="mt-5">
                            <label className="flex items-start gap-3">

                                <input
                                    type="checkbox"
                                    name="consent"
                                    checked={
                                        formData.consent
                                    }
                                    onChange={handleChange}
                                    onBlur={handleBlur}
                                    className="mt-1 h-4 w-4 rounded border-gray-300"
                                    aria-invalid={Boolean(
                                        errors.consent
                                    )}
                                    aria-describedby={
                                        errors.consent
                                            ? "consent-error"
                                            : undefined
                                    }
                                />

                                <span className="text-sm leading-6 text-gray-700">
                                    I understand that sanitization may permanently
                                    remove data and I confirm that I am authorized
                                    to request this operation.
                                </span>

                            </label>

                            <FieldError
                                message={errors.consent}
                                id="consent-error"
                            />
                        </div>
                    </section>

                    <section className="rounded-xl border border-gray-200 bg-white p-6 shadow-sm">
                        <h2 className="text-xl font-semibold text-gray-900">
                            6. Request Summary
                        </h2>

                        <div className="mt-5 grid gap-4 sm:grid-cols-2">

                            <div>
                                <p className="text-xs font-medium uppercase text-gray-500">
                                    Customer
                                </p>

                                <p className="mt-1 text-sm text-gray-900">
                                    {summary.customer}
                                </p>
                            </div>

                            <div>
                                <p className="text-xs font-medium uppercase text-gray-500">
                                    Device Type
                                </p>

                                <p className="mt-1 text-sm text-gray-900">
                                    {summary.deviceType}
                                </p>
                            </div>

                            <div>
                                <p className="text-xs font-medium uppercase text-gray-500">
                                    Capacity
                                </p>

                                <p className="mt-1 text-sm text-gray-900">
                                    {summary.capacity}
                                </p>
                            </div>

                            <div>
                                <p className="text-xs font-medium uppercase text-gray-500">
                                    Number of Devices
                                </p>

                                <p className="mt-1 text-sm text-gray-900">
                                    {summary.deviceCount}
                                </p>
                            </div>

                            <div>
                                <p className="text-xs font-medium uppercase text-gray-500">
                                    Physical Serial Number
                                </p>

                                <p className="mt-1 text-sm text-gray-900 break-all">
                                    {summary.serialNumber}
                                </p>
                            </div>

                            <div>
                                <p className="text-xs font-medium uppercase text-gray-500">
                                    Preferred Date
                                </p>

                                <p className="mt-1 text-sm text-gray-900">
                                    {summary.preferredDate}
                                </p>
                            </div>

                        </div>
                    </section>

                    <section>

                        {submitError && (
                            <div
                                className="mb-4 rounded-lg border border-red-200 bg-red-50 p-4 text-sm text-red-700"
                                role="alert"
                            >
                                {submitError}
                            </div>
                        )}

                        <button
                            type="submit"
                            disabled={isSubmitting}
                            className="w-full rounded-lg bg-blue-600 px-5 py-3 text-sm font-semibold text-white transition hover:bg-blue-700 disabled:cursor-not-allowed disabled:opacity-60"
                        >
                            {isSubmitting
                                ? "Submitting..."
                                : "Submit Sanitization Request"}
                        </button>

                    </section>

                </div>
            </form>
        </div>
    );
}