#pragma once

#include "clients/friend_client.hpp"

#include <userver/ugrpc/client/simple_client_component.hpp>

#include <friends_service.usrv.pb.hpp>

namespace read_service {

class FriendClientComponent final : public userver::ugrpc::client::SimpleClientComponent<friends::FriendsServiceClient> {
public:
    static constexpr std::string_view kName = "grpc-friend-client";

    using Base = userver::ugrpc::client::SimpleClientComponent<friends::FriendsServiceClient>;

    FriendClientComponent(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context)
        : Base(config, context),
          client_wrapper_(GetClient())
    {}

    using Base::GetClient;

    FriendClient& GetClientWrapper() noexcept { return client_wrapper_; }

private:
    FriendClient client_wrapper_;
};

}