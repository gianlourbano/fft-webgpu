const express = require("express");
const path = require("path");

const app = express();

app.use(express.static(path.resolve(__dirname, "..", "public")));

app.get("/", (req, res) => {
    res.sendFile(path.resolve(__dirname + "/index.html"));
});

app.listen(3000, () => {
    console.log("Server is running on port 3000");
});