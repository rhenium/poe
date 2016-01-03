class AddCommandLineToCompilers < ActiveRecord::Migration[5.0]
  def change
    add_column :compilers, :command_line, :string, null: false
  end
end
