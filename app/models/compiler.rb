require "shellwords"

class Compiler < ApplicationRecord
  def to_s
    "#{language} #{version}"
  end

  def run_lazy(snippet)
    r = Result.prepare_execution(self, snippet)
    ExecuteJob.perform_later(self, snippet, r)
    r
  end

  def run(snippet)
    r = Result.prepare_execution(self, snippet)
    run_body(snippet, r)
    r
  end

  private
  def run_body(snippet, r)
    sf = Tempfile.open(encoding: Encoding::BINARY)
    sf.write(snippet.code)
    sf.fsync
    of = Tempfile.open(encoding: Encoding::BINARY)
    ef = Tempfile.open(encoding: Encoding::BINARY)
    pf = Tempfile.open(encoding: Encoding::BINARY)

    plan = {
      "base" => Rails.root.join("playground/base").to_s,
      "environment" => Rails.root.join("playground").join(language).join(version).to_s,
      "source" => sf.path,
      "command" => Shellwords.split(command_line)
    }
    pf.write(JSON.generate(plan))
    pf.fsync
    pf.rewind

    pid = spawn(Rails.root.join("sandbox-go/bin/sandbox-run").to_s,
                in: pf,
                out: of,
                err: ef)
    _, pst = Process.waitpid2(pid)
    ef.rewind
    err = ef.read
    if pst.signaled? || pst.exitstatus > 0
      result = :errored
      status = -1
      Rails.logger.error(err)
      err = nil
    else
      rx, status = err.slice!(0, 8).unpack("ii")
      if status == 0
        result = :success
      else
        result = [:failed, :failed, :timedout][rx]
      end
    end

    of.rewind
    output = of.read(65535)
    r.update!(output: output.to_s,
              truncated: !of.eof?,
              status: status,
              result: result,
              error: err)
  rescue => e
    r.update!(status: -1, result: :errored)
    raise e
  ensure
    sf.close if sf
    of.close if of
    ef.close if ef
    pf.close if pf
  end
end
