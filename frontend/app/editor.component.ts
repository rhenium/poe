import {Component, OnInit, ElementRef, Input, Output, EventEmitter} from "angular2/core";
import {EditingData, EditingDataService} from "./editing-data.service";
import "brace";
import "brace/mode/php";
import "brace/mode/ruby";

@Component({
  selector: "editor",
  template: `
    <div style.height="300px"></div>
  `,
})
export class EditorComponent implements OnInit {
  private _mode: string;
  @Input() set mode(val: string) {
    this._mode = val;
    if (this.editor) {
      let session = this.editor.getSession();
      session.setMode("ace/mode/" + val);
    }
  }
  private _value: string;
  @Input() set value(val: string) {
    if (this._value !== val) {
      this._value = val;
      if (this.editor) {
        this.editor.setValue(val);
        this.editor.clearSelection();
      }
    }
  }
  @Output() valueChange = new EventEmitter();
  @Output() onSubmit = new EventEmitter();

  private editor: AceAjax.Editor;

  constructor(
    private elementRef: ElementRef) { }

  ngOnInit() {
    if (!this.editor) {
      const elm = this.elementRef.nativeElement;
      const div = elm.querySelector("div");
      this.editor = ace.edit(div);
      this.editor.setOptions({
        maxLines: Infinity,
        minLines: 10,
      });
      this.mode = this._mode;
      this.editor.commands.addCommand({
        name: "run",
        bindKey: { win: "Ctrl-Enter", mac: "Command-Enter" },
        exec: e => this.onSubmit.emit(this),
      });

      let changed = false;
      this.editor.addEventListener("input", () => {
        changed = true;
      }); // なにこれ
      this.editor.addEventListener("change", e => {
        if (!changed) return;
        changed = false;
        let s = this.editor.getValue();
        if (this._value !== s) {
          this._value = s;
          this.valueChange.emit(s);
        }
      });
    }
  }
}
