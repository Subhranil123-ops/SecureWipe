import {
    createContext,
    useCallback,
    useContext,
    useEffect,
    useState,
} from "react";

import {
    getCurrentUser,
    loginUser,
} from "../services/authService";

import {
    clearAuthToken,
} from "../services/api";

const AuthContext = createContext(null);

const TOKEN_KEY = "securewipe_token";

export const AuthProvider = ({ children }) => {
    const [token, setToken] = useState(
        () => localStorage.getItem(TOKEN_KEY)
    );

    const [user, setUser] = useState(null);
    const [loading, setLoading] = useState(true);

    const refreshUser = useCallback(async (authToken) => {
        const currentToken =
            authToken || token;

        if (!currentToken) {
            setUser(null);
            return null;
        }

        try {
            const response =
                await getCurrentUser(currentToken);

            setUser(response.user);

            return response.user;
        } catch (error) {
            if (
                error.status === 401 ||
                error.status === 403
            ) {
                clearAuthToken();
                setToken(null);
                setUser(null);
            }

            throw error;
        }
    }, [token]);

    useEffect(() => {
        const initializeAuth = async () => {
            const storedToken =
                localStorage.getItem(TOKEN_KEY);

            if (!storedToken) {
                setLoading(false);
                return;
            }

            try {
                await refreshUser(storedToken);
            } catch {
                setUser(null);
            } finally {
                setLoading(false);
            }
        };

        initializeAuth();
    }, [refreshUser]);

    const login = async (loginData) => {
        const response =
            await loginUser(loginData);

        if (!response.token) {
            throw new Error(
                "Login response did not contain a token"
            );
        }

        localStorage.setItem(
            TOKEN_KEY,
            response.token
        );

        setToken(response.token);

        const currentUser =
            await refreshUser(response.token);

        return {
            ...response,
            user: currentUser,
        };
    };

    const logout = () => {
        clearAuthToken();
        setToken(null);
        setUser(null);
    };

    const value = {
        token,
        user,
        loading,
        isAuthenticated:
            Boolean(token && user),
        login,
        logout,
        refreshUser,
    };

    return (
        <AuthContext.Provider value={value}>
            {children}
        </AuthContext.Provider>
    );
};

export const useAuth = () => {
    const context = useContext(AuthContext);

    if (!context) {
        throw new Error(
            "useAuth must be used inside AuthProvider"
        );
    }

    return context;
};