import "rxjs/Rx";

import {bootstrap} from "angular2/platform/browser"
import {enableProdMode, provide} from "angular2/core";
import {ROUTER_PROVIDERS, APP_BASE_HREF} from "angular2/router";
import {HTTP_PROVIDERS} from "angular2/http";

import {AppComponent} from "./app.component"

declare var ENV: any;
if (typeof ENV !== "undefined" && ENV === "production")
  enableProdMode();

bootstrap(AppComponent, [
  ROUTER_PROVIDERS,
  HTTP_PROVIDERS,
  provide(APP_BASE_HREF, { useValue: "/" })
]);
