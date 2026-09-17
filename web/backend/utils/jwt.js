const jwt = require("jsonwebtoken");
const fs = require("fs");
const path = require("path");

const readKey = (
    envName,
    renderPath,
    localPath
) => {
    const envValue = process.env[envName];

    if (envValue) {
        return envValue.replace(/\\n/g, "\n");
    }

    if (fs.existsSync(renderPath)) {
        return fs.readFileSync(
            renderPath,
            "utf8"
        );
    }

    return fs.readFileSync(
        localPath,
        "utf8"
    );
};

const privateKey = readKey(
    "JWT_PRIVATE_KEY",
    "/etc/secrets/private.pem",
    path.join(__dirname, "../secrets/private.pem")
);

const publicKey = readKey(
    "JWT_PUBLIC_KEY",
    "/etc/secrets/public.pem",
    path.join(__dirname, "../secrets/public.pem")
);

const generateToken = (user) => {
    return jwt.sign(
        {
            sub: user._id.toString(),
            role: user.role
        },
        privateKey,
        {
            algorithm: "RS256",
            expiresIn: "15m",
            issuer: "securewipe-api",
            audience: "securewipe-client"
        }
    );
};

const verifyToken = (token) => {
    return jwt.verify(
        token,
        publicKey,
        {
            algorithms: ["RS256"],
            issuer: "securewipe-api",
            audience: "securewipe-client"
        }
    );
};

module.exports = {
    generateToken,
    verifyToken
};