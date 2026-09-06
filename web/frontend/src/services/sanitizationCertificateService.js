import { apiRequest } from "./api";

export const submitSanitizationCertificate =
    async (
        requestId,
        payload
    ) => {

        const response =
            await apiRequest(
                `/api/sanitization-certificates/${requestId}`,
                {
                    method: "POST",
                    body: JSON.stringify(
                        payload
                    ),
                }
            );

        return response.data;
    };

export const getSanitizationCertificate =
    async (
        certificateId
    ) => {

        const response =
            await apiRequest(
                `/api/sanitization-certificates/${certificateId}`
            );

        return response.data;
    };

export const verifySanitizationCertificate =
    async (
        certificateId
    ) => {

        const response =
            await apiRequest(
                `/api/sanitization-certificates/${certificateId}/verify`
            );

        return response.data;
    };