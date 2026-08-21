import { useEffect, useState } from "react";
import {
    Link,
    useLocation,
    useNavigate,
} from "react-router-dom";

import toast from "react-hot-toast";

import { useAuth } from "../../context/AuthContext";

function Login() {
    const [email, setEmail] = useState("");
    const [password, setPassword] = useState("");
    const [loading, setLoading] = useState(false);

    const {
        login,
        isAuthenticated,
        user,
    } = useAuth();

    const navigate = useNavigate();
    const location = useLocation();

    useEffect(() => {
        if (!isAuthenticated || !user) {
            return;
        }

        if (user.role === "ADMIN") {
            navigate("/admin/dashboard", {
                replace: true,
            });
        } else if (
            user.role === "WORKSTATION_HEAD"
        ) {
            navigate(
                "/workstation-head/dashboard",
                {
                    replace: true,
                }
            );
        } else if (
            user.role === "WORKSTATION_EMPLOYEE"
        ) {
            navigate(
                "/workstation-employee/dashboard",
                {
                    replace: true,
                }
            );
        } else if (
            user.role === "CUSTOMER"
        ) {
            navigate("/customer/dashboard", {
                replace: true,
            });
        }
    }, [
        isAuthenticated,
        user,
        navigate,
    ]);

    const handleSubmit = async (event) => {
        event.preventDefault();

        setLoading(true);

        try {
            await login({
                email,
                password,
            });

            toast.success("Login successful");
        } catch (error) {
            toast.error(
                error.message ||
                "Unable to login"
            );
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="flex min-h-screen items-center justify-center bg-gradient-to-br from-slate-50 to-indigo-50 px-4 py-8">
            <div className="w-full max-w-md rounded-xl bg-white p-8 shadow-md">
                <div className="mb-7 text-center">
                    <h1 className="text-3xl font-bold text-slate-900">
                        Login
                    </h1>

                    <p className="mt-2 text-sm text-slate-500">
                        Sign in to your SecureWipe account
                    </p>
                </div>

                <form
                    onSubmit={handleSubmit}
                    className="space-y-5"
                >
                    <div className="space-y-2">
                        <label
                            htmlFor="email"
                            className="block text-sm font-medium text-slate-700"
                        >
                            Email
                        </label>

                        <input
                            id="email"
                            type="email"
                            value={email}
                            onChange={(event) =>
                                setEmail(
                                    event.target.value
                                )
                            }
                            placeholder="Enter your email"
                            required
                            className="w-full rounded-lg border border-slate-300 px-4 py-2.5 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100"
                        />
                    </div>

                    <div className="space-y-2">
                        <label
                            htmlFor="password"
                            className="block text-sm font-medium text-slate-700"
                        >
                            Password
                        </label>

                        <input
                            id="password"
                            type="password"
                            value={password}
                            onChange={(event) =>
                                setPassword(
                                    event.target.value
                                )
                            }
                            placeholder="Enter your password"
                            required
                            className="w-full rounded-lg border border-slate-300 px-4 py-2.5 text-sm outline-none focus:border-indigo-500 focus:ring-2 focus:ring-indigo-100"
                        />
                    </div>

                    <button
                        type="submit"
                        disabled={loading}
                        className="w-full rounded-lg bg-indigo-600 px-4 py-2.5 text-sm font-semibold text-white hover:bg-indigo-700 disabled:cursor-not-allowed disabled:opacity-60"
                    >
                        {loading
                            ? "Logging in..."
                            : "Login"}
                    </button>
                </form>

                <p className="mt-6 text-center text-sm text-slate-500">
                    Don't have an account?{" "}

                    <Link
                        to="/register"
                        state={{
                            from: location.pathname,
                        }}
                        className="font-semibold text-indigo-600 hover:text-indigo-700"
                    >
                        Register
                    </Link>
                </p>
            </div>
        </div>
    );
}

export default Login;