Rails.application.routes.draw do
  resources :snippets
  # For details on the DSL available within this file, see http://guides.rubyonrails.org/routing.html

  # Serve websocket cable requests in-process
  # mount ActionCable.server => "/cable"

  root to: "snippets#new"

  post "/create" => "snippets#create"
  put "/update" => "snippets#update"
  get "/:id" => "snippets#show", constraints: { id: /[1-9][0-9]*/ }

  match "*any" => "application#not_found", via: :all
end
