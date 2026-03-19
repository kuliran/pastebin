#include "components/friend_server_component.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/postgres/component.hpp>

using namespace userver;

namespace user_service {

FriendServerComponent::FriendServerComponent(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : ugrpc::server::ServiceComponentBase(config, component_context)
    , server_(component_context.FindComponent<FriendService>(FriendService::kName))
{
    RegisterService(server_);
}

}