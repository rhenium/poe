directories %w(sandbox sandbox-go)

guard :shell do
  watch(%r{^sandbox/(.*\.c|.*\.h|Makefile)$}) { |m|
    FileUtils.chdir(File.expand_path("../sandbox", __FILE__)) {
      system("make && sudo make install")
    }
  }
  watch(%r{^sandbox-go/src/[^/]*\.go$}) { |m|
    p m
    FileUtils.chdir(File.expand_path("../sandbox-go/src", __FILE__)) { |dir|
      system("GOPATH=#{dir}/.. go build -o sandbox && sudo install -m 6755 -o root sandbox ../bin/sandbox-run && echo Successfully compiled")
    }
  }
end
