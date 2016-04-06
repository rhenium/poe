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
  File.write($config_file, JSON.pretty_generate($tmp_config) + "\n")
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

RUBY_PATCHES = {
  ruby: {
    /^(1.8.[01])/ => ["tcltklib-Tcl_GetStringResult"],
    /^(1.8.[0-2])/ => ["r8532-X509_STORE_CTX-flags"],
    /^(1.8.[0-6])/ => ["r16422-New-OpenSSL"],
    /^(1.8|1.9.1)/ => ["r26781-OpenSSL10"],
    /^(1.8|1.9|2.0|2.1|2.2)/ => ["r31346-r31528-SSLv2", "r51722-SSLv3"],
    /^(1.8.7|1.9|2.[012])/ => ["r41808-EC2M"],
  }
}

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
          RUBY_PATCHES[:ruby].each { |regexp, patch_names|
            next if regexp !~ version
            patch_names.each { |name|
              puts "applying patch #{name}..."
              system("patch -N -p1 <#{File.expand_path("../patches/ruby/#{name}.patch", __FILE__)}") or
                puts("patching failed: #{name}, ignoring")
            }
          }
          retriable {
            system("./configure --prefix=#{prefix} --enable-shared --disable-install-doc") or raise("failed to configure")
            system("make -j6") or raise("failed to make")
            system("make install DESTDIR=#{destdir}") or raise("failed to install")
          }

          add_compiler("ruby", id, {
            version: `LD_LIBRARY_PATH=#{destdir}#{prefix}/lib #{destdir}#{prefix}/bin/ruby -v`.lines.first.chomp,
            version_command: "#{prefix}/bin/ruby -v",
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
            version: `LD_LIBRARY_PATH=#{destdir}#{prefix}/lib #{destdir}#{prefix}/bin/php -v`.lines.first.chomp,
            version_command: "#{prefix}/bin/php -v",
            commandline: ["#{prefix}/bin/php", "{}"]
          })
        }
      }
    }
  end
end
