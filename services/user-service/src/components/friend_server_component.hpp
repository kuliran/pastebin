#pragma once

#include "handlers/grpc/friend_server.hpp"

#include <userver/ugrpc/client/simple_client_component.hpp>

#include <friends_service.usrv.pb.hpp>

namespace user_service {

class FriendServerComponent final : public userver::ugrpc::server::ServiceComponentBase {
public:
    static constexpr std::string_view kName = "grpc-friend-server";

    FriendServerComponent(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context);

private:
    FriendServer server_;
};

}