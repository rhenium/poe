import {Injectable} from "angular2/core";
import {Http, Response, Headers, RequestOptions, URLSearchParams} from "angular2/http";
import {Observable}     from "rxjs/Observable";
import {EditingData} from "./editing-data.service";

export class Compiler {
  private static _pt = (<any>new Compiler()).__proto__;
  public id: string;
  public lang: string;
  public version: string;
  public version_command: string;
  public commandline: string[];

  // これどうしたらいいんだろ
  public static fromJSON(obj: any): Compiler {
    obj.__proto__ = Compiler._pt;
    return obj;
  }

  public abbrev() {
    if (this.id.startsWith(this.lang + "-"))
      return this.id.substr(this.lang.length + 1);
    else
      return this.id;
  }
}

export class Result {
  private static _pt = (<any>new Result()).__proto__;
  public compiler: Compiler;
  public result: number;
  public exit: number;
  public elapsed_ms: number;
  public message: string;
  public output: any[];
  public _: string;
  public truncated: boolean;

  public static fromJSON(obj: any): Result {
    obj.__proto__ = Result._pt;
    Compiler.fromJSON(obj.compiler);
    return obj;
  }

  public isSameResult(b: Result) {
    return this.result === b.result &&
      this.exit === b.exit &&
      this.message === b.message &&
      this.output && b.output &&
      this.output.every((c, i) => b.output[i] && c[0] === b.output[i][0] && c[1] === b.output[i][1]) &&
      this.truncated === b.truncated;
  }

  public isRunning() {
    return this.result === null;
  }

  public isSuccess() {
    return this.result === 0 && this.exit === 0;
  }

  public isFailure() {
    return !this.isSuccess() && !this.isRunning();
  }
}

export class Snippet {
  private static _pt = (<any>new Snippet()).__proto__;
  public id: string;
  public lang: string;
  public code: string;
  public created: number;
  public results: Result[];

  public static fromJSON(obj: any): Snippet {
    obj.__proto__ = Snippet._pt;
    obj.results.forEach(Result.fromJSON);
    return obj;
  }
}

@Injectable()
export class SnippetService {
  constructor(private http: Http) {}

  getSnippet(id: string) {
    return this.http.get("/api/snippet/" + id)
      .map(res => Snippet.fromJSON(res.json()))
      .catch(this.handleError);
  }

  createSnippet(edit: EditingData) {
    const data = {
      lang: edit.lang,
      code: edit.code
    };
    return this.http.post("/api/snippet/new", this.urlEncode(data))
      .map(res => Snippet.fromJSON(res.json()))
      .catch(this.handleError);
  }

  runSnippet(snip: Snippet, comp: Compiler) {
    const data = {
      sid: snip.id,
      cid: comp.id
    };
    return this.http.post("/api/snippet/run", this.urlEncode(data))
      .map(res => Result.fromJSON(res.json()))
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
