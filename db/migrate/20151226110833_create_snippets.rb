class CreateSnippets < ActiveRecord::Migration[5.0]
  def change
    create_table :snippets do |t|
      t.string :title, null: true
      t.binary :code, null: false

      t.datetime :created_at, null: false
    end
  end
end
