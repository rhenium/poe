require "fileutils"
require "tmpdir"
require "shellwords"

namespace :compiler do
  RUBIES = {
    "2.3.0" => "https://cache.ruby-lang.org/pub/ruby/2.3/ruby-2.3.0.tar.gz",
    "2.2.4" => "https://cache.ruby-lang.org/pub/ruby/2.2/ruby-2.2.4.tar.gz",
    "2.2.3" => "https://cache.ruby-lang.org/pub/ruby/2.2/ruby-2.2.3.tar.gz",
    "2.1.8" => "https://cache.ruby-lang.org/pub/ruby/2.1/ruby-2.1.8.tar.gz",
    "2.0.0-p648" => "https://cache.ruby-lang.org/pub/ruby/2.0/ruby-2.0.0-p648.tar.gz",
  }
  desc "Install a ruby"
  task :ruby, :version
  task ruby: :environment do |t, args|
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

          Compiler.create!(language: "ruby",
                           version: args[:version],
                           version_long: `LD_LIBRARY_PATH=#{destdir}#{prefix}/lib #{destdir}#{prefix}/bin/ruby -v`.lines.first.chomp,
                           command_line: "#{prefix}/bin/ruby PROGRAM")
        }
      }
    }
  end

  PHPS = {
    "7.0.2" => "http://jp2.php.net/distributions/php-7.0.2.tar.gz",
    "5.6.17" => "http://jp2.php.net/distributions/php-5.6.17.tar.gz",
  }
  desc "Install a php"
  task :php, :version
  task php: :environment do |t, args|
    url = PHPS[args[:version]] or raise(ArgumentError, "unknown php")
    name = url.split("/").last

    destdir = Rails.root.join("playground/php/#{args[:version]}")
    raise ArgumentError, "already installed?" if Dir.exist?(destdir.to_s)
    prefix = "/opt"

    Dir.mktmpdir { |dir|
      FileUtils.chdir(dir) {
        system("curl -O #{Shellwords.escape(url)}") or raise("failed to download")
        system("tar xf #{Shellwords.escape(name)}") or raise("failed to extract")
        FileUtils.chdir(name.split(".tar.gz").first) {
          system("./configure --prefix=#{prefix} --enable-shared") or raise("failed to configure")
          system("make -j6") or raise("failed to make")
          system("make install INSTALL_ROOT=#{destdir.to_s}") or raise("failed to install")

          Compiler.create!(language: "php",
                           version: args[:version],
                           version_long: `LD_LIBRARY_PATH=#{destdir}#{prefix}/lib #{destdir}#{prefix}/bin/php -v`.lines.first.chomp,
                           command_line: "#{prefix}/bin/php PROGRAM")
        }
      }
    }
  end
end
