import {Component, OnInit} from "angular2/core";
import {Router, RouteParams} from "angular2/router";
import {Snippet, SnippetService} from "./snippet.service";
import {EditingData, EditingDataService} from "./editing-data.service";

@Component({
  template: `
  `,
})
export class HomeComponent {
  constructor(
    private _router: Router,
    private _routeParams: RouteParams,
    private _service: SnippetService,
    private _edit_service: EditingDataService) {
      const msg = "# paste your code here";
      _edit_service.set("ruby", msg);
  }
}
