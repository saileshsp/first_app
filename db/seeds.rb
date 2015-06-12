User.create!(name:  "Sailesh",
             email: "saileshsp@sp.com",
             password:              "saileshsp",
             password_confirmation: "saileshsp",
             admin: true,
             :avatar => open("/home/knome/personal/first_app/sailesh.jpg"),
             activated: true,
             activated_at: Time.zone.now)

99.times do |n|
  name  = Faker::Name.name
  email = "saileshsp-#{n+1}@sp.com"
  password = "password"
  User.create!(name:  name,
               email: email,
               password:              password,
               password_confirmation: password,
               :avatar => open("/home/knome/personal/first_app/sailesh.jpg"),
               activated: true,
               activated_at: Time.zone.now)
end
users = User.order(:created_at).take(6)
50.times do
  content = Faker::Lorem.sentence(5)
  users.each { |user| user.microposts.create!(content: content) }
end