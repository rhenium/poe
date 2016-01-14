guard :shell do
  watch(%r{^sandbox/(.*\.c|.*\.h|Makefile)$}) { |m|
    p m
    FileUtils.chdir(File.expand_path("../sandbox", __FILE__)) {
      system("make && sudo make install")
    }
  }
end
