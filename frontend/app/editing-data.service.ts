import {Injectable} from "angular2/core";
import {Http, Response} from "angular2/http";
import {Headers, RequestOptions, URLSearchParams} from "angular2/http";
import {Observable}     from "rxjs/Observable";

import {Snippet, SnippetService} from "./snippet.service";

export class EditingData {
  constructor(
    public code: string,
    public lang: string) { }
}

@Injectable()
export class EditingDataService {
  private data: EditingData;

  getData(lang: string) {
    if (!this.data) {
      this.data = new EditingData("", lang);
    }
    return this.data;
  }

  fromSnippet(snip: Snippet) {
    let data = this.getData(snip.lang);
    data.code = snip.code;
    data.lang = snip.lang;
  }
}
