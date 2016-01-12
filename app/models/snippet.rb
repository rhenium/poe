class Snippet < ApplicationRecord
  has_many :results

  validates :code_bytes, length: { maximum: 65535 }

  def compilers
    Compiler.where(language: language).order(version: :desc)
  end

  def results_all
    r = results.map { |r| [r.compiler_id, r] }.to_h
    compilers.map { |c| [r[c.id], c] }
  end

  private
  def code_bytes
    code.bytes
  end
end
