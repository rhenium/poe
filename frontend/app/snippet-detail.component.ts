import {Component, OnInit} from "angular2/core";
import {Router, RouteParams} from "angular2/router";
import {Snippet, Result, SnippetService} from "./snippet.service";
import {EditingData, EditingDataService} from "./editing-data.service";

@Component({
  template: `
    <div class="result-item panel panel-default" *ngFor="#r of snippet.results"><!-- panel-{success|danger|default} -->
      <div class="panel-heading">{{r.compiler.id}} ({{r.compiler.version}})</div>
      <div class="panel-body">
        <pre *ngIf="r.result !== null"><code [innerHTML]="formatted_output(r)"></code></pre>
        <pre *ngIf="!r.result === null"><code>Running....</code></pre>
      </div>
    </div>
  `,
})
export class SnippetDetailComponent implements OnInit {
  snippet = new Snippet("", "", "", -1, []);

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
        this._edit_service.fromSnippet(snip);
        this.runRemaining();
      },
      error => console.log(error)
    );
  }

  // Result に移動したいんだけどどうすればいいんだろ
  formatted_output(r: Result): string {
    let str = r.output.reduce((str, pair) => {
      let fd = pair[0];
      let escaped = this.escapeHTML(pair[1]);
      return str + "<span class=\"result-fd-" + fd + "\">" + escaped + "</span>";
    }, "");

    if (r.truncated) {
      str += "<span class=\"result-info\">[truncated]</span>"
    } else if (!r.output[r.output.length - 1][1].endsWith("\n")) {
      str += "<span class=\"result-info\">%</span>"
    }

    if (r.result === 0) { // Success
      if (r.exit !== 0) {
        str += "\n<span class=\"result-exit\">Process exited with status " + r.exit + "</span>";
      }
    }

    return str;
  }

  // これも
  result_text(r: Result): string {
    return ["Success", "Signaled", "Timed out"][r.result];
  }

  runRemaining() {
    this.snippet.results.forEach((r, i, a) => {
      if (r.result === null) {
        this._service.runSnippet(this.snippet, r.compiler).subscribe(
          res => a[i] = res,
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
