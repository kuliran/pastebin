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
#include <userver/storages/mongo/component.hpp>    
#include <userver/storages/postgres/component.hpp> 

#include <userver/utils/daemon_run.hpp>

#include "services/paste_service.hpp"
#include "components/metadata_repo.hpp"
#include "components/blob_repo.hpp"
#include "components/cache_purger.hpp"
#include "handlers/get_paste.hpp"
#include "handlers/upload_paste.hpp"
#include "handlers/delete_paste.hpp"

int main(int argc, char* argv[]) {
    auto component_list =
        userver::components::MinimalServerComponentList()
            .Append<userver::server::handlers::Ping>()
            .Append<userver::components::TestsuiteSupport>()
            .AppendComponentList(userver::clients::http::ComponentList())
            .Append<userver::clients::dns::Component>()
            .Append<userver::server::handlers::TestsControl>()
            .Append<userver::congestion_control::Component>()
            .Append<userver::components::Postgres>("postgres-db-1")
            .Append<userver::components::Mongo>("mongo-db-1")

            .Append<paste_service::PasteService>()
            .Append<paste_service::MetadataRepo>()
            .Append<paste_service::CachePurger>()
            .Append<paste_service::BlobRepo>()
            .Append<paste_service::GetPaste>()
            .Append<paste_service::UploadPaste>()
            .Append<paste_service::DeletePaste>()
        ;

    return userver::utils::DaemonMain(argc, argv, component_list);
}