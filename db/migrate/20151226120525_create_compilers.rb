class CreateCompilers < ActiveRecord::Migration[5.0]
  def change
    create_table :compilers do |t|
      t.string :language, null: false, limit: 64
      t.string :version, null: false, limit: 64

      t.timestamps null: false
    end

    add_index :compilers, [:language, :version], unique: true
  end
end
