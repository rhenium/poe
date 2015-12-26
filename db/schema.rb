# encoding: UTF-8
# This file is auto-generated from the current state of the database. Instead
# of editing this file, please use the migrations feature of Active Record to
# incrementally modify your database, and then regenerate this schema definition.
#
# Note that this schema.rb definition is the authoritative source for your
# database schema. If you need to create the application database on another
# system, you should be using db:schema:load, not running all the migrations
# from scratch. The latter is a flawed and unsustainable approach (the more migrations
# you'll amass, the slower it'll run and the greater likelihood for issues).
#
# It's strongly recommended that you check this file into your version control system.

ActiveRecord::Schema.define(version: 20151226120716) do

  create_table "compilers", force: :cascade, options: "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4" do |t|
    t.string   "language",   limit: 64, null: false
    t.string   "version",    limit: 64, null: false
    t.datetime "created_at",            null: false
    t.datetime "updated_at",            null: false
    t.index ["language", "version"], name: "index_compilers_on_language_and_version", unique: true, using: :btree
  end

  create_table "results", force: :cascade, options: "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4" do |t|
    t.integer  "snippet_id",                null: false
    t.integer  "compiler_id",               null: false
    t.binary   "output",      limit: 65535, null: false
    t.datetime "created_at",                null: false
    t.index ["compiler_id"], name: "index_results_on_compiler_id", using: :btree
    t.index ["snippet_id", "compiler_id"], name: "index_results_on_snippet_id_and_compiler_id", unique: true, using: :btree
    t.index ["snippet_id"], name: "index_results_on_snippet_id", using: :btree
  end

  create_table "snippets", force: :cascade, options: "ENGINE=InnoDB DEFAULT CHARSET=utf8mb4" do |t|
    t.string   "title"
    t.binary   "code",       limit: 65535, null: false
    t.datetime "created_at",               null: false
  end

  add_foreign_key "results", "compilers"
  add_foreign_key "results", "snippets"
end
