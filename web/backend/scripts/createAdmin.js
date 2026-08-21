require("dotenv").config();
const mongoose = require("mongoose");
const argon2 = require("argon2");

const User = require("../models/User");

const createAdmin = async () => {
    try {
        await mongoose.connect(process.env.MONGO_URI);

        const existingAdmin = await User.findOne({ role: "ADMIN" });

        if (existingAdmin) {
            console.log("Admin already exists");
            return;
        }

        const passwordHash = await argon2.hash(process.env.ADMIN_PASSWORD);

        await User.create({
            name: process.env.ADMIN_NAME,
            email: process.env.ADMIN_EMAIL,
            passwordHash,
            role: "ADMIN",
            status: "ACTIVE",
            emailVerified: true
        });

        console.log("Admin created successfully");

    } catch (err) {
        console.log("Failed to create the Admin", err.message);
    } finally {
        await mongoose.disconnect();
    }
}

createAdmin();