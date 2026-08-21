const argon2 = require("argon2");
const User = require("../models/User");
const AppError = require("../utils/AppError");
const { generateToken } = require("../utils/jwt");

const registerUser = async ({ name, email, password }) => {

    const existingUser = await User.findOne({ email });

    if (existingUser) {
        throw new AppError("User already exists", 409);
    }

    const passwordHash = await argon2.hash(password);

    const user = await User.create({
        name,
        email,
        passwordHash
    });

    return user;
};

const loginUser = async ({ email, password }) => {

    const user = await User
        .findOne({ email })
        .select("+passwordHash");

    if (!user) {
        throw new AppError("Invalid email or password", 401);
    }

    const valid = await argon2.verify(
        user.passwordHash,
        password
    );

    if (!valid) {
        throw new AppError("Invalid email or password", 401);
    }

    if (user.status !== "ACTIVE") {
        throw new AppError("Account is not active", 403);
    }

    user.lastLoginAt = new Date();

    await user.save();

    return generateToken(user);
};

module.exports = {
    registerUser,
    loginUser
};