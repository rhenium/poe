class ExecuteJob < ApplicationJob
  queue_as :default

  def perform(compiler, snippet)
    compiler.run! snippet
  end
end
