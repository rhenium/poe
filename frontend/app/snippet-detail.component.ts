import {Component, OnInit, OnChanges, SimpleChange} from "angular2/core";
import {Router, RouteParams} from "angular2/router";
import {Snippet, Result, SnippetService} from "./snippet.service";
import {EditingData, EditingDataService} from "./editing-data.service";

@Component({
  template: `
    <div class="result-items-container">
      <div class="result-item panel"
          *ngFor="#r of snippet.results.slice().reverse(); #i = index"
          [ngClass]="{'panel-success': isSuccess(r), 'panel-failure': isFailure(r), 'panel-running': isRunning(r), 'result-item-collapsed': isHiddenIdx(i)}">
        <div class="panel-heading" (click)="toggleHiddenIdx(i)">{{r.compiler.id}} ({{r.compiler.version}})</div>
        <div class="panel-body container">
          <pre><code [innerHTML]="formatted_output(r)"></code></pre>
        </div>
      </div>
    </div>
  `,
})
export class SnippetDetailComponent implements OnInit {
  snippet = new Snippet("", "", "", -1, []);
  hidden_list = [];

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
    let str = r.output.reduce((str, pair) => {
      let fd = pair[0];
      let escaped = this.escapeHTML(pair[1]);
      return str + "<span class=\"result-fd-" + fd + "\">" + escaped + "</span>";
    }, "");

    if (r.truncated) {
      str += "<span class=\"result-info\">[truncated]</span>";
    } else if (r.output.length === 0 || !r.output[r.output.length - 1][1].endsWith("\n")) {
      str += "<span class=\"result-missing-newline\">%\n</span>";
    }

    if (r.result === 0) { // Success
      if (r.exit !== 0) {
        str += "<span class=\"result-exit\">Process exited with status " + r.exit + "</span>";
      }
    } else if (r.result === 1) { // Signaled
      if (r.message.length > 0) {
        str += "<span class=\"result-exit\">" + this.escapeHTML(r.message) + "</span>";
      }
    } else if (r.result === 2) {
      str += "<span class=\"result-exit\">" + this.escapeHTML("Time limit exceeded") + "</span>";
    }

    r._ = str;
    return str;
  }

  // これも
  result_text(r: Result): string {
    return ["Success", "Signaled", "Timed out"][r.result];
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
