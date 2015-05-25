# -*- encoding: utf-8 -*-
# stub: rvm-capistrano 1.5.0 ruby lib

Gem::Specification.new do |s|
  s.name = "rvm-capistrano"
  s.version = "1.5.0"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.require_paths = ["lib"]
  s.authors = ["Wayne E. Seguin", "Michal Papis"]
  s.date = "2013-09-04"
  s.description = "RVM / Capistrano Integration Gem"
  s.email = ["wayneeseguin@gmail.com", "mpapis@gmail.com"]
  s.homepage = "https://github.com/wayneeseguin/rvm-capistrano"
  s.licenses = ["MIT"]
  s.rubygems_version = "2.4.3"
  s.summary = "RVM / Capistrano Integration Gem"

  s.installed_by_version = "2.4.3" if s.respond_to? :installed_by_version

  if s.respond_to? :specification_version then
    s.specification_version = 3

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<capistrano>, [">= 2.15.4"])
      s.add_development_dependency(%q<rake>, [">= 0"])
      s.add_development_dependency(%q<rspec>, [">= 0"])
      s.add_development_dependency(%q<capistrano-spec>, [">= 0"])
      s.add_development_dependency(%q<guard-rspec>, [">= 0"])
      s.add_development_dependency(%q<guard-bundler>, [">= 0"])
    else
      s.add_dependency(%q<capistrano>, [">= 2.15.4"])
      s.add_dependency(%q<rake>, [">= 0"])
      s.add_dependency(%q<rspec>, [">= 0"])
      s.add_dependency(%q<capistrano-spec>, [">= 0"])
      s.add_dependency(%q<guard-rspec>, [">= 0"])
      s.add_dependency(%q<guard-bundler>, [">= 0"])
    end
  else
    s.add_dependency(%q<capistrano>, [">= 2.15.4"])
    s.add_dependency(%q<rake>, [">= 0"])
    s.add_dependency(%q<rspec>, [">= 0"])
    s.add_dependency(%q<capistrano-spec>, [">= 0"])
    s.add_dependency(%q<guard-rspec>, [">= 0"])
    s.add_dependency(%q<guard-bundler>, [">= 0"])
  end
end
