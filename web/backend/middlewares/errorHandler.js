const errorHandler = (err, req, res, next) => {
    const isProduction = process.env.NODE_ENV === "production";

    // Default values for unexpected errors
    let statusCode = err.statusCode || 500;
    let message = err.message || "Internal Server Error";
    let code = err.code || "INTERNAL_SERVER_ERROR";

    // Never expose internal error details in production
    if (statusCode === 500 && isProduction) {
        message = "Internal Server Error";
        code = "INTERNAL_SERVER_ERROR";
    }

    // Server-side logging
    if (statusCode >= 500) {
        console.error({
            name: err.name,
            message: err.message,
            stack: err.stack,
            method: req.method,
            url: req.originalUrl,
            timestamp: new Date().toISOString()
        });
    }

    const response = {
        success: false,
        error: {
            code,
            message
        }
    };

    // Helpful during development only
    if (!isProduction) {
        response.error.stack = err.stack;
    }

    return res.status(statusCode).json(response);
};

module.exports = errorHandler;