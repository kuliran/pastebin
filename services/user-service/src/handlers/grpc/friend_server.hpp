#pragma once

#include "services/friend_service.hpp"

#include <friends_service.usrv.pb.hpp>

namespace user_service {

class FriendServer final : public friends::FriendsServiceBase {
public:
    explicit FriendServer(FriendService& friend_service)
        : friend_service_(friend_service) {}

    IsFriendResult IsFriend(CallContext&, friends::IsFriendRequest&&);

private:
    FriendService& friend_service_;
};

}