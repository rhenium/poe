import {Component, OnInit, OnChanges, SimpleChange} from "angular2/core";
import {Router, RouteParams} from "angular2/router";
import {Snippet, Result, SnippetService} from "./snippet.service";
import {EditingData, EditingDataService} from "./editing-data.service";

@Component({
  template: `
    <div class="result-items-container" *ngIf="snippet">
      <div class="result-item panel"
          *ngFor="#r of snippet.results.slice().reverse(); #i = index"
          [ngClass]="{'panel-success': isSuccess(r), 'panel-failure': isFailure(r), 'panel-running': isRunning(r), 'result-item-collapsed': isHiddenIdx(i)}">
        <div class="panel-heading" (click)="toggleHiddenIdx(i)" [id]="'result-'+r.compiler.id">
          <span class="inline-left">{{r.compiler.id}}</span>
          <span class="inline-right" *ngIf="!isRunning(r)">{{r.elapsed}}ms</span>
        </div>
        <div class="panel-body">
          <pre><code [innerHTML]="formatted_output(r)"></code></pre>
        </div>
      </div>
    </div>
  `,
})
export class SnippetDetailComponent implements OnInit {
  private snippet: Snippet = null;
  private hidden_list: boolean[] = [];

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
        this.updateHiddenList();
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

  updateHiddenList() {
    this.hidden_list = [];
    let prev = null;
    this.snippet.results.slice().reverse().forEach((curr, i) => {
      if (prev)
        this.hidden_list.push(Result.compareOutput(prev, curr));
      prev = curr;
    });
    this.hidden_list.push(false);
  }

  isHiddenIdx(i: number) {
    return this.hidden_list[i];
  }

  toggleHiddenIdx(i: number) {
    this.hidden_list[i] = !this.hidden_list[i];
  }

  runRemaining() {
    this.snippet.results.forEach((r, i, a) => {
      if (r.result === null) {
        this._service.runSnippet(this.snippet, r.compiler).subscribe(
          res => {
            a[i] = res;
            this.updateHiddenList();
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
