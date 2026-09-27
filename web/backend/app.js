require("dotenv").config();

const express =
    require("express");

const cors =
    require("cors");

const app =
    express();


// --------------------------------------------------
// CORS
// --------------------------------------------------

/*
 * LOCAL DEVELOPMENT / HOPPSCOTCH TESTING
 *
 * We intentionally allow every Origin while testing
 * the local API from Hoppscotch.
 *
 * credentials = false, so browser credentials such as
 * cookies are not being enabled by this configuration.
 *
 * IMPORTANT:
 * Before production deployment, replace this with the
 * restricted origin whitelist.
 */

const isProduction =
    process.env.NODE_ENV === "production";


const allowedOrigins =
    (
        process.env.CORS_ORIGINS ||
        "http://localhost:5173,https://securewipe-web.onrender.com"
    )
        .split(",")
        .map(
            origin =>
                origin.trim()
        )
        .filter(
            Boolean
        );


app.use(
    cors(
        {

            origin(
                origin,
                callback
            ) {

                /*
                 * Non-browser clients such as:
                 * - native desktop application
                 * - curl
                 * - Postman
                 * - Hoppscotch
                 * - CLI clients
                 *
                 * may not send an Origin header.
                 */

                if (
                    !origin
                ) {

                    return callback(
                        null,
                        true
                    );
                }


                /*
                 * LOCAL TESTING
                 *
                 * Hoppscotch may use an Origin different
                 * from localhost:5173.
                 *
                 * Since this backend is currently being
                 * tested locally, allow it.
                 */

                if (
                    !isProduction
                ) {

                    console.log(
                        `[CORS] Local origin allowed: ${origin}`
                    );

                    return callback(
                        null,
                        true
                    );
                }


                /*
                 * PRODUCTION
                 *
                 * Only explicitly configured origins
                 * are allowed.
                 */

                if (
                    allowedOrigins.includes(
                        origin
                    )
                ) {

                    return callback(
                        null,
                        true
                    );
                }


                console.error(
                    `[CORS] Origin rejected: ${origin}`
                );

                return callback(
                    new Error(
                        "Origin not allowed by CORS"
                    )
                );
            },


            credentials:
                false
        }
    )
);


// --------------------------------------------------
// BODY PARSING
// --------------------------------------------------

app.use(
    express.json(
        {
            limit:
                "40mb"
        }
    )
);


// --------------------------------------------------
// HEALTH
// --------------------------------------------------

app.get(
    "/health",
    (
        req,
        res
    ) => {

        res.status(
            200
        ).json({

            success:
                true,

            status:
                "ok",

            service:
                "securewipe-api"
        });
    }
);


// --------------------------------------------------
// ROUTES
// --------------------------------------------------

const authRoute =
    require(
        "./Routes/auth.routes"
    );


const workstationRoute =
    require(
        "./Routes/workstationCenterRoutes"
    );


const userRoute =
    require(
        "./Routes/users.routes"
    );


const workstationManagementRoute =
    require(
        "./Routes/workstation.routes"
    );


const sanitizationRequestRoute =
    require(
        "./Routes/sanitizationRequest.routes"
    );


const sanitizationResultRoute =
    require(
        "./Routes/sanitizationResult.routes"
    );


const sanitizationCertificateRoute =
    require(
        "./Routes/sanitizationCertificate.routes"
    );


const sanitizationAuditChainRoute =
    require(
        "./Routes/sanitizationAuditChain.routes"
    );


const forensicCaseRoute =
    require(
        "./Routes/forensicCase.routes"
    );


const certificateRegistryRoute =
    require(
        "./Routes/certificateRegistry.routes"
    );


/*
 * LIVE OPERATION PROGRESS
 *
 * This route is the bridge between:
 *
 *     CLI / C++ engine
 *              ↓
 *       MongoDB live state
 *              ↓
 *          Website
 *
 * The desktop GUI is NOT involved in this path.
 */

const liveProgressRoute =
    require(
        "./Routes/liveProgress.routes"
    );


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


/*
 * LIVE OPERATION PROGRESS
 *
 * Examples:
 *
 * PATCH
 * /api/live-progress/sanitization/REQ-0011
 *
 * GET
 * /api/live-progress/sanitization/REQ-0011
 *
 * PATCH
 * /api/live-progress/forensics/FR-0001
 *
 * GET
 * /api/live-progress/forensics/FR-0001
 */

app.use(
    "/api/live-progress",
    liveProgressRoute
);


/*
 * CENTRAL CERTIFICATE REGISTRY
 */

app.use(
    "/api/certificates",
    certificateRegistryRoute
);


// --------------------------------------------------
// ERROR MIDDLEWARE
// --------------------------------------------------

const notFound =
    require(
        "./middlewares/notFound"
    );


const errorHandler =
    require(
        "./middlewares/errorHandler"
    );


app.use(
    notFound
);


app.use(
    errorHandler
);


module.exports =
    app;