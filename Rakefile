require "fileutils"
require "json"
require "tmpdir"
require "shellwords"

begin
  $config_file = File.expand_path(ENV["CONFIG"] || "config.json")
  $config = JSON.parse(File.read($config_file))
rescue
  puts "Failed to load config: #{$config_file}"
  puts "You can specify config file with CONFIG environment variable"
  raise
end

def save_config
  File.write($config_file, JSON.pretty_generate($config))
end

def retriable
  yield
rescue
  puts "An error occurred: #{$!}"
  puts "Current directory: #{`pwd`}"
  print "Press a key to retry:"
  $stdin.getch
  retry
end

namespace :compiler do
  RUBY_MIRROR = "https://cache.ruby-lang.org/pub/ruby"
  desc "Install a ruby"
  task :ruby, [:version] do |t, args|
    version = args[:version]
    id = "ruby-#{version}"
    if version =~ /^(1\.1[a-d]|1\.[02-9]|2\..)/
      url = "#{RUBY_MIRROR}/#{$1}/ruby-#{version}.tar.gz"
    elsif version =~ /^0\./
      url = "#{RUBY_MIRROR}/1.0/ruby-#{version}.tar.gz"
    else
      raise ArgumentError, "unknown ruby"
    end

    destdir = $config["datadir"] + "/env/ruby/#{id}"
    raise ArgumentError, "already installed?" if Dir.exist?(destdir.to_s)
    prefix = "/opt"

    Dir.mktmpdir { |dir|
      FileUtils.chdir(dir) {
        system("curl -o archive.tar.gz #{Shellwords.escape(url)}") or raise("failed to download")
        system("tar xf archive.tar.gz") or raise("failed to extract")
        FileUtils.chdir("ruby-#{version}") {
          patch_ccnames = ["ruby", "ruby/#{version.split("-")[0]}", "ruby/#{version.split("-").join("/")}"]
          patch_ccnames.each { |patch_ccname|
            rvm_patchsets_path = File.expand_path("../vendor/rvm/patchsets/#{patch_ccname}/default", __FILE__)
            if File.exist?(rvm_patchsets_path)
              puts "RVM patchset found (#{patch_ccname})... applying..."
              patches = File.read(rvm_patchsets_path).lines.map(&:chomp)
              patches.each { |patch|
                patch_path = patch_ccnames
                  .map { |pp| File.expand_path("../vendor/rvm/patches/#{pp}/#{patch}.patch", __FILE__) }
                  .find(&File.method(:exist?))
                patch_path and system("patch -R -p1 --silent --dry-run <#{patch_path} || patch -p1 <#{patch_path}") or
                  raise("failed to apply patch")
              }
            end
          }
          retriable {
            system("./configure --prefix=#{prefix} --enable-shared --disable-install-doc") or raise("failed to configure")
            system("make -j6") or raise("failed to make")
            system("make install DESTDIR=#{destdir}") or raise("failed to install")
          }

          ($config["compilers"]["ruby"] ||= {})[id] = {
            version: `LD_LIBRARY_PATH=#{destdir}#{prefix}/lib #{destdir}#{prefix}/bin/ruby -v`.lines.first.chomp,
            commandline: ["#{prefix}/bin/ruby", "{}"]
          }
          save_config
        }
      }
    }
  end

  PHPS = {
    "7.0.3" => "http://jp2.php.net/distributions/php-7.0.3.tar.gz",
    "5.6.17" => "http://jp2.php.net/distributions/php-5.6.17.tar.gz",
  }
  desc "Install a php"
  task :php, [:version] do |t, args|
    version = args[:version]
    id = "ruby-#{version}"
    url = PHPS[version] or raise(ArgumentError, "unknown php")
    name = url.split("/").last

    destdir = $config["datadir"] + "/env/php/#{id}"
    raise ArgumentError, "already installed?" if Dir.exist?(destdir.to_s)
    prefix = "/opt"

    Dir.mktmpdir { |dir|
      FileUtils.chdir(dir) {
        system("curl -O #{Shellwords.escape(url)}") or raise("failed to download")
        system("tar xf #{Shellwords.escape(name)}") or raise("failed to extract")
        FileUtils.chdir(name.split(".tar.gz").first) {
          retriable {
            system("./configure --prefix=#{prefix} --enable-shared") or raise("failed to configure")
            system("make -j6") or raise("failed to make")
            system("make install INSTALL_ROOT=#{destdir.to_s}") or raise("failed to install")
          }

          ($config["compilers"]["php"] ||= {})[id] = {
            version: `LD_LIBRARY_PATH=#{destdir}#{prefix}/lib #{destdir}#{prefix}/bin/php -v`.lines.first.chomp,
            commandline: ["#{prefix}/bin/php", "{}"]
          }
          save_config
        }
      }
    }
  end
end
