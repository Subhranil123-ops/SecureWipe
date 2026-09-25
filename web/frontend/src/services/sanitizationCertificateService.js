import { apiRequest } from "./api";

// ============================================================
// SANITIZATION CERTIFICATE
// ============================================================

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

// ============================================================
// SANITIZATION AUDIT CHAIN
// ============================================================

export const submitSanitizationAuditChain =
    async (
        requestId,
        payload
    ) => {
        const response =
            await apiRequest(
                `/api/sanitization-audit/${requestId}`,
                {
                    method: "POST",
                    body: JSON.stringify(
                        payload
                    ),
                }
            );

        return response.data;
    };

export const getSanitizationAuditChain =
    async (
        requestId
    ) => {
        const response =
            await apiRequest(
                `/api/sanitization-audit/${requestId}`
            );

        return response.data;
    };

export const verifySanitizationAuditChain =
    async (
        requestId
    ) => {
        const response =
            await apiRequest(
                `/api/sanitization-audit/${requestId}/verify`
            );

        return response.data;
    };