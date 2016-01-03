class ExecuteJob < ApplicationJob
  queue_as :default

  def perform(compiler, snippet, result = nil)
    if result
      compiler.run_body(snippet, result)
    else
      compiler.run(snippet)
    end
  end
end
