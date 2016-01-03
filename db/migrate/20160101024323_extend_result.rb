class ExtendResult < ActiveRecord::Migration[5.0]
  def change
    add_column :results, :result, :integer, null: false
    add_column :results, :status, :integer, null: false
  end
end
