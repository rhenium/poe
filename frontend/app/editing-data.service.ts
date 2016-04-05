import {Injectable, EventEmitter} from "angular2/core";
import {Http, Response} from "angular2/http";
import {Headers, RequestOptions, URLSearchParams} from "angular2/http";
import {Observable}     from "rxjs/Observable";

import {Snippet, SnippetService} from "./snippet.service";

export class EditingData {
  public static availableLangs = ["ruby", "php"];
  private change = new EventEmitter(true);

  private _code: string = "";
  get code() { return this._code; }
  set code(val: string) {
    this._code = val;
    this.change.emit(this);
  }

  private _lang: string = EditingData.availableLangs[0];
  get lang() { return this._lang; }
  set lang(val: string) {
    this._lang = val;
    this.change.emit(this);
  }

  constructor() {
  }

  summarize() {
    let head = this.code.substring(0, 50)

    switch (this.lang) {
      case "ruby":
        head = head.replace(/^#+ */mg, "");
        let firstLine = head.substr(0, head.indexOf("\n")).trim();
        if (firstLine.length > 3)
          head = firstLine;
        else
          head = head.replace("\n", "  ").replace(/\s{3,}/, "  ");
        break;
    }

    head = head.trim();
    if (head.length >= 40)
      return head.substring(0, 39) + "…";
    else
      return head;
  }

  onChange(cb) {
    this.change.subscribe(cb);
  }
}

@Injectable()
export class EditingDataService {
  private data: EditingData = new EditingData();

  get() {
    return this.data;
  }

  set(lang: string, code: string) {
    this.data.lang = lang;
    this.data.code = code;
  }

  fromSnippet(snip: Snippet) {
    this.data.code = snip.code;
    this.data.lang = snip.lang;
  }
}
