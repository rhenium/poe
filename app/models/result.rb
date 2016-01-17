require "cgi"

class Result < ApplicationRecord
  belongs_to :snippet
  belongs_to :compiler

  enum result: [:success, :failed, :errored, :timedout, :running]

  def finished?
    !running?
  end

  def parse_output
    orig = output.b
    ret = []
    while orig.bytesize > 0
      fd, len = orig.slice!(0, 8).unpack("ii")
      p [fd, len]
      if !truncated? && (!len || orig.bytesize < len)
        p orig
        p output.b
        raise "output is too short"
      end
      ret << [fd, orig.slice!(0, len)]
    end
    ret
  end

  def formatted_output
    s = "".b
    last_c = nil
    parse_output.each { |fd, c|
      if fd == 1
        s << CGI.escapeHTML(c)
      else
        s << "<span style='color: red'>" << CGI.escapeHTML(c) << "</span>"
      end
      last_c = c
    }

    if truncated?
      s << "<span style='color: white; background-color: black'>#truncated#</span>"
    elsif last_c && last_c[-1] != "\n"
      s << "<span style='color: white; background-color: black'>%</span>"
    end

    if error.present?
      s << "<span style='color: #6666ff; font-style: bold'>" << CGI.escapeHTML(error) << "</span>"
    end

    s.html_safe
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
