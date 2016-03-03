import {Component, OnInit, ElementRef, Input, Output, EventEmitter} from "angular2/core";
import {EditingData, EditingDataService} from "./editing-data.service";
import CodeMirror from "codemirror";
import "codemirror/mode/ruby/ruby";
import "codemirror/mode/php/php";
import "codemirror/mode/htmlmixed/htmlmixed";
import "codemirror/mode/xml/xml";
import "codemirror/mode/css/css";
import "codemirror/mode/clike/clike";
import "codemirror/mode/javascript/javascript";
import "codemirror/addon/edit/closebrackets";
import "codemirror/addon/edit/matchbrackets";

@Component({
  selector: "editor",
  template: "",
})
export class EditorComponent implements OnInit {
  private _mode: string;
  @Input() set mode(val: string) {
    this._mode = val;
    if (this.cm) {
      this.cm.setOption("mode", val);
    }
  }
  private _value: string;
  @Input() set value(val: string) {
    if (this._value !== val) {
      this._value = val;
      if (this.cm) {
        this.cm.getDoc().setValue(val);
      }
    }
  }
  @Output() valueChange = new EventEmitter();
  @Output() onSubmit = new EventEmitter();

  private cm: CodeMirror.Editor;

  constructor(
    private elementRef: ElementRef) {
  }

  ngOnInit() {
    if (!this.cm) {
      const elm = this.elementRef.nativeElement;
      this.cm = CodeMirror(elm, {
        mode: this._mode,
        lineNumbers: true,
        value: this._value,
        extraKeys: {
          "Ctrl-Enter": cm => this.onSubmit.emit(this._value),
        }
      });
      this.cm.on("change", cm => {
        const val = cm.getDoc().getValue();
        if (this._value !== val) {
          this._value = val;
          this.valueChange.emit(val);
        }
      });
    }
  }
}
