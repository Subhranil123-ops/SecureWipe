import { apiRequest } from "./api";

export const getSanitizationLiveProgress =
    async (
        requestId
    ) => {
        const response =
            await apiRequest(
                `/api/live-progress/sanitization/${encodeURIComponent(
                    requestId
                )}`,
                {
                    method: "GET"
                }
            );

        return response.data || null;
    };

export const getForensicLiveProgress =
    async (
        caseId
    ) => {
        const response =
            await apiRequest(
                `/api/live-progress/forensics/${encodeURIComponent(
                    caseId
                )}`,
                {
                    method: "GET"
                }
            );

        return response.data || null;
    };