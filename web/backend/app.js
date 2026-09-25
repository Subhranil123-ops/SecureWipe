require("dotenv").config();

const express = require("express");
const cors = require("cors");

const app = express();

// --------------------------------------------------
// CORS
// --------------------------------------------------

const allowedOrigins = (
    process.env.CORS_ORIGINS ||
    "http://localhost:5173,https://securewipe-web.onrender.com"
)
    .split(",")
    .map((origin) => origin.trim())
    .filter(Boolean);

app.use(
    cors({
        origin(origin, callback) {
            // Requests without an Origin header
            // (for example curl/server-to-server)
            // are allowed.
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

// --------------------------------------------------
// BODY PARSING
// --------------------------------------------------
//
// The audit JSONL file is sent inside a JSON request.
// Keep the limit large enough for real audit evidence.
//
// --------------------------------------------------

app.use(
    express.json({
        limit: "10mb"
    })
);

// --------------------------------------------------
// HEALTH
// --------------------------------------------------

app.get(
    "/health",
    (req, res) => {
        res.status(200).json({
            success: true,
            status: "ok",
            service: "securewipe-api"
        });
    }
);

// --------------------------------------------------
// ROUTES
// --------------------------------------------------

const authRoute =
    require("./Routes/auth.routes");

const workstationRoute =
    require("./Routes/workstationCenterRoutes");

const userRoute =
    require("./Routes/users.routes");

const workstationManagementRoute =
    require("./Routes/workstation.routes");

const sanitizationRequestRoute =
    require("./Routes/sanitizationRequest.routes");

const sanitizationResultRoute =
    require("./Routes/sanitizationResult.routes");

const sanitizationCertificateRoute =
    require("./Routes/sanitizationCertificate.routes");

const sanitizationAuditChainRoute =
    require("./Routes/sanitizationAuditChain.routes");

const forensicCaseRoute =
    require("./Routes/forensicCase.routes");

// --------------------------------------------------
// ROUTING
// --------------------------------------------------

app.use(
    "/api/auth",
    authRoute
);

app.use(
    "/api/workstation-centers",
    workstationRoute
);

app.use(
    "/api/users",
    userRoute
);

app.use(
    "/api/workstations",
    workstationManagementRoute
);

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

app.use(
    "/api/sanitization-audit",
    sanitizationAuditChainRoute
);

app.use(
    "/api/forensics",
    forensicCaseRoute
);

// --------------------------------------------------
// ERROR MIDDLEWARE
// --------------------------------------------------

const notFound =
    require("./middlewares/notFound");

const errorHandler =
    require("./middlewares/errorHandler");

app.use(
    notFound
);

app.use(
    errorHandler
);

module.exports = app;