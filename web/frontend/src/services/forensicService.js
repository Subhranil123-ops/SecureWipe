import { apiRequest } from "./api";

export const getForensicDashboard = async () => {
    const response = await apiRequest(
        "/api/forensics/dashboard",
        {
            method: "GET",
        }
    );

    return response.data || {};
};

export const getForensicCases = async () => {
    const response = await apiRequest(
        "/api/forensics",
        {
            method: "GET",
        }
    );

    return response.data || [];
};

export const getForensicCase = async (
    caseId
) => {
    const response = await apiRequest(
        `/api/forensics/${encodeURIComponent(caseId)}`,
        {
            method: "GET",
        }
    );

    return response.data;
};

export const getForensicAuditTrail = async (caseId) => {
    const response = await apiRequest(
        `/api/forensics/${encodeURIComponent(caseId)}/audit`,
        { method: "GET" }
    );

    return response.data?.events || [];
};


export const createForensicCase = async (
    data
) => {
    const response = await apiRequest(
        "/api/forensics",
        {
            method: "POST",
            body: JSON.stringify(data),
        }
    );

    return response.data;
};

export const assignForensicCase = async (
    caseId,
    employeeId,
    workstationId = ""
) => {
    const response = await apiRequest(
        `/api/forensics/${encodeURIComponent(caseId)}/assign`,
        {
            method: "PATCH",
            body: JSON.stringify({
                employeeId,
                workstationId,
            }),
        }
    );

    return response.data;
};

export const updateForensicStatus = async (
    caseId,
    status,
    note = "",
    workstationId = ""
) => {
    const payload = {
        status,
        note,
    };

    if (workstationId) {
        payload.workstationId =
            workstationId;
    }

    const response = await apiRequest(
        `/api/forensics/${encodeURIComponent(caseId)}/status`,
        {
            method: "PATCH",
            body: JSON.stringify(payload),
        }
    );

    return response.data;
};

export const submitForensicResults = async (
    caseId,
    payload
) => {
    const response = await apiRequest(
        `/api/forensics/${encodeURIComponent(caseId)}/results`,
        {
            method: "POST",
            body: JSON.stringify(payload),
        }
    );

    return response.data;
};

export const generateForensicReport = async (
    caseId
) => {
    const response = await apiRequest(
        `/api/forensics/${encodeURIComponent(caseId)}/report`,
        {
            method: "POST",
        }
    );

    return response.data;
};