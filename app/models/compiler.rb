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
    baseroot = Rails.root.join("playground/base").to_s
    env_overlay = Rails.root.join("playground").join(language).join(version).to_s
    sf = Tempfile.open
    sf.write(snippet.code)
    sf.fsync
    of = Tempfile.open
    pid = spawn("/usr/bin/sudo", Rails.root.join("sandbox/sandbox").to_s, baseroot, env_overlay, sf.path, *Shellwords.split(command_line),
                in: :close, # TODO
                out: of,
                err: STDERR)
    _, pst = Process.waitpid2(pid)
    if pst.signaled? || pst.exitstatus > 0
      result = :errored
      status = -1
    else
      result = :success
      status = 0
    end

    of.rewind
    r.update!(output: of.read,
              status: status,
              result: result)
  rescue
    r.update!(status: -1, result: :errored)
  ensure
    sf.close if sf
    of.close if of
  end
end
