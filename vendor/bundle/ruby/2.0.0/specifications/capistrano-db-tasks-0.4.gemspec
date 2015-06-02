# -*- encoding: utf-8 -*-
# stub: capistrano-db-tasks 0.4 ruby lib

Gem::Specification.new do |s|
  s.name = "capistrano-db-tasks"
  s.version = "0.4"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.require_paths = ["lib"]
  s.authors = ["Sebastien Gruhier"]
  s.date = "2015-02-26"
  s.description = "A collection of capistrano tasks for syncing assets and databases"
  s.email = ["sebastien.gruhier@xilinus.com"]
  s.homepage = "https://github.com/sgruhier/capistrano-db-tasks"
  s.rubyforge_project = "capistrano-db-tasks"
  s.rubygems_version = "2.4.3"
  s.summary = "A collection of capistrano tasks for syncing assets and databases"

  s.installed_by_version = "2.4.3" if s.respond_to? :installed_by_version

  if s.respond_to? :specification_version then
    s.specification_version = 4

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<capistrano>, [">= 3.0.0"])
    else
      s.add_dependency(%q<capistrano>, [">= 3.0.0"])
    end
  else
    s.add_dependency(%q<capistrano>, [">= 3.0.0"])
  end
end
