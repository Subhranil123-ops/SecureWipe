require("dotenv").config();

const express = require("express");
const cors = require("cors");

const app = express();

const allowedOrigins = (
    process.env.CORS_ORIGINS ||
    "http://localhost:5173",
    "https://securewipe-web.onrender.com"
)
    .split(",")
    .map((origin) => origin.trim())
    .filter(Boolean);

app.use(
    cors({
        origin(origin, callback) {
            if (!origin) {
                return callback(null, true);
            }

            if (allowedOrigins.includes(origin)) {
                return callback(null, true);
            }

            return callback(
                new Error("Origin not allowed by CORS")
            );
        },
        credentials: false
    })
);

app.use(express.json());

app.get("/health", (req, res) => {
    res.status(200).json({
        success: true,
        status: "ok",
        service: "securewipe-api"
    });
});

// Routes
const authRoute = require("./Routes/auth.routes");
const workstationRoute = require("./Routes/workstationCenterRoutes");
const userRoute = require("./Routes/users.routes");
const workstationManagementRoute = require("./Routes/workstation.routes");
const sanitizationRequestRoute = require("./Routes/sanitizationRequest.routes");
const sanitizationResultRoute = require("./Routes/sanitizationResult.routes");
const sanitizationCertificateRoute = require("./Routes/sanitizationCertificate.routes");
const forensicCaseRoute = require("./Routes/forensicCase.routes");

// Error middlewares
const notFound = require("./middlewares/notFound");
const errorHandler = require("./middlewares/errorHandler");

// Routing
app.use("/api/auth", authRoute);
app.use("/api/workstation-centers", workstationRoute);
app.use("/api/users", userRoute);
app.use("/api/workstations", workstationManagementRoute);

app.use(
    "/api/sanitization-requests",
    sanitizationRequestRoute
);

app.use(
    "/api/sanitization-results",
    sanitizationResultRoute
);

app.use(
    "/api/sanitization-certificates",
    sanitizationCertificateRoute
);

app.use("/api/forensics", forensicCaseRoute);

// Errors
app.use(notFound);
app.use(errorHandler);

module.exports = app;