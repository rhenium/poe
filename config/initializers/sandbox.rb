FileUtils.chdir(Rails.root.join("sandbox")) {
  raise "Sandbox is not built" unless /'sandbox' is up to date./ =~ `make -n`
}
