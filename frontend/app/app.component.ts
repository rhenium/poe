import {Component} from "angular2/core";
import {Router, RouteParams} from "angular2/router";
import {RouteConfig, ROUTER_DIRECTIVES} from "angular2/router";
import {HTTP_PROVIDERS} from "angular2/http";
import {Title} from "angular2/platform/browser";

import {HomeComponent} from "./home.component";
import {SnippetDetailComponent} from "./snippet-detail.component";

import {Snippet, SnippetService} from "./snippet.service";
import {EditingData, EditingDataService} from "./editing-data.service";
import {EditorComponent} from "./editor.component";

@Component({
  selector: "my-app",
  template: `
    <nav class="navbar">
      <div class="brand">
        <a class="navbar-brand" [routerLink]="['Home']">Poe</a>
      </div>
      <!--<ul>
        <li><a href="/about">about</a></li>
      </ul>-->
    </nav>
    <div class="container">
      <div class="panel panel-default">
        <div class="panel-body">
          <form (ngSubmit)="onSubmit()">
            <div class="form-group" id="code-field">
              <editor (onSubmit)="onSubmit()" [(value)]="editing.code" [mode]="editing.lang"></editor>
            </div>
            <div class="form-inline">
              <select class="form-control" [(ngModel)]="editing.lang" required>
                <option *ngFor="#p of availableLangs()" [value]="p">{{p}}</option>
              </select>
              <button class="btn btn-default" type="submit">Run</button>
            </div>
          </form>
        </div>
      </div>
      <router-outlet></router-outlet>
    </div>
  `,
  directives: [ROUTER_DIRECTIVES, EditorComponent],
  providers: [SnippetService, EditingDataService, Title],
})
@RouteConfig([
  { path: "/",      name: "Home",           component: HomeComponent },
  { path: "/:sid",  name: "SnippetDetail",  component: SnippetDetailComponent },
])
export class AppComponent {
  editing: EditingData;

  constructor(
    private _service: SnippetService,
    private _router: Router,
    _edit_service: EditingDataService,
    _title: Title) {
    this.editing = _edit_service.get();
    _router.subscribe(path => {
      let title = "poe: online ruby environment";
      if (path !== "")
        title = "poe: " + path;
      _title.setTitle(title);
    });
  }

  onSubmit() {
    this._service.createSnippet(this.editing).subscribe(
      snip => this._router.navigate(["SnippetDetail", { sid: snip.id }]),
      error => console.log(error)
    );
  }

  // これなんとかならない？
  availableLangs() {
    return EditingData.availableLangs;
  }
}
