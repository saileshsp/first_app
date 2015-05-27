# config valid only for current version of Capistrano
#lock '3.4.0'
# rbenv

    
#require "bundler/capistrano"
#require 'capistrano/ext/multistage'
#require "whenever/capistrano"

#set :whenever_command, "bundle exec whenever"
#set :whenever_identifier, defer { "#{application}_#{stage}" }

#load 'deploy/assets'
#set :rbenv_type, :user
#set :rbenv_ruby, '2.0.0-p247'
#require "rvm/capistrano"
#require "capistrano/rvm"     
#require "bundler/capistrano" 
#set :rbenv_type, :user
#set :rbenv_ruby, '2.0.0-p598'
#set :rvm_ruby_version, '2.0.0p598'
#set :default_env, { rvm_bin_path: '~/.rvm/bin' }
#SSHKit.config.command_map[:rake] ||= "rake"
#SSHKit.config.command_map[:rake].sub!(/\(.*\)rake/, "\1bundle exec rake")
#SSHKit.config.command_map[:rake] = "#{fetch(:default_env)[:rvm_bin_path]}/rvm ruby-#{fetch(:rvm_ruby_version)} do bundle exec rake"
#
# bundler

# rails
set :rails_env, 'production'
# Define the name of the application
set :application, 'first_app'

# Define where can Capistrano access the source repository
# set :repo_url, 'https://github.com/[user name]/[application name].git'
set :scm, :git
set :repo_url, 'https://github.com/saileshsp/first_app.git'

# Define where to put your application code
#set :deploy_to, "/home/deployer/apps/first_app"
set :deploy_to, "/home/knome/sailesh/first_app"

set :pty, true

set :format, :pretty

set :use_sudo, true            
set :user, "knome"
#set :default_env, { rvm_bin_path: '~/.rvm/bin' }

# setup rvm.
#set :rbenv_type, :system
#set :rbenv_ruby, '2.1.2'
#set :rbenv_prefix, "RBENV_ROOT=#{fetch(:rbenv_path)} RBENV_VERSION=#{fetch(:rbenv_ruby)} #{fetch(:rbenv_path)}/bin/rbenv exec"
#set :rbenv_map_bins, %w{rake gem bundle ruby rails}

set :to_symlink,
  ["config/database.yml","public/assets"]

set :keep_releases, 5
#set :linked_files, %w{config/database.yml}
#$set :linked_dirs, %w{bin log tmp/pids tmp/cache tmp/sockets vendor/bundle public/system}
#set :tests, []

# Set the post-deployment instructions here.
# Once the deployment is complete, Capistrano
# will begin performing them as described.
# To learn more about creating tasks,
# check out:
# http://capistranorb.com/

after "deploy:bundle_install", "deploy:start"

#before "deploy:restart", "fix:permission"

#namespace :fix do
  #task :permission, roles(:app, :background, :live) do
  #  run  "chown -R deploy:deploy #{deploy_to}"
  #end
#end
namespace :deploy do
  desc "Start Application"
  task :start do 
   on roles(:app) do

    #run "cd #{previous_release}; source $HOME/.bash_profile && thin stop -C config/thin.yml"
    run "cd #{deploy_to}/current && thin start -C config/thin.yml"
   end
  end
  desc "Bundle install for RVMs sake"
  task :bundle_install do
   on roles(:app) do
    
    run   "cd #{deploy_to}/current && bundle install" 
    
  end
end
  after :restart, :clear_cache do
    on roles(:web), in: :groups, limit: 3, wait: 10 do
      # Here we can do anything such as:
      # within release_path do
      #   execute :rake, 'cache:clear'
      # end
   end
  end

  after :finishing, 'deploy:cleanup'


end



# desc 'Restart application'
 # task :restart do
  #  on roles(:app), in: :sequence, wait: 5 do
     # execute "service thin restart"  ## -> line you should add
   # end
  #end


#   after :publishing, :restart

#   after :restart, :clear_cache do
#     on roles(:web), in: :groups, limit: 3, wait: 10 do
#       # Here we can do anything such as:
#       # within release_path do
#       #   execute :rake, 'cache:clear'
#       # end
#     end
#   end

# end



# Default branch is :master
# ask :branch, `git rev-parse --abbrev-ref HEAD`.chomp

# Default deploy_to directory is /var/www/my_app_name
# set :deploy_to, '/var/www/my_app_name'

# Default value for :scm is :git
# set :scm, :git

# Default value for :format is :pretty
# set :format, :pretty

# Default value for :log_level is :debug
# set :log_level, :debug

# Default value for :pty is false
# set :pty, true

# Default value for :linked_files is []
# set :linked_files, fetch(:linked_files, []).push('config/database.yml', 'config/secrets.yml')

# Default value for linked_dirs is []
# set :linked_dirs, fetch(:linked_dirs, []).push('log', 'tmp/pids', 'tmp/cache', 'tmp/sockets', 'vendor/bundle', 'public/system')

# Default value for default_env is {}
# set :default_env, { path: "/opt/ruby/bin:$PATH" }

# Default value for keep_releases is 5
# set :keep_releases, 5

#namespace :deploy do

  #after :restart, :clear_cache do
  #  on roles(:web), in: :groups, limit: 3, wait: 10 do
      # Here we can do anything such as:
      # within release_path do
      #   execute :rake, 'cache:clear'
      # end
   # end
 # end


