#include "clients/friends_client.hpp"

using namespace friends;

namespace read_service {

bool FriendsClient::AreFriends(std::string user_id, std::string friend_id) const {
    AreFriendsRequest request;
    request.set_user_id(std::move(user_id));
    request.set_friend_id(std::move(friend_id));
 
    userver::ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(std::chrono::seconds{10});

    AreFriendsResponse response = raw_client_.AreFriends(request, std::move(call_options));
    return response.are_friends();
}

}