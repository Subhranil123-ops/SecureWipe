const app = require("./app");
const connectDB = require("./config/db");

const PORT = Number(process.env.PORT) || 10000;

const startServer = async () => {
    try {
        await connectDB();

        app.listen(
            PORT,
            "0.0.0.0",
            () => {
                console.log(
                    `Server running on 0.0.0.0:${PORT}`
                );
            }
        );
    } catch (error) {
        console.error(
            "Server startup failed:",
            error
        );

        process.exit(1);
    }
};

startServer();