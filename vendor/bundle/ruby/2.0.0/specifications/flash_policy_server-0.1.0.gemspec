# -*- encoding: utf-8 -*-
# stub: flash_policy_server 0.1.0 ruby lib

Gem::Specification.new do |s|
  s.name = "flash_policy_server"
  s.version = "0.1.0"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.require_paths = ["lib"]
  s.authors = ["Dave hrycyszyn"]
  s.date = "2013-10-12"
  s.description = "\n    This is a simple Ruby-based policy server to serve Flash's crossdomain.xml \n    policy file.\n\n    The web is increasingly realtime, but websockets still aren't supported on \n    older browser clients. Many server push libraries (e.g. socket.io) attempt \n    to use websockets, with a Flash fallback. Others (amqp.js, for instance) \n    are Flash only.\n\n    When using Flash sockets, it's necessary to have a policy server running on \n    port 843, in order to set cross domain policy. This library does the job.\n  "
  s.email = "dave.hrycyszyn@headlondon.com"
  s.executables = ["flash_policy_server"]
  s.extra_rdoc_files = ["LICENSE.txt", "README.rdoc"]
  s.files = ["LICENSE.txt", "README.rdoc", "bin/flash_policy_server"]
  s.homepage = "http://github.com/futurechimp/flash_policy_server"
  s.licenses = ["MIT"]
  s.rubygems_version = "2.4.3"
  s.summary = "A flash policy server."

  s.installed_by_version = "2.4.3" if s.respond_to? :installed_by_version

  if s.respond_to? :specification_version then
    s.specification_version = 4

    if Gem::Version.new(Gem::VERSION) >= Gem::Version.new('1.2.0') then
      s.add_development_dependency(%q<minitest>, [">= 0"])
      s.add_development_dependency(%q<yard>, ["~> 0.6.0"])
      s.add_development_dependency(%q<bundler>, ["~> 1.0"])
      s.add_development_dependency(%q<jeweler>, [">= 0"])
      s.add_development_dependency(%q<rcov>, [">= 0"])
    else
      s.add_dependency(%q<minitest>, [">= 0"])
      s.add_dependency(%q<yard>, ["~> 0.6.0"])
      s.add_dependency(%q<bundler>, ["~> 1.0"])
      s.add_dependency(%q<jeweler>, [">= 0"])
      s.add_dependency(%q<rcov>, [">= 0"])
    end
  else
    s.add_dependency(%q<minitest>, [">= 0"])
    s.add_dependency(%q<yard>, ["~> 0.6.0"])
    s.add_dependency(%q<bundler>, ["~> 1.0"])
    s.add_dependency(%q<jeweler>, [">= 0"])
    s.add_dependency(%q<rcov>, [">= 0"])
  end
end
