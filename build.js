// Vercel build: copies the dashboard into dist/ and writes firebase-config.js
// from environment variables, so the Firebase config never lives in the repo.
const fs = require("fs");
const path = require("path");

const required = [
  "FIREBASE_API_KEY",
  "FIREBASE_AUTH_DOMAIN",
  "FIREBASE_DATABASE_URL",
  "FIREBASE_PROJECT_ID",
];
const missing = required.filter((name) => !process.env[name]);
if (missing.length) {
  console.error("Missing environment variables: " + missing.join(", "));
  process.exit(1);
}

const config = {
  apiKey: process.env.FIREBASE_API_KEY,
  authDomain: process.env.FIREBASE_AUTH_DOMAIN,
  databaseURL: process.env.FIREBASE_DATABASE_URL,
  projectId: process.env.FIREBASE_PROJECT_ID,
};

const out = path.join(__dirname, "dist");
fs.rmSync(out, { recursive: true, force: true });
fs.mkdirSync(out);
fs.copyFileSync(path.join(__dirname, "Dashboard.html"), path.join(out, "index.html"));
fs.writeFileSync(
  path.join(out, "firebase-config.js"),
  "window.firebaseConfig = " + JSON.stringify(config, null, 2) + ";\n"
);
console.log("Built dist/ (index.html + firebase-config.js)");
