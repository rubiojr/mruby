MRuby::Gem::Specification.new('mruby-io') do |spec|
  spec.license = 'MIT'
  spec.authors = 'Internet Initiative Japan'

  spec.mruby.cc.defines << 'ENABLE_IO'
  spec.cc.include_paths << "#{build.root}/src"
end
