import {Component, OnInit} from "angular2/core";
import {Router, RouteParams} from "angular2/router";
import {Snippet, SnippetService} from "./snippet.service";

@Component({
  template: `
    home
  `,
})
export class HomeComponent {
  constructor(
    private _router: Router,
    private _routeParams: RouteParams,
    private _service: SnippetService) { }
}
