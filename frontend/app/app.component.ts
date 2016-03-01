import {Component, AfterViewInit, ViewChild, Input} from "angular2/core";
import {Router, RouteParams} from "angular2/router";
import {NgForm}    from 'angular2/common';
import {RouteConfig, ROUTER_DIRECTIVES} from "angular2/router";
import {HTTP_PROVIDERS} from 'angular2/http';

import {HomeComponent} from "./home.component";
import {SnippetDetailComponent} from "./snippet-detail.component";

import {Snippet, SnippetService} from "./snippet.service";
import {EditingData, EditingDataService} from "./editing-data.service";
import {EditorComponent} from "./editor.component";

@Component({
  selector: "my-app",
  template: `
    {{diagnostic}}
    <div class="panel panel-default">
      <div class="panel-body">
        <form (ngSubmit)="onSubmit()">
          <div class="form-group" id="code-field">
            <editor (onSubmit)="onSubmit()" [(value)]="editing.code" [mode]="editing.lang"></editor>
          </div>
          <div class="form-inline">
            <select class="form-control" [(ngModel)]="editing.lang" required>
              <option *ngFor="#p of langs" [value]="p">{{p}}</option>
            </select>
            <button class="btn btn-default" type="submit">Run</button>
          </div>
        </form>
      </div>
    </div>
    <router-outlet></router-outlet>
  `,
  directives: [ROUTER_DIRECTIVES, EditorComponent],
  providers: [HTTP_PROVIDERS, SnippetService, EditingDataService],
})
@RouteConfig([
  { path: "/",      name: "Home",           component: HomeComponent },
  { path: "/:sid",  name: "SnippetDetail",  component: SnippetDetailComponent },
])
export class AppComponent {
  langs = ["ruby", "php"];
  editing: EditingData;

  constructor(
    private _service: SnippetService,
    private _edit_service: EditingDataService,
    private _router: Router) {
    this.editing = _edit_service.getData(this.langs[0]);
  }

  onSubmit() {
    this._service.createSnippet(this.editing).subscribe(
      snip => this._router.navigate(["SnippetDetail", { sid: snip.id }]),
      error => console.log(error)
    );
  }

  ngAfterViewInit() {
    const codeMirrorBlock = document.querySelector("#code-field .CodeMirror");
    if (codeMirrorBlock === null) {
      const origContainer = <HTMLElement>document.querySelector("#code-field");
      const codeMirror = CodeMirror(origContainer, {
        mode: "ruby",
        lineNumbers: true,
        value: this.editing.code,
        extraKeys: {
          "Ctrl-Enter": cm => {
            this.editing.code = cm.getValue();
            this.onSubmit();
          }
        }
      });
      codeMirror.on("change", cm => this.editing.code = cm.getDoc().getValue());
    }
  }
}
