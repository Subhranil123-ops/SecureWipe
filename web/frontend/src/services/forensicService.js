import API_BASE_URL, {
    apiRequest,
    getAuthToken
} from "./api";

export const getForensicDashboard =
    async () => {
        const response =
            await apiRequest(
                "/api/forensics/dashboard",
                {
                    method:
                        "GET"
                }
            );

        return (
            response.data ||
            {}
        );
    };

export const getForensicCases =
    async () => {
        const response =
            await apiRequest(
                "/api/forensics",
                {
                    method:
                        "GET"
                }
            );

        return (
            response.data ||
            []
        );
    };

export const getForensicCase =
    async (
        caseId
    ) => {
        const response =
            await apiRequest(
                `/api/forensics/${encodeURIComponent(
                    caseId
                )}`,
                {
                    method:
                        "GET"
                }
            );

        return response.data;
    };

export const getForensicAuditTrail =
    async (
        caseId
    ) => {
        const response =
            await apiRequest(
                `/api/forensics/${encodeURIComponent(
                    caseId
                )}/audit`,
                {
                    method:
                        "GET"
                }
            );

        return (
            response.data?.events ||
            []
        );
    };

export const createForensicCase =
    async (
        data
    ) => {
        const response =
            await apiRequest(
                "/api/forensics",
                {
                    method:
                        "POST",
                    body:
                        JSON.stringify(
                            data
                        )
                }
            );

        return response.data;
    };

export const assignForensicCase =
    async (
        caseId,
        employeeId,
        workstationId = ""
    ) => {
        const response =
            await apiRequest(
                `/api/forensics/${encodeURIComponent(
                    caseId
                )}/assign`,
                {
                    method:
                        "PATCH",

                    body:
                        JSON.stringify(
                            {
                                employeeId,
                                workstationId
                            }
                        )
                }
            );

        return response.data;
    };

export const updateForensicStatus =
    async (
        caseId,
        status,
        note = "",
        workstationId = ""
    ) => {
        const payload = {
            status,
            note
        };

        if (
            workstationId
        ) {
            payload.workstationId =
                workstationId;
        }

        const response =
            await apiRequest(
                `/api/forensics/${encodeURIComponent(
                    caseId
                )}/status`,
                {
                    method:
                        "PATCH",

                    body:
                        JSON.stringify(
                            payload
                        )
                }
            );

        return response.data;
    };

export const submitForensicResults =
    async (
        caseId,
        payload
    ) => {
        const response =
            await apiRequest(
                `/api/forensics/${encodeURIComponent(
                    caseId
                )}/results`,
                {
                    method:
                        "POST",

                    body:
                        JSON.stringify(
                            payload
                        )
                }
            );

        return response.data;
    };

export const generateForensicReport =
    async (
        caseId
    ) => {
        const response =
            await apiRequest(
                `/api/forensics/${encodeURIComponent(
                    caseId
                )}/report`,
                {
                    method:
                        "POST"
                }
            );

        return response.data;
    };

export const getForensicIntegrity =
    async (
        caseId
    ) => {
        const response =
            await apiRequest(
                `/api/forensics/${encodeURIComponent(
                    caseId
                )}/integrity`,
                {
                    method:
                        "GET"
                }
            );

        return (
            response.data ||
            {}
        );
    };

export const verifyForensicIntegrity =
    async (
        caseId
    ) => {
        const response =
            await apiRequest(
                `/api/forensics/${encodeURIComponent(
                    caseId
                )}/integrity/verify`,
                {
                    method:
                        "POST"
                }
            );

        return (
            response.data ||
            {}
        );
    };

export const getForensicArtifactBlob =
    async (
        caseId,
        artifactId
    ) => {
        const token =
            getAuthToken();

        const response =
            await fetch(
                `${API_BASE_URL}/api/forensics/${encodeURIComponent(
                    caseId
                )}/artifacts/${encodeURIComponent(
                    artifactId
                )}/content`,
                {
                    method:
                        "GET",

                    headers:
                        token
                            ? {
                                  Authorization:
                                      `Bearer ${token}`
                              }
                            : {}
                }
            );

        if (
            !response.ok
        ) {
            let message =
                "Unable to load recovered evidence";

            try {
                const data =
                    await response.json();

                message =
                    data?.error?.message ||
                    data?.message ||
                    message;
            } catch {
                // Keep default message.
            }

            const error =
                new Error(
                    message
                );

            error.status =
                response.status;

            throw error;
        }

        return response.blob();
    };