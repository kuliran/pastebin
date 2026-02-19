#include "services/paste_service.hpp"
#include "utils/id_gen.hpp"

#include <userver/components/component.hpp>
#include <userver/utils/uuid4.hpp>
#include <userver/tracing/span.hpp>

using namespace userver;
using namespace paste_service::dto;

namespace paste_service {

PasteService::PasteService(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , metadata_repo_(component_context.FindComponent<MetadataRepo>(MetadataRepo::kName))
    , blob_repo_(component_context.FindComponent<BlobRepo>(BlobRepo::kName))
    , cache_purger_(component_context.FindComponentOptional<CachePurger>(CachePurger::kName))
{}

utils::expected<GetPasteResult, GetPasteError> PasteService::GetPaste(const std::string_view& id) const {
    const auto metadata = metadata_repo_.GetPasteMetadata(id);
    if (!metadata) {
        switch (metadata.error()) {
            case GetPasteMetadataError::kNotFound:
                return {GetPasteError::kNotExists};
            case GetPasteMetadataError::kSoftExpired:
                return {GetPasteError::kSoftExpired};
            default:
                return {GetPasteError::kDbError};
        }
    }

    const auto blob = blob_repo_.GetPasteBlob(id);
    if (!blob) {
        switch (blob.error()) {
            case paste_service::GetPasteBlobError::kNotFound: {
                LOG_WARNING() << "Paste metadata exists, but no blob; paste_id=" << id;
                return {GetPasteError::kNotExists};
            }
            default: {
                return {GetPasteError::kDbError};
            }
        }
    }
    return GetPasteResult(std::move(metadata.value()), std::move(blob.value()));
}

utils::expected<UploadPasteResult, UploadPasteError> PasteService::UploadPaste(std::string text, UploadPasteLifetime lifetime) const {
    if (text.empty())
        return {UploadPasteError::kEmptyText};
    if (text.size() > kMaxBlobSizeBytes)
        return {UploadPasteError::kTextTooLarge};

    auto expires_in = ToDuration(lifetime);
    if (!expires_in)
        return {UploadPasteError::kInvalidLifetimeParam};

    std::string delete_key = utils::generators::GenerateUuid();
    auto now = std::chrono::system_clock::now();
    auto expires_at = now + *expires_in;

    PasteBlob blob{{}, std::move(text), expires_at};
    PasteMetadata metadata{
        .id = {}, // is set below
        .created_at = storages::postgres::TimePointTz(now),
        .expires_at = storages::postgres::TimePointTz(expires_at),
        .delete_key = std::move(delete_key),
        .size_bytes = static_cast<int>(blob.text.size()),
    };

    for (int i = 0; i <= kIdCollisionRetries; ++i) {
        blob.id = id_gen::GenId();
        tracing::Span::CurrentSpan().AddTag("paste_id", blob.id); // distributed tracing

        auto blob_err = blob_repo_.UploadPasteBlob(blob);
        if (blob_err) {
            if (*blob_err == UploadPasteBlobError::kIdCollision)
                continue;

            return {UploadPasteError::kDbError};
        }

        metadata.id = std::move(blob.id);
        auto metadata_err = metadata_repo_.UploadPasteMetadata(metadata);
        if (metadata_err) {
            if (*metadata_err == UploadPasteMetadataError::kIdCollision)
                continue;

            return {UploadPasteError::kDbError};
        }

        return dto::UploadPasteResult(std::move(metadata));
    }
    
    LOG_WARNING() << "Upload id gen exceeded max number of retries";
    return {UploadPasteError::kIdCollisionRetryExceeded};
}

utils::expected<DeletePasteResult, DeletePasteError> PasteService::DeletePaste(const std::string_view& id, const std::string_view& delete_key) const {
    if (id.empty() || id.size() > 128)
        return {DeletePasteError::kInvalidId};
    if (delete_key.empty() || delete_key.size() > 128)
        return {DeletePasteError::kInvalidDeleteKey};

    auto metadata_err = metadata_repo_.DeletePasteMetadata(id, delete_key);
    if (metadata_err) {
        switch (*metadata_err) {
            case DeletePasteMetadataError::kNotExists:
                return {DeletePasteError::kNotExists};
            default: {
                return {DeletePasteError::kDbError};
            }
        }
    }

    // Background orphan blob cleanup
    background_tasks_.AsyncDetach(
        "blob_cleanup",
        [&blob_repo = blob_repo_, // passing repo by ref - it's a component with lifetime of the whole process
            id = std::move(id)]() {
            try {
                blob_repo.DeletePasteBlob(id);
            } catch (const engine::TaskCancelledException&) {
                LOG_WARNING() << "Blob cleanup cancelled during shutdown; paste_id=" << id;
            }
        }
    );

    // Background nginx cache purging
    if (cache_purger_) {
        background_tasks_.AsyncDetach(
            "cache_purge",
            [cache_purger = cache_purger_,
                id = std::move(id)]() {
                try {
                    cache_purger->PurgePaste(id);
                } catch (const engine::TaskCancelledException&) {
                    LOG_WARNING() << "Cache purging cancelled during shutdown; paste_id=" << id;
                }
            }
        );
    }

    return dto::DeletePasteResult();
}

}