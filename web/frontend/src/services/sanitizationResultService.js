import { apiRequest } from "./api";

export const getSanitizationResult =
    async (
        requestId
    ) => {

        const response =
            await apiRequest(
                `/api/sanitization-results/${requestId}`
            );

        return response.data;
    };

export const submitSanitizationResult =
    async (
        requestId,
        payload
    ) => {

        const response =
            await apiRequest(
                `/api/sanitization-results/${requestId}`,
                {
                    method: "POST",
                    body: JSON.stringify(
                        payload
                    ),
                }
            );

        return response.data;
    };