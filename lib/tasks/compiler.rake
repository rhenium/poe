require "fileutils"
require "tmpdir"
require "shellwords"

namespace :compiler do
  RUBIES = {
    "2.3.0" => "https://cache.ruby-lang.org/pub/ruby/2.3/ruby-2.3.0.tar.gz",
    "2.2.4" => "https://cache.ruby-lang.org/pub/ruby/2.2/ruby-2.2.4.tar.gz",
  }
  desc "Install a ruby"
  task :install, :version
  task install: :environment do |t, args|
    url = RUBIES[args[:version]] or raise(ArgumentError, "unknown ruby")
    name = url.split("/").last

    destdir = Rails.root.join("playground/ruby/#{args[:version]}")
    raise ArgumentError, "already installed?" if Dir.exist?(destdir.to_s)
    prefix = "/opt"

    Dir.mktmpdir { |dir|
      FileUtils.chdir(dir) {
        system("curl -O #{Shellwords.escape(url)}") or raise("failed to download")
        system("tar xf #{Shellwords.escape(name)}") or raise("failed to extract")
        FileUtils.chdir(name.split(".tar.gz").first) {
          system("./configure --prefix=#{prefix} --enable-shared --disable-install-doc") or raise("failed to configure")
          system("make -j6") or raise("failed to make")
          system("make install DESTDIR=#{destdir.to_s}") or raise("failed to install")

          Compiler.create!(language: "ruby", version: args[:version], command_line: "#{prefix}/bin/ruby PROGRAM")
        }
      }
    }
  end
end
