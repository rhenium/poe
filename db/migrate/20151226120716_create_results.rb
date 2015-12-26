class CreateResults < ActiveRecord::Migration[5.0]
  def change
    create_table :results do |t|
      t.belongs_to :snippet, index: true, foreign_key: true, null: false
      t.belongs_to :compiler, index: true, foreign_key: true, null: false
      t.binary :output, null: false

      t.datetime :created_at, null: false
    end

    add_index :results, [:snippet_id, :compiler_id], unique: true
  end
end
