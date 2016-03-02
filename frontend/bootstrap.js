var xcb = function() {
  System.config({
    packages: {
      app: {
        format: "register",
        defaultExtension: "js"
      }
    },
    defaultJSExtensions: true,
    paths: {
      "*": "./node_modules/*",
      "app/*": "./app/*",
    },
    bundles: {
      "angular2/bundles/angular2.dev": ["angular2/core", "angular2/platform/browser", "angular2/common"],
      "angular2/bundles/http.dev": ["angular2/http"],
      "angular2/bundles/router.dev": ["angular2/router"],
      "rxjs/bundles/Rx": ["rxjs/*"],
    },
    packageConfigPaths: ["./node_modules/*/package.json"],
  });
}

if (typeof module === "undefined") {
  var cb = function() {
    xcb();
    System.import("app/main").then(null, console.error.bind(console));
  }

  var tag = document.createElement("script");
  tag.setAttribute("src", "node_modules/systemjs/dist/system.src.js");
  tag.onload = cb;
  document.head.appendChild(tag);
} else {
  xcb();
}
