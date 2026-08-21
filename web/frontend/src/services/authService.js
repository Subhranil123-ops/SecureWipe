import API_BASE_URL from "./api";

export const registerUser = async (userData) => {
    const response = await fetch(
        `${API_BASE_URL}/api/auth/register`,
        {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(userData),
        }
    );

    const data = await response.json();

    if (!response.ok) {
        throw new Error(
            data?.error?.message ||
            data?.message ||
            "Registration failed"
        );
    }

    return data;
};

export const loginUser = async (loginData) => {
    const response = await fetch(
        `${API_BASE_URL}/api/auth/login`,
        {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(loginData),
        }
    );

    const data = await response.json();

    if (!response.ok) {
        throw new Error(
            data?.error?.message ||
            data?.message ||
            "Login failed"
        );
    }

    return data;
};

export const getCurrentUser = async (token) => {
    const response = await fetch(
        `${API_BASE_URL}/api/auth/me`,
        {
            method: "GET",
            headers: {
                Authorization: `Bearer ${token}`,
            },
        }
    );

    const data = await response.json();

    if (!response.ok) {
        const error = new Error(
            data?.error?.message ||
            data?.message ||
            "Unable to fetch current user"
        );

        error.status = response.status;
        error.data = data;

        throw error;
    }

    return data;
};