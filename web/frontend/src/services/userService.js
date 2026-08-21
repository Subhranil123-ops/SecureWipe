import { apiRequest } from "./api";

export const getAllUsers = async () => {
    return apiRequest("/api/users", {
        method: "GET",
    });
};

export const updateUserRole = async (
    userId,
    role
) => {
    return apiRequest(
        `/api/users/${userId}/role`,
        {
            method: "PATCH",
            body: JSON.stringify({
                role,
            }),
        }
    );
};

export const getEligibleCenterHeads =
    async () => {
        return apiRequest(
            "/api/users/eligible-center-heads",
            {
                method: "GET",
            }
        );
    };