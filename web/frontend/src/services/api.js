const API_BASE_URL = import.meta.env.VITE_API_URL;

const getToken = () => {
    return localStorage.getItem("securewipe_token");
};

export const apiRequest = async (endpoint, options = {}) => {
    const token = getToken();

    const headers = {
        ...(options.body
            ? { "Content-Type": "application/json" }
            : {}),
        ...(options.headers || {}),
    };

    if (token) {
        headers.Authorization = `Bearer ${token}`;
    }

    const response = await fetch(
        `${API_BASE_URL}${endpoint}`,
        {
            ...options,
            headers,
        }
    );

    let data = null;

    try {
        data = await response.json();
    } catch {
        data = null;
    }

    if (!response.ok) {
        const error = new Error(
            data?.error?.message ||
            data?.message ||
            "Something went wrong"
        );

        error.status = response.status;
        error.data = data;

        throw error;
    }

    return data;
};

export const getAuthToken = () => {
    return getToken();
};

export const clearAuthToken = () => {
    localStorage.removeItem("securewipe_token");
};

export default API_BASE_URL;