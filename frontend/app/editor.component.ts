import {Component, OnInit, ElementRef, Input, Output, EventEmitter} from "angular2/core";
import {EditingData, EditingDataService} from "./editing-data.service";

@Component({
  selector: "editor",
  template: `
    <textarea></textarea>
  `,
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

  private cm: CodeMirror.EditorFromTextArea;

  constructor(
    private elementRef: ElementRef) { }

  ngOnInit() {
    if (!this.cm) {
      const elm = this.elementRef.nativeElement;
      const origTextarea = <HTMLTextAreaElement>elm.querySelector("textarea");
      this.cm = CodeMirror.fromTextArea(origTextarea, {
        mode: this._mode,
        lineNumbers: true,
        value: this._value,
        extraKeys: {
          "Ctrl-Enter": cm => this.onSubmit.emit(this._value),
        }
      });
      this.cm.on("change", cm => {
        this.cm.save();
        if (this._value !== origTextarea.value) {
          this._value = origTextarea.value;
          this.valueChange.emit(this._value);
        }
      });
    }
  }
}
