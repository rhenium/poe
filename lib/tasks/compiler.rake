require "fileutils"
require "tmpdir"
require "shellwords"

namespace :compiler do
  RUBY_MIRROR = "https://cache.ruby-lang.org/pub/ruby"
  desc "Install a ruby"
  task :ruby, :version
  task ruby: :environment do |t, args|
    version = args[:version]
    if version =~ /^(1\.1[a-d]|1\.[02-9]|2\..)/
      url = "#{RUBY_MIRROR}/#{$1}/ruby-#{version}.tar.gz"
    elsif version =~ /^0\./
      url = "#{RUBY_MIRROR}/1.0/ruby-#{version}.tar.gz"
    else
      raise ArgumentError, "unknown ruby"
    end

    destdir = Rails.root.join("playground/ruby/#{version}")
    raise ArgumentError, "already installed?" if Dir.exist?(destdir.to_s)
    prefix = "/opt"

    Dir.mktmpdir { |dir|
      FileUtils.chdir(dir) {
        system("curl -o archive.tar.gz #{Shellwords.escape(url)}") or raise("failed to download")
        system("tar xf archive.tar.gz") or raise("failed to extract")
        FileUtils.chdir("ruby-#{version}") {
          system("./configure --prefix=#{prefix} --enable-shared --disable-install-doc") or raise("failed to configure")
          system("make -j6") or raise("failed to make")
          system("make install DESTDIR=#{destdir}") or raise("failed to install")

          Compiler.create!(language: "ruby",
                           version: version,
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
