class Snippet < ApplicationRecord
  has_many :results

  validates :code, length: { maximum: 65535, tokenizer: :bytes.to_proc }

  def compilers
    Compiler.where(language: language)
  end

  def results_all
    r = results.map { |r| [r.compiler_id, r] }.to_h
    compilers.map { |c| [r[c.id], c] }
  end
end
