export const DEVICE_TYPES = [
    "SSD",
    "HDD",
    "USB Drive",
    "NVMe SSD",
    "Other",
];

export const CAPACITIES = [
    "2 GB",
    "4 GB",
    "8 GB",
    "16 GB",
    "32 GB",
    "64 GB",
    "128 GB",
    "256 GB",
    "512 GB",
    "1 TB",
    "2 TB",
    "Other",
];

const EMAIL_REGEX =
    /^[^\s@]+@[^\s@]+\.[^\s@]+$/;

const PHONE_REGEX =
    /^\+?[0-9\s()-]{7,20}$/;

export function validateSanitizationForm(
    formData
) {
    const errors = {};

    if (!formData.workstationCenter) {
        errors.workstationCenter =
            "Please select a workstation center.";
    }

    const name = formData.name.trim();
    const email = formData.email.trim();
    const phone = formData.phone.trim();

    if (!name) {
        errors.name =
            "Full name is required.";
    } else if (name.length < 2) {
        errors.name =
            "Full name must be at least 2 characters.";
    } else if (name.length > 100) {
        errors.name =
            "Full name must not exceed 100 characters.";
    }

    if (!email) {
        errors.email =
            "Email is required.";
    } else if (!EMAIL_REGEX.test(email)) {
        errors.email =
            "Please enter a valid email address.";
    } else if (email.length > 254) {
        errors.email =
            "Email address is too long.";
    }

    if (!phone) {
        errors.phone =
            "Phone number is required.";
    } else if (!PHONE_REGEX.test(phone)) {
        errors.phone =
            "Please enter a valid phone number.";
    }

    if (
        !DEVICE_TYPES.includes(
            formData.deviceType
        )
    ) {
        errors.deviceType =
            "Please select a valid storage device type.";
    }

    const serialNumber =
        (formData.serialNumber || "").trim();

    if (!serialNumber) {
        errors.serialNumber =
            "Physical device serial number is required.";
    } else if (serialNumber.length > 100) {
        errors.serialNumber =
            "Serial number must not exceed 100 characters.";
    }

    if (
        !CAPACITIES.includes(
            formData.capacity
        )
    ) {
        errors.capacity =
            "Please select a valid storage capacity.";
    }

    if (
        formData.deviceCount === "" ||
        formData.deviceCount === null ||
        formData.deviceCount === undefined
    ) {
        errors.deviceCount =
            "Number of devices is required.";
    } else if (
        !Number.isInteger(
            Number(formData.deviceCount)
        )
    ) {
        errors.deviceCount =
            "Number of devices must be a whole number.";
    } else if (
        Number(formData.deviceCount) < 1
    ) {
        errors.deviceCount =
            "Number of devices must be at least 1.";
    } else if (
        Number(formData.deviceCount) > 100
    ) {
        errors.deviceCount =
            "Number of devices cannot exceed 100.";
    }

    if (
        formData.assetIdentifier.trim()
            .length > 100
    ) {
        errors.assetIdentifier =
            "Asset identifier must not exceed 100 characters.";
    }

    if (
        formData.additionalRequirements
            .trim().length > 1000
    ) {
        errors.additionalRequirements =
            "Additional requirements must not exceed 1000 characters.";
    }

    if (
        formData.notes.trim().length > 2000
    ) {
        errors.notes =
            "Additional notes must not exceed 2000 characters.";
    }

    if (formData.preferredDate) {
        const selectedDate = new Date(
            `${formData.preferredDate}T00:00:00`
        );

        const today = new Date();

        today.setHours(
            0,
            0,
            0,
            0
        );

        if (
            Number.isNaN(
                selectedDate.getTime()
            )
        ) {
            errors.preferredDate =
                "Please enter a valid service date.";
        } else if (
            selectedDate < today
        ) {
            errors.preferredDate =
                "Preferred service date cannot be in the past.";
        }
    }

    if (!formData.consent) {
        errors.consent =
            "You must confirm that you understand the sanitization risks and are authorized to request this operation.";
    }

    return errors;
}