const express = require("express");
const path = require("path");

const app = express();

const base = path.resolve(__dirname, "..", "build");

app.use(express.static(base));

app.get("/", (req, res) => {
    res.sendFile(path.resolve(base ,"app.html"));
});

app.listen(3000, () => {
    console.log("Server is running on port 3000");
});