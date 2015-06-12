module UsersHelper
  

  # Returns the Gravatar for the given user.
   def gravatar_for(user)
    gravatar_id = Digest::MD5::hexdigest(user.email.downcase)
    #gravatar_url = "https://secure.gravatar.com/avatar/#{gravatar_id}"
    image_tag(current_user.avatar(:thumb), alt: user.name, class: "gravatar")
   end
end

