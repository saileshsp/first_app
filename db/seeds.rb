User.create!(name:  "Sailesh",
             email: "saileshsp@tcs.com",
             password:              "saileshsp",
             password_confirmation: "saileshsp",
             admin: true)

99.times do |n|
  name  = Faker::Name.name
  email = "saileshsp-#{n+1}@railstutorial.org"
  password = "password"
  User.create!(name:  name,
               email: email,
               password:              password,
               password_confirmation: password)
end