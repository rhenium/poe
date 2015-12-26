class Compiler < ApplicationRecord
  def _run!(snippet)
    raise NotImplementedError
  end

  if Rails.env.development?
    require "shellwords"
    def run!(snippet)
      if self.language == "ruby" && self.version == "!!unsafe!!"
        out = `ruby -e #{Shellwords.escape(snippet.code)}`
        Result.create(snippet: snippet, compiler: self, output: out)
      else
        _run!(snippet)
      end
    end
  else
    alias run! _run!
  end
end
