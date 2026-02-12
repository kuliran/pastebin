#include "repo/blob_repo.hpp"

#include <userver/components/component.hpp>
#include <userver/storages/mongo/component.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>

using namespace userver;

namespace paste_service {

BlobRepo::BlobRepo(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : mongo_pool_(component_context.FindComponent<components::Mongo>(
        config["mongo-component"].As<std::string>(kDefaultMongoComponent)
    ).GetPool())
{}

utils::expected<PasteBlob, GetPasteBlobError> BlobRepo::GetPasteBlob(const std::string_view& id) const {
    using formats::bson::MakeDoc;

    try {
        const auto blob_collection = mongo_pool_->GetCollection("pastes");
        const auto mongo_result = blob_collection.FindOne(MakeDoc("_id", id));
        if (!mongo_result)
            return {GetPasteBlobError::kNotFound};

        try {
            return mongo_result->As<PasteBlob>();
        } catch (const std::exception& e) {
            LOG_ERROR() << "Failed to parse PasteBlob: " << e.what();
            return {GetPasteBlobError::kInvalidData};
        }
    } catch (const storages::mongo::MongoException& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {GetPasteBlobError::kDbError};
    }
}

std::optional<UploadPasteBlobError> BlobRepo::UploadPasteBlob(const PasteBlob& blob) const {
    using formats::bson::MakeDoc;

    try {
        auto blob_collection = mongo_pool_->GetCollection("pastes");
        auto blob_doc = formats::bson::ValueBuilder(blob).ExtractValue();

        auto result = blob_collection.UpdateOne(
            MakeDoc("_id", blob.id),
            MakeDoc("$setOnInsert", blob_doc),
            storages::mongo::options::Upsert{}
        );

        if (result.UpsertedCount() == 0)
            return {UploadPasteBlobError::kIdCollision};
    } catch (const storages::mongo::MongoException& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {UploadPasteBlobError::kDbError};
    } catch (const std::exception& e) {
        LOG_ERROR() << "Failed to serialize PasteBlob: " << e.what();
        return {UploadPasteBlobError::kInvalidData};
    }
    return std::nullopt;
}

std::optional<DeletePasteBlobError> BlobRepo::DeletePasteBlob(const std::string_view& id) const {
    using formats::bson::MakeDoc;

    try {
        auto blob_collection = mongo_pool_->GetCollection("pastes");
        auto result = blob_collection.DeleteOne(
            MakeDoc("_id", id)
        );

        if (result.DeletedCount() == 0)
            return {DeletePasteBlobError::kNotExists};
    } catch (const storages::mongo::MongoException& e) {
        LOG_ERROR() << "DB error: " << e.what();
        return {DeletePasteBlobError::kDbError};
    }
    return std::nullopt;
}

}