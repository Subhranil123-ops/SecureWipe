const jwt = require("jsonwebtoken");
const fs = require("fs");
const path = require("path");

const privateKey = fs.readFileSync(
    path.join(__dirname, "../secrets/private.pem"),
    "utf8"
);

const publicKey = fs.readFileSync(
    path.join(__dirname, "../secrets/public.pem"),
    "utf8"
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