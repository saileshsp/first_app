class Photo < MockRecord
  attr_accessor :caption, :lat, :lng, :size, :average_rating, :created_at, :post_id, :photo_container_id
end

Sunspot.setup(Photo) do
  text :caption, :default_boost => 1.5
  string :caption
  integer :photo_container_id
  boost 0.75
  integer :size, :trie => true
  float :average_rating, :trie => true
  time :created_at, :trie => true
end

class PhotoContainer < MockRecord
  def id
    1
  end
end

Sunspot.setup(PhotoContainer) do
  join(:caption, :type => :string, :join_string => 'from=photo_container_id to=id')
  join(:photo_rating, :type => :trie_float, :join_string => 'from=photo_container_id to=id', :as => 'average_rating_ft')
end