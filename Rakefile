require "fileutils"
require "json"
require "tmpdir"
require "shellwords"

def load_config
  begin
    $config_file ||= File.expand_path(ENV["CONFIG"] || "config.json")
    $tmp_config = JSON.parse(File.read($config_file))
    $datadir = $tmp_config["datadir"]
  rescue
    puts "Failed to load config: #{$config_file}"
    puts "You can specify config file with CONFIG environment variable"
    raise
  end
end

def add_compiler(lang, id, val)
  load_config
  list = $tmp_config["compilers"][lang] || {}
  list[id] = val
  $tmp_config["compilers"][lang] = list.sort.reverse.to_h
  File.write($config_file, JSON.pretty_generate($tmp_config))
end

def retriable
  yield
rescue
  puts "An error occurred: #{$!}"
  puts "Current directory: #{`pwd`}"
  print "Press a key to retry:"
  $stdin.getc
  retry
end

load_config

namespace :compiler do
  RUBY_MIRROR = "https://cache.ruby-lang.org/pub/ruby"
  desc "Install a ruby"
  task :ruby, [:version] do |t, args|
    version = args[:version]
    id = "ruby-#{version}"
    if version =~ /^(1\.1[a-d]|1\.[02-9]|2\..)/
      archive_dir = "ruby-#{version}"
      url = "#{RUBY_MIRROR}/#{$1}/ruby-#{version}.tar.gz"
    elsif version =~ /^0\./
      archive_dir = "ruby-#{version}"
      url = "#{RUBY_MIRROR}/1.0/ruby-#{version}.tar.gz"
    elsif version =~ /snapshot/
      archive_dir = version
      url = "#{RUBY_MIRROR}/#{version}.tar.gz"
    else
      raise ArgumentError, "unknown ruby"
    end

    destdir = $datadir + "/env/ruby/#{id}"
    raise ArgumentError, "already installed?" if Dir.exist?(destdir.to_s)
    prefix = "/opt"

    Dir.mktmpdir { |tmpdir|
      FileUtils.chdir(tmpdir) {
        system("curl -o archive.tar.gz #{Shellwords.escape(url)}") or raise("failed to download")
        system("tar xf archive.tar.gz") or raise("failed to extract")
        FileUtils.chdir(archive_dir) {
          to_be_applied = []
          patch_ccnames = ["ruby/#{version.split("-").join("/")}", "ruby/#{version.split("-")[0]}", "ruby"]
          patch_ccnames.each { |patch_ccname|
            rvm_patchsets_path = File.expand_path("../vendor/rvm/patchsets/#{patch_ccname}/default", __FILE__)
            if File.exist?(rvm_patchsets_path)
              patches = File.read(rvm_patchsets_path).lines.map(&:chomp)
              puts "RVM patchset found (#{patch_ccname})... #{patches.join(" ")}"
              to_be_applied += patches
            end
          }
          to_be_applied.uniq.each { |patch|
            patch_path = patch_ccnames
              .flat_map { |pp| ["patch", "diff"].map { |ext| File.expand_path("../vendor/rvm/patches/#{pp}/#{patch}.#{ext}", __FILE__) } }
              .find(&File.method(:exist?))
            puts "applying... #{patch}"
            patch_path and system("patch -R -N -p1 --dry-run <#{patch_path} || patch -N -p1 <#{patch_path}") or
              raise("failed to apply patch")
          }
          retriable {
            system("./configure --prefix=#{prefix} --enable-shared --disable-install-doc") or raise("failed to configure")
            system("make -j6") or raise("failed to make")
            system("make install DESTDIR=#{destdir}") or raise("failed to install")
          }

          add_compiler("ruby", id, {
            version_command: "#{prefix}/bin/ruby -v",
            version: `LD_LIBRARY_PATH=#{destdir}#{prefix}/lib #{destdir}#{prefix}/bin/ruby -v`.lines.first.chomp,
            commandline: ["#{prefix}/bin/ruby", "{}"]
          })
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

    destdir = $datadir + "/env/php/#{id}"
    raise ArgumentError, "already installed?" if Dir.exist?(destdir.to_s)
    prefix = "/opt"

    Dir.mktmpdir { |tmpdir|
      FileUtils.chdir(tmpdir) {
        system("curl -O #{Shellwords.escape(url)}") or raise("failed to download")
        system("tar xf #{Shellwords.escape(name)}") or raise("failed to extract")
        FileUtils.chdir(name.split(".tar.gz").first) {
          retriable {
            system("./configure --prefix=#{prefix} --enable-shared") or raise("failed to configure")
            system("make -j6") or raise("failed to make")
            system("make install INSTALL_ROOT=#{destdir.to_s}") or raise("failed to install")
          }

          add_compiler("php", id, {
            version_command: "#{prefix}/bin/php -v",
            version: `LD_LIBRARY_PATH=#{destdir}#{prefix}/lib #{destdir}#{prefix}/bin/php -v`.lines.first.chomp,
            commandline: ["#{prefix}/bin/php", "{}"]
          })
        }
      }
    }
  end
end
