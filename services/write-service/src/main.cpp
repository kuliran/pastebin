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
#include <userver/server/handlers/auth/auth_checker_factory.hpp>

#include "jwt/auth_checker_http.hpp"
#include "services/write_service.hpp"
#include "aws/aws_sdk_component.hpp"
#include "components/metadata_repo.hpp"
#include "components/blob_repo.hpp"
#include "components/cleanup_job.hpp"
#include "handlers/upload_create_url.hpp"
#include "handlers/upload_submit.hpp"
#include "handlers/delete_paste.hpp"
#include "handlers/patch_paste.hpp"

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
            
            .Append<write_service::WriteService>()
            .Append<write_service::MetadataRepo>()
            .Append<write_service::BlobRepo>()
            .Append<write_service::CleanupJob>()
            .Append<write_service::UploadCreateUrl>()
            .Append<write_service::UploadSubmit>()
            .Append<write_service::DeletePaste>()
            .Append<write_service::PatchPaste>()
        ;

    return userver::utils::DaemonMain(argc, argv, component_list);
}