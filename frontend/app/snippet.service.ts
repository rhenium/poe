import {Injectable} from "angular2/core";
import {Http, Response, Headers, RequestOptions, URLSearchParams} from "angular2/http";
import {Observable}     from "rxjs/Observable";
import {EditingData} from "./editing-data.service";

export class Compiler {
  constructor(
    public id: string) { }
}

export class Result {
  constructor(
    public compiler: Compiler,
    public result: number,
    public exit: number,
    public message: string,
    public output: any[],
    public _: any,
    public truncated: boolean) { }

  public static compareOutput(a: Result, b: Result) {
    return a.result === b.result &&
      a.exit === b.exit &&
      a.message === b.message &&
      a.output && b.output &&
      a.output.every((c, i) => b.output[i] && c[0] === b.output[i][0] && c[1] === b.output[i][1]) &&
      a.truncated === b.truncated;
  }
}

export class Snippet {
  constructor(
    public id: string,
    public lang: string,
    public code: string,
    public created: number,
    public results: Result[]) { }
}

@Injectable()
export class SnippetService {
  constructor(private http: Http) {}

  getSnippet(id: string) {
    return this.http.get("/api/snippet/" + id)
      .map(res => <Snippet>res.json())
      .catch(this.handleError);
  }

  createSnippet(edit: EditingData) {
    const data = {
      lang: edit.lang,
      code: edit.code
    };
    return this.http.post("/api/snippet/new", this.urlEncode(data))
      .map(res => <Snippet>res.json())
      .catch(this.handleError);
  }

  runSnippet(snip: Snippet, comp: Compiler) {
    const data = {
      sid: snip.id,
      cid: comp.id
    };
    return this.http.post("/api/snippet/run", this.urlEncode(data))
      .map(res => <Result>res.json())
      .catch(this.handleError);
  }

  private handleError (error: Response) {
    console.error(error);
    return Observable.throw(error.json().error || "Server error");
  }

  private urlEncode(obj: Object): string {
    let out = "";
    for (let key in obj) {
      const c = encodeURIComponent(key) + "=" + encodeURIComponent(obj[key]);
      if (out === "")
        out = c;
      else
        out += "&" + c;
    }
    return out;
  }
}
