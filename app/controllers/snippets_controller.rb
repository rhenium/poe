class SnippetsController < ApplicationController
  before_action :set_snippet, only: [:show]

  # GET /1234
  def show
  end

  # GET /
  def new
    @snippet = Snippet.new
  end

  # POST /create
  def create
    @snippet = Snippet.new(snippet_params)

    if @snippet.save
      redirect_to @snippet, notice: "Snippet was successfully created."
    else
      render :new
    end
  end

  private
  # Use callbacks to share common setup or constraints between actions.
  def set_snippet
    @snippet = Snippet.find(params[:id])
  end

  # Only allow a trusted parameter "white list" through.
  def snippet_params
    params.require(:snippet).permit(:title, :code, :language)
  end
end
