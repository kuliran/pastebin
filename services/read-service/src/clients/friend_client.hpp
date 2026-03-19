#pragma once

#include <friends_client.usrv.pb.hpp>

namespace read_service {

class FriendClient final {
public:
    explicit FriendClient(friends::FriendsServiceClient& raw_client)
        : raw_client_(raw_client) {};

    bool IsFriend(std::string user_id, std::string friend_id) const;

private:
    friends::FriendsServiceClient& raw_client_;
};

}