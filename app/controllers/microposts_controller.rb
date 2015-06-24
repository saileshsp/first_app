class MicropostsController < ApplicationController
before_action :logged_in_user, only: [:create, :destroy]
before_action :correct_user,   only: :destroy
 def create
    @micropost = current_user.microposts.build(micropost_params)
    if @micropost.save
      flash[:success] = "Micropost created!"
      redirect_to root_url
    else
      @feed_items = []
      render 'static_pages/home'
    end
  end
 def index
  @microposts = Micropost.all
 end
 def upvote
  @micropost = Micropost.find(params[:id])
  @micropost.upvote_by current_user
  redirect_to  root_path
end
def downvote
  @micropost = Link.find(params[:id])
  @micropost.downvote_by current_user
  redirect_to  root_path
end
  def score
  self.get_upvotes.size - self.get_downvotes.size
  end
  def destroy
     @micropost.destroy
    flash[:success] = "Micropost deleted"
    redirect_to request.referrer || root_url

  end
 private

  def micropost_params
    params.require(:micropost).permit(:content, :picture)
  end
  def correct_user
      @micropost = current_user.microposts.find_by(id: params[:id])
      redirect_to root_url if @micropost.nil?
    end
end
