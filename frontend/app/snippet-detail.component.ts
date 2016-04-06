import {Component, OnInit, OnChanges, SimpleChange} from "angular2/core";
import {Router, RouteParams} from "angular2/router";
import {Snippet, Result, SnippetService} from "./snippet.service";
import {EditingData, EditingDataService} from "./editing-data.service";

@Component({
  template: `
    <div class="result-items-container" *ngIf="snippet">
      <div class="result-item panel"
          *ngFor="#group of result_classes; #i = index"
          [ngClass]="{'panel-success': isSuccess(group.results[0]), 'panel-failure': isFailure(group.results[0]), 'panel-running': isRunning(group.results[0])}">
        <div class="panel-heading" [id]="'result-type-'+i">
          <div *ngFor="#r of group.results" class="result-compiler-tab-item" (click)="group.current = r" [ngClass]="{'active': group.current === r}">
            {{r.compiler.id}} ({{r.elapsed_ms}}ms)
          </div>
        </div>
        <div class="panel-body" *ngIf="group.current">
          <pre><code [innerHTML]="formatted_output(group.current)"></code></pre>
        </div>
      </div>
    </div>
  `,
})
export class SnippetDetailComponent implements OnInit {
  private snippet: Snippet = null;
  private result_classes: any[] = [];

  constructor(
    private _router: Router,
    private _routeParams: RouteParams,
    private _service: SnippetService,
    private _edit_service: EditingDataService) { }

  ngOnInit() {
    let sid = this._routeParams.get("sid");
    this._service.getSnippet(sid).subscribe(
      snip => {
        this.snippet = snip;
        this.classifyResults();
        this._edit_service.fromSnippet(snip);
        this.runRemaining();
      },
      error => console.log(error)
    );
  }

  // Result に移動したいんだけどどうすればいいんだろ
  formatted_output(r: Result): string {
    if (this.isRunning(r)) return "Running...";
    if (r._) return r._; // うーーーーーーーーーーん💩

    let str = "";
    str += "<span class=\"result-info\">% " + this.escapeHTML(r.compiler.version_command) + "</span>\n";
    str += "<span class=\"result-info\">" + this.escapeHTML(r.compiler.version) + "</span>\n";
    // TODO: /tmp/prog どうしよ
    str += "<span class=\"result-info\">% " + this.escapeHTML(r.compiler.commandline.join(" ").replace("{}", "/tmp/prog")) + "</span>\n";

    str += r.output.reduce((str, pair) => {
      let fd = pair[0];
      let escaped = this.escapeHTML(pair[1]);
      return str + "<span class=\"result-fd-" + fd + "\">" + escaped + "</span>";
    }, "");

    if (r.truncated)
      str += "<span class=\"result-info\">[truncated]</span>\n";
    else if (r.output.length === 0 || !r.output[r.output.length - 1][1].endsWith("\n"))
      str += "<span class=\"result-missing-newline\">%</span>\n";

    switch (r.result) {
      case 0: // Success
        if (r.exit !== 0)
          str += "<span class=\"result-exit\">Process exited with status " + r.exit + "</span>";
        break;
      case 1: // Signaled
        if (r.message.length > 0)
          str += "<span class=\"result-exit\">" + this.escapeHTML(r.message) + "</span>";
        break;
      case 2: // Timed out
        // 5s どうしよ
        str += "<span class=\"result-exit\">Time limit exceeded (5s)</span>";
        break;
    }

    r._ = str;
    return str;
  }

  isSuccess(r: Result) {
    return r.result === 0 && r.exit === 0;
  }

  isFailure(r: Result) {
    return !this.isRunning(r) && !this.isSuccess(r);
  }

  isRunning(r: Result) {
    return r.result === null;
  }

  classifyResults() {
    var classes = [];
    this.snippet.results.slice().reverse().forEach(curr => {
      const found = classes.some(group =>
        Result.compareOutput(curr, group.results[0]) &&
          group.results.push(curr));
      if (!found)
        classes.push({ current: curr, results: [curr] });
    });
    this.result_classes = classes;
  }

  runRemaining() {
    this.snippet.results.forEach((r, i, a) => {
      if (r.result === null) {
        this._service.runSnippet(this.snippet, r.compiler).subscribe(
          res => {
            a[i] = res;
            this.classifyResults();
          },
          err => console.log(err)
        );
      }
    });
  }

  private escapeHTML(str: string): string {
    const map = {
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': '&quot;',
    };
    return str.replace(/[&<>"]/g, s => map[s]);
  }
}
