# -*- encoding: utf-8 -*-
# stub: rvm1-capistrano3 1.3.2.2 ruby lib

Gem::Specification.new do |s|
  s.name = "rvm1-capistrano3"
  s.version = "1.3.2.2"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.require_paths = ["lib"]
  s.authors = ["Michal Papis"]
  s.date = "2014-10-08"
  s.description = "RVM 1.x / Capistrano 3.x Integration Gem"
  s.email = ["mpapis@gmail.com"]
  s.homepage = "https://github.com/rvm/rvm1-capistrano3"
  s.licenses = ["Apache 2"]
  s.rubygems_version = "2.4.3"
  s.summary = "RVM 1.x / Capistrano 3.x Integration Gem"

  s.installed_by_version = "2.4.3" if s.respond_to? :installed_by_version

  if s.respond_to? :specification_version then
    s.specification_version = 4

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<capistrano>, ["~> 3.0"])
      s.add_runtime_dependency(%q<sshkit>, [">= 1.2"])
      s.add_development_dependency(%q<tf>, ["~> 0.4.3"])
    else
      s.add_dependency(%q<capistrano>, ["~> 3.0"])
      s.add_dependency(%q<sshkit>, [">= 1.2"])
      s.add_dependency(%q<tf>, ["~> 0.4.3"])
    end
  else
    s.add_dependency(%q<capistrano>, ["~> 3.0"])
    s.add_dependency(%q<sshkit>, [">= 1.2"])
    s.add_dependency(%q<tf>, ["~> 0.4.3"])
  end
end
