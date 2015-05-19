# server-based syntax
# ======================
# Define roles, user and IP address of deployment server
# role :name, %{[user]@[IP adde.]}
role :app, %w{deployer@10.18.83.143}
role :web, %w{deployer@10.18.83.143}
role :db, %w{deployer@10.18.83.143}

# Define server(s)
server "10.18.83.143", user: "deployer", role: %w{app}
server "10.18.83.143", user: "deployer", role: %w{db}
server "10.18.83.143", user: "deployer", role: %w{web}
# SSH Options
# See the example commented out section in the file
# for more options.

set :ssh_options, {
    forward_agent: false,
    auth_methods: %w(password),
    password: 'password',
    user: 'deployer',
    keys: %w(/home/knome/.ssh/id_rsa)
}

#set :enable_ssl, false
set :deploy_to, "/home/deployer/apps/first_app"
#server "#{server_ip_here}", user: "deploy", roles: %w{web app db}, port: 222
#before "deploy:restart", "fix:permission"

#namespace :fix do
 # task :permission do
  #	on  roles(:app) do
   # run  "chown -R deploy:deploy #{deploy_to}"
  	#end
  #end




# Defines a single server with a list of roles and multiple properties.
# You can define all roles on a single server, or split them:

# server 'example.com', user: 'deploy', roles: %w{app db web}, my_property: :my_value
# server 'example.com', user: 'deploy', roles: %w{app web}, other_property: :other_value
# server 'db.example.com', user: 'deploy', roles: %w{db}



# role-based syntax
# ==================

# Defines a role with one or multiple servers. The primary server in each
# group is considered to be the first unless any  hosts have the primary
# property set. Specify the username and a domain or IP for the server.
# Don't use `:all`, it's a meta role.

# role :app, %w{deploy@example.com}, my_property: :my_value
# role :web, %w{user1@primary.com user2@additional.com}, other_property: :other_value
# role :db,  %w{deploy@example.com}



# Configuration
# =============
# You can set any configuration variable like in config/deploy.rb
# These variables are then only loaded and set in this stage.
# For available Capistrano configuration variables see the documentation page.
# http://capistranorb.com/documentation/getting-started/configuration/
# Feel free to add new variables to customise your setup.



# Custom SSH Options
# ==================
# You may pass any option but keep in mind that net/ssh understands a
# limited set of options, consult the Net::SSH documentation.
# http://net-ssh.github.io/net-ssh/classes/Net/SSH.html#method-c-start
#
# Global options
# --------------
#  set :ssh_options, {
#    keys: %w(/home/rlisowski/.ssh/id_rsa),
#    forward_agent: false,
#    auth_methods: %w(password)
#  }
#
# The server-based syntax can be used to override options:
# ------------------------------------
# server 'example.com',
#   user: 'user_name',
#   roles: %w{web app},
#   ssh_options: {
#     user: 'user_name', # overrides user setting above
#     keys: %w(/home/user_name/.ssh/id_rsa),
#     forward_agent: false,
#     auth_methods: %w(publickey password)
#     # password: 'please use keys'
#   }
