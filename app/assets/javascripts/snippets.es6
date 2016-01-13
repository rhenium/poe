// Place all the behaviors and hooks related to the matching controller here.
// All this logic will automatically be available in application.js.

const codeField = document.querySelector("#code-field");
if (codeField !== null) {
  const origTextarea = codeField.querySelector("textarea[name=\"snippet[code]\"]");
  const codeMirror = CodeMirror.fromTextArea(origTextarea, {
    mode: "ruby",
    lineNumbers: true,
    extraKeys: {
      "Ctrl-Enter": cm => {
        cm.save();
        origTextarea.form.submit();
      }
    }
  });
  codeMirror.on("change", cm => cm.save());
}

const agg = (elm, n) => {
  if (!n) n = 0;
  n++;
  setTimeout(() => {
    $.ajax({
      type: "GET",
      url: "/results/" + elm.getAttribute("data-id"),
      dataType: "text",
      success: (text, st) => {
        elm.outerHTML = text;
        if (n < 3) agg(elm, n);
      }
    });
  }, 1000);
};

const refresher = () => {
  const runnings = document.querySelectorAll(".result-item[data-status=running]");
  for (var i = 0, len = runnings.length; i < len; i++) {
    agg(runnings[i]);
  }
};
refresher();

const notrans = document.querySelectorAll(".result-item[data-status=notran]");
const snippet_id = document.querySelector("#snippet-id").innerHTML;
for (var i = 0, len = notrans.length; i < len; i++) {
  const elm = notrans[i];
  $.ajax({
    type: "POST",
    url: "/results/run",
    dataType: "text",
    data: { compiler_id: elm.getAttribute("data-compiler-id"), snippet_id: snippet_id },
    success: (text, st) => {
      elm.outerHTML = text;
    }
  });
}
