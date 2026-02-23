#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/storages/secdist/provider_component.hpp>
#include <userver/storages/postgres/component.hpp>

#include <userver/utils/daemon_run.hpp>

#include "jwt/auth_checker_http.hpp"
#include "services/auth_service.hpp"
#include "components/auth_repo.hpp"
#include "components/cookie_factory.hpp"
#include "handlers/signup.hpp"
#include "handlers/login.hpp"
#include "handlers/refresh.hpp"

int main(int argc, char* argv[]) {
    userver::server::handlers::auth::RegisterAuthCheckerFactory<jwt_wrapper::JwtCheckerFactory>();
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<userver::server::handlers::Ping>()
            .Append<userver::components::TestsuiteSupport>()
            .AppendComponentList(userver::clients::http::ComponentList())
            .Append<userver::clients::dns::Component>()
            .Append<userver::server::handlers::TestsControl>()
            .Append<userver::congestion_control::Component>()
            .Append<userver::components::Postgres>("postgres-db-1")

            .Append<jwt_wrapper::JwtVerifierComponent>()
            .Append<user_service::AuthService>()
            .Append<user_service::AuthRepo>()
            .Append<user_service::CookieFactory>()
            .Append<user_service::Signup>()
            .Append<user_service::Login>()
            .Append<user_service::Refresh>()
        ;

    return userver::utils::DaemonMain(argc, argv, component_list);
}