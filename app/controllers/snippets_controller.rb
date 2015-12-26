class SnippetsController < ApplicationController
  before_action :set_snippet, only: [:show]

  # GET /snippets
  # TODO
  # def index
  #   @snippets = Snippet.all
  # end

  # GET /1234
  def show
  end

  # GET /
  def new
    @snippet = Snippet.new
  end

  # GET /snippets/1/edit
  # TODO
  # def edit
  # end

  # POST /create
  def create
    @snippet = Snippet.new(snippet_params)

    if @snippet.save
      redirect_to @snippet, notice: "Snippet was successfully created."
    else
      render :new
    end
  end

  # PATCH/PUT /snippets/1
  # TODO
  # def update
  #   if @snippet.update(snippet_params)
  #     redirect_to @snippet, notice: "Snippet was successfully updated."
  #   else
  #     render :edit
  #   end
  # end

  # DELETE /snippets/1
  # TODO
  # def destroy
  #   @snippet.destroy
  #   redirect_to snippets_url, notice: "Snippet was successfully destroyed."
  # end

  private
  # Use callbacks to share common setup or constraints between actions.
  def set_snippet
    @snippet = Snippet.find(params[:id])
  end

  # Only allow a trusted parameter "white list" through.
  def snippet_params
    params.require(:snippet).permit(:title, :code)
  end
end
