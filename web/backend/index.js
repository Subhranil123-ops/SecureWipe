require("dotenv").config();

const express = require("express");
const connectDB = require("./config/db");
const cors = require("cors");
const app = express();

app.use(cors({
    origin: "http://localhost:5173",
    credentials: true
}));

app.use(express.json());

const PORT = process.env.PORT || 5000;

// Routes
const authRoute = require("./Routes/auth.routes");
const workstationRoute = require("./Routes/workstationCenterRoutes");
const userRoute = require("./Routes/users.routes");

// error middlewares
const notFound = require("./middlewares/notFound");
const errorHandler = require("./middlewares/errorHandler");

// routing
app.use("/api/auth", authRoute);
app.use("/api/workstation-centers", workstationRoute);
app.use("/api/users", userRoute);

//adding errors
app.use(notFound);
app.use(errorHandler);

//server starting
const startServer = async () => {
    try {
        await connectDB();

        app.listen(PORT, () => {
            console.log(`Server running on port ${PORT}`);
        });
    } catch (error) {
        console.error("Server startup failed");
        process.exit(1);
    }
};

startServer();