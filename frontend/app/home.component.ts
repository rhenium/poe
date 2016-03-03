import {Component, OnInit} from "angular2/core";
import {Router, RouteParams} from "angular2/router";
import {Snippet, SnippetService} from "./snippet.service";
import {EditingData, EditingDataService} from "./editing-data.service";

@Component({
  template: `
    <h1>ぽえってなに</h1>
    <p>同じコードを複数のバージョンの処理系で同時に実行できるオンラインコンパイラみたいなやつ</p>
    <h1>環境</h1>
    <ul>
    <li>OS: GNU/Linux x86_64</li>
    <li>メモリ制限: 128MB</li>
    <li>時間制限: 5 秒</li>
    </ul>
  `,
})
export class HomeComponent {
  constructor(
    private _router: Router,
    private _routeParams: RouteParams,
    private _service: SnippetService,
    private _edit_service: EditingDataService) {
      _edit_service.reset();
  }
}
