class AddErrorToResult < ActiveRecord::Migration[5.0]
  def change
    add_column :results, :error, :string, null: true
  end
end
