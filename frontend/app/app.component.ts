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
    <nav class="navbar container">
      <div class="brand">
        <a class="navbar-brand" [routerLink]="['Home']">poe</a>
        <span class="navbar-text">{{poe_description}}</span>
      </div>
    </nav>
    <div class="container">
      <div id="editor" class="panel">
        <div class="panel-body">
          <form (ngSubmit)="onSubmit()">
            <div class="form-group" id="editor-editor">
              <editor (onSubmit)="onSubmit()" [(value)]="editing.code" [mode]="editing.lang"></editor>
            </div>
            <div class="form-group container" id="editor-options">
              <select class="button dropdown-button" [(ngModel)]="editing.lang" required>
                <option *ngFor="#p of availableLangs()" [value]="p">{{p}}</option>
              </select>
              <button class="button default-button" type="submit">{{editing.lang}} /tmp/prog</button>
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
  private editing: EditingData;
  private poe_description = "eval code in 15+ Ruby interpreters";

  constructor(
    private _service: SnippetService,
    private _router: Router,
    _edit_service: EditingDataService,
    private _title: Title) {
    this.editing = _edit_service.get();
    this.editing.onChange(e => this._title.setTitle(this.generateTitle()));
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

  generateHeader() {
    const summary = this.editing.summarize();
    if (summary === "")
      return this.poe_description;
    else
      return summary;
  }

  generateTitle() {
    return this.generateHeader() + " - poe";
  }
}
