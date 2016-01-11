class ResultsController < ApplicationController
  before_action :set_result, only: [:show]

  def run
    compiler = Compiler.find(params[:compiler_id])
    snippet = Snippet.find(params[:snippet_id])
    @result = compiler.run(snippet)

    show
  end

  def show
    render plain: render_to_string(partial: "results/result", locals: { result: @result, compiler: @result.compiler })
  end

  private
  def set_result
    @result = Result.find(params[:id])
  end
end
