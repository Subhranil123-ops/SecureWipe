require("dotenv").config();

const express = require("express");

const cors = require("cors");
const app = express();

app.use(cors({
    origin: "http://localhost:5173",
    credentials: true
}));

app.use(express.json());


// Routes
const authRoute = require("./Routes/auth.routes");
const workstationRoute = require("./Routes/workstationCenterRoutes");
const userRoute = require("./Routes/users.routes");
const workstationManagementRoute = require("./Routes/workstation.routes");
const sanitizationRequestRoute = require("./Routes/sanitizationRequest.routes");
const sanitizationResultRoute = require("./Routes/sanitizationResult.routes");
const sanitizationCertificateRoute = require("./Routes/sanitizationCertificate.routes");
const forensicCaseRoute = require("./Routes/forensicCase.routes");


// error middlewares
const notFound = require("./middlewares/notFound");
const errorHandler = require("./middlewares/errorHandler");

// routing
app.use("/api/auth", authRoute);
app.use("/api/workstation-centers", workstationRoute);
app.use("/api/users", userRoute);
app.use("/api/workstations", workstationManagementRoute);
app.use("/api/sanitization-requests", sanitizationRequestRoute);
app.use(
    "/api/sanitization-certificates",
    sanitizationCertificateRoute
);
app.use("/api/forensics", forensicCaseRoute);

//adding errors
app.use(notFound);
app.use(errorHandler);

module.exports = app;