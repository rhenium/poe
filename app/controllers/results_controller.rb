class ResultsController < ApplicationController
  before_action :set_result, only: [:show]

  def run
    compiler = Compiler.find(params[:compiler_id])
    snippet = Snippet.find(params[:snippet_id])
    @result = compiler.run(snippet)

    show
  end

  def show
    render text: render_to_string(partial: "results/result", locals: { result: @result, compiler: @result.compiler }), content_type: "text/plain"
  end

  private
  def set_result
    @result = Result.find(params[:id])
  end
end
