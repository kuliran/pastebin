#include "handlers/grpc/friend_server.hpp"

#include <userver/ugrpc/server/exceptions.hpp>

using namespace userver;

namespace user_service {

FriendServer::AreFriendsResult FriendServer::AreFriends(CallContext&, friends::AreFriendsRequest&& request) {
    friends::AreFriendsResponse response;

    auto result = friend_service_.AreFriends(request.user_id(), request.friend_id());
    if (!result) {
        switch (result.error()) {
            case dto::AreFriendsError::kDbError:
                throw ugrpc::server::ErrorWithStatus(grpc::StatusCode::INTERNAL, "Internal DB error");
        }
    }

    response.set_are_friends(result.value());
    return response;
}

}