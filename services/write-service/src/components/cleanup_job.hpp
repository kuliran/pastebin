#pragma once

#include "components/blob_repo.hpp"

#include <userver/components/component_base.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/testsuite/testsuite_support.hpp>

namespace write_service {

class CleanupJob final : public userver::components::ComponentBase {
public:
    static constexpr std::string_view kName = "cleanup-job";

    CleanupJob(const userver::components::ComponentConfig&,
               const userver::components::ComponentContext&);

    ~CleanupJob() override;

    static userver::yaml_config::Schema GetStaticConfigSchema();

private:
    inline static const std::string kTaskName = "cleanup_job";

    void Run();
    void CleanupDeleted();
    void CleanupExpired();

    BlobRepo& blob_repo_;
    userver::storages::postgres::ClusterPtr pg_;
    userver::utils::PeriodicTask task_;
    std::optional<userver::testsuite::PeriodicTaskRegistrationHolder> periodic_task_holder_;
    int32_t batch_size_expired_;
    int32_t batch_size_deleted_;
};

}