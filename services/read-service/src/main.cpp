#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component_list.hpp>
#include <userver/components/component.hpp>
#include <userver/components/component_list.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/congestion_control/component.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/storages/postgres/component.hpp> 

#include <userver/utils/daemon_run.hpp>

#include "jwt/auth_checker_http.hpp"
#include "aws/aws_sdk_component.hpp"
#include "services/read_service.hpp"
#include "components/metadata_repo.hpp"
#include "components/blob_repo.hpp"
#include "handlers/get_paste.hpp"
#include "handlers/get_paste_details.hpp"
#include "handlers/get_my_pastes.hpp"

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
            .Append<Aws::AwsSdkComponent>()
            
            .Append<read_service::ReadService>()
            .Append<read_service::MetadataRepo>()
            .Append<read_service::BlobRepo>()
            .Append<read_service::GetPaste>()
            .Append<read_service::GetPasteDetails>()
            .Append<read_service::GetMyPastes>()
        ;

    return userver::utils::DaemonMain(argc, argv, component_list);
}