#pragma once

#include "repo/paste_blob.hpp"

#include <userver/utils/expected.hpp>
#include <userver/storages/mongo/pool.hpp>

namespace paste_service {

enum class GetPasteBlobError {
    kNotFound,
    kDbError,
    kInvalidData,
};

class BlobRepo {
public:
    BlobRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<PasteBlob, GetPasteBlobError> GetPasteBlob(const std::string_view& id) const;
private:
    static constexpr std::string_view kDefaultMongoComponent = "mongo-db-1";

    userver::storages::mongo::PoolPtr mongo_pool_;
};
    
}