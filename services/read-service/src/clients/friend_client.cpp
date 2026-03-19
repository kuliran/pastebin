#include "clients/friend_client.hpp"

using namespace friends;

namespace read_service {

bool FriendClient::IsFriend(std::string user_id, std::string friend_id) const {
    IsFriendRequest request;
    request.set_user_id(std::move(user_id));
    request.set_friend_id(std::move(friend_id));
 
    userver::ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(std::chrono::seconds{10});

    IsFriendResponse response = raw_client_.IsFriend(request, std::move(call_options));
    return response.is_friend();
}

}