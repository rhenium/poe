threads 8, 16
workers 2
environment "production"

bind "unix://" + File.join(File.expand_path("../../", __FILE__), "tmp/sockets/rack.sock")

on_worker_boot do
  ActiveSupport.on_load(:active_record) do
    ActiveRecord::Base.establish_connection
  end
end

before_fork do
  ActiveRecord::Base.connection_pool.disconnect!
end

preload_app!
