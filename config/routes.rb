Rails.application.routes.draw do
  post "results/run" => "results#run"
  get "results/:id" => "results#show"
  get "about" => "about#index"

  resources :snippets, only: [:new, :create, :update, :show], path: "", path_names: {
    new: "",
    create: "create",
    update: ":id",
    show: ":id"
  }
  # Serve websocket cable requests in-process
  # mount ActionCable.server => "/cable"

  #root to: "snippets#new"

  #post "/create" => "snippets#create"
  #put "/update" => "snippets#update"
  #get "/:id" => "snippets#show", constraints: { id: /[1-9][0-9]*/ }

  match "*any" => "application#not_found", via: :all
end
