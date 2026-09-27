import {
    apiRequest
} from "./api";


/*
 * ============================================================
 * GET CENTRAL CERTIFICATE REGISTRY
 * ============================================================
 *
 * Returns:
 *
 * {
 *     sanitization: [...],
 *     forensic: [...]
 * }
 *
 * Backend applies role-based visibility.
 */

export const getCertificateRegistry =
    async () => {

        const response =
            await apiRequest(
                "/api/certificates",
                {
                    method:
                        "GET"
                }
            );

        return (
            response.data || {
                sanitization: [],
                forensic: []
            }
        );
    };


/*
 * ============================================================
 * GET SANITIZATION CERTIFICATE
 * ============================================================
 */

export const getSanitizationCertificateDetails =
    async (
        certificateId
    ) => {

        const response =
            await apiRequest(
                `/api/certificates/sanitization/${encodeURIComponent(
                    certificateId
                )}`,
                {
                    method:
                        "GET"
                }
            );

        return response.data;
    };


/*
 * ============================================================
 * GET FORENSIC CERTIFICATE
 * ============================================================
 *
 * Forensics certificates are tied to a forensic CASE.
 */

export const getForensicCertificateDetails =
    async (
        caseId
    ) => {

        const response =
            await apiRequest(
                `/api/certificates/forensic/${encodeURIComponent(
                    caseId
                )}`,
                {
                    method:
                        "GET"
                }
            );

        return response.data;
    };