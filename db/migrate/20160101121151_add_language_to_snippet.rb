class AddLanguageToSnippet < ActiveRecord::Migration[5.0]
  def change
    add_column :snippets, :language, :string, null: false
  end
end
