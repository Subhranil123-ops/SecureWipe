import { apiRequest } from "./api";

export const createSanitizationRequest =
    async (data) => {
        const response =
            await apiRequest(
                "/api/sanitization-requests",
                {
                    method: "POST",
                    body: JSON.stringify(data),
                }
            );

        return response.data;
    };

export const getMySanitizationRequests =
    async () => {
        const response =
            await apiRequest(
                "/api/sanitization-requests/my"
            );

        return response.data || [];
    };

export const getAllSanitizationRequests =
    async () => {
        const response =
            await apiRequest(
                "/api/sanitization-requests"
            );

        return response.data || [];
    };

export const getHeadSanitizationRequests =
    async () => {
        const response =
            await apiRequest(
                "/api/sanitization-requests/head"
            );

        return response.data || [];
    };

export const getHeadApprovedSanitizationRequests =
    async () => {
        const response =
            await apiRequest(
                "/api/sanitization-requests/head/approved"
            );

        return response.data || [];
    };

export const updateSanitizationRequestStatus =
    async (
        requestId,
        data
    ) => {
        const response =
            await apiRequest(
                `/api/sanitization-requests/${requestId}/status`,
                {
                    method: "PATCH",
                    body: JSON.stringify(data),
                }
            );

        return response.data;
    };

export const assignSanitizationRequest =
    async (
        requestId,
        data
    ) => {
        const response =
            await apiRequest(
                `/api/sanitization-requests/${requestId}/assign`,
                {
                    method: "PATCH",
                    body: JSON.stringify(data),
                }
            );

        return response.data;
    };

export const getAllHeadSanitizationRequests =
    async () => {
        const response =
            await apiRequest(
                "/api/sanitization-requests/head/all"
            );

        return response.data || [];
    };

export const getEmployeeSanitizationRequests =
    async () => {
        const response =
            await apiRequest(
                "/api/sanitization-requests/employee"
            );

        return response.data || [];
    };

export const updateEmployeeSanitizationStatus =
    async (
        requestId,
        status
    ) => {
        const response =
            await apiRequest(
                `/api/sanitization-requests/${requestId}/employee-status`,
                {
                    method: "PATCH",
                    body: JSON.stringify({
                        status,
                    }),
                }
            );

        return response.data;
    };