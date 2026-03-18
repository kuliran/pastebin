#pragma once

#include "clients/friends_client.hpp"

#include <userver/ugrpc/client/simple_client_component.hpp>

#include <friends_service.usrv.pb.hpp>

namespace read_service {

class FriendsClientComponent final : public userver::ugrpc::client::SimpleClientComponent<friends::FriendsServiceClient> {
public:
    static constexpr std::string_view kName = "grpc-friends-client";

    using Base = userver::ugrpc::client::SimpleClientComponent<friends::FriendsServiceClient>;

    FriendsClientComponent(const userver::components::ComponentConfig& config, const userver::components::ComponentContext& context)
        : Base(config, context),
          client_wrapper_(GetClient())
    {}

    using Base::GetClient;

    FriendsClient& GetClientWrapper() noexcept { return client_wrapper_; }

private:
    FriendsClient client_wrapper_;
};

}