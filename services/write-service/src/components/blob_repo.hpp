#pragma once

#include "models/paste_blob.hpp"

#include <userver/components/component_base.hpp>
#include <userver/utils/expected.hpp>
#include <userver/storages/mongo/pool.hpp>

namespace write_service {

enum class GetPasteBlobError {
    kNotFound,
    kDbError,
    kInvalidData,
};

enum class UploadPasteBlobError {
    kDbError,
    kInvalidData,
    kIdCollision,
};

enum class DeletePasteBlobError {
    kNotExists,
    kDbError,
};

class BlobRepo : public userver::components::LoggableComponentBase {
public:
    static constexpr std::string_view kName = "blob-repo";

    BlobRepo(const userver::components::ComponentConfig&, const userver::components::ComponentContext&);

    userver::utils::expected<PasteBlob, GetPasteBlobError> GetPasteBlob(const std::string_view& id) const;
    std::optional<UploadPasteBlobError> UploadPasteBlob(const PasteBlob&) const;
    std::optional<DeletePasteBlobError> DeletePasteBlob(const std::string_view& id) const;
private:
    static constexpr std::string_view kDefaultMongoComponent = "mongo-db-1";

    userver::storages::mongo::PoolPtr mongo_pool_;
};

}