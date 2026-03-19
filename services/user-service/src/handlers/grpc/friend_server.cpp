#include "handlers/grpc/friend_server.hpp"

#include <userver/ugrpc/server/exceptions.hpp>

using namespace userver;

namespace user_service {

FriendServer::IsFriendResult FriendServer::IsFriend(CallContext&, friends::IsFriendRequest&& request) {
    friends::IsFriendResponse response;

    auto result = friend_service_.IsFriend(request.user_id(), request.friend_id());
    if (!result) {
        switch (result.error()) {
            case dto::IsFriendError::kDbError:
                throw ugrpc::server::ErrorWithStatus(grpc::StatusCode::INTERNAL, "Internal DB error");
        }
    }

    response.set_is_friend(result.value());
    return response;
}

}