require "cgi"

class Result < ApplicationRecord
  belongs_to :snippet
  belongs_to :compiler

  enum result: [:success, :failed, :errored, :running]

  def finished?
    !running?
  end

  def parse_output
    orig = output.b
    ret = []
    while orig.bytesize > 0
      fd, len = orig.slice!(0, 5).unpack("CV")
      raise "output is too short" if !len || orig.bytesize < len
      ret << [fd, orig.slice!(0, len)]
    end
    ret
  end

  def formatted_output
    parse_output.inject("".b) { |s, (fd, c)|
      if fd == 1
        s << CGI.escapeHTML(c)
      else
        s << "<span style='color: red'>" << CGI.escapeHTML(c) << "</span>"
      end
    }.html_safe
  end

  def self.prepare_execution(compiler, snippet)
    # kuso
    r = Result.find_by(snippet: snippet, compiler: compiler)
    if r
      r.update(output: "", status: -1, result: :running)
    else
      r = Result.create!(snippet: snippet, compiler: compiler, output: "", status: -1, result: :running)
    end
    r
  end
end
