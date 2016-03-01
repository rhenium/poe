const historyFallback = require("connect-history-api-fallback");
const log = require("connect-logger");
const proxy = require("proxy-middleware");
const url = require("url");

var proxyOpts = url.parse("http://localhost:3000/api");
proxyOpts.route = "/api";

module.exports = {
  injectChanges: false,
  ui: { port: 3002 },
  port: 3001,
  files: ["./**/*.{html,htm,css,js}"],
  server: {
    baseDir: "./",
    middleware: [
      proxy(proxyOpts),
      log({format: "%date %status %method %url"}),
      historyFallback({"index": "/index.html"})
    ]
  }
};
