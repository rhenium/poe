class AddTruncatedToResult < ActiveRecord::Migration[5.0]
  def change
    add_column :results, :truncated, :boolean, null: false, default: false
  end
end
