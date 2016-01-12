class AddVersionLongToCompiler < ActiveRecord::Migration[5.0]
  def change
    add_column :compilers, :version_long, :text, null: false
  end
end
