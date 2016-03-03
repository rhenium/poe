import {Injectable} from "angular2/core";
import {Http, Response} from "angular2/http";
import {Headers, RequestOptions, URLSearchParams} from "angular2/http";
import {Observable}     from "rxjs/Observable";

import {Snippet, SnippetService} from "./snippet.service";

export class EditingData {
  public static availableLangs = ["ruby", "php"];

  constructor(
    public code: string,
    public lang: string) { }
}

@Injectable()
export class EditingDataService {
  private data: EditingData = new EditingData("", "ruby");

  get() {
    return this.data;
  }

  reset() {
    this.data.code = "";
    this.data.lang = EditingData.availableLangs[0];
  }

  fromSnippet(snip: Snippet) {
    this.data.code = snip.code;
    this.data.lang = snip.lang;
  }
}
