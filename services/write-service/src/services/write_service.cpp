/*
When a user wants to upload:
    - User requests /create-url    
        - Generate a random paste_id (cheap)
        - Check user's rate limit, increment rate count, create a new paste with 'pending' status in the metadata DB
        - CreatePresignedPut url, return the url to user
    - User uploads directly to S3
    - User requests /submit
        - Get the latest VersionId from S3
        - Perform trivial validations (must be quick, otherwise we'd have to InvalidatePresignedUrl before to prevent TOCTOU)
        - Save VersionId and status 'submitted' to the metadata DB
        - Respond OK to user
*/

#include "services/write_service.hpp"
#include "utils/id_gen.hpp"

#include <userver/components/component.hpp>
#include <userver/utils/uuid4.hpp>
#include <userver/tracing/span.hpp>

using namespace userver;
using namespace write_service::dto;

namespace write_service {

WriteService::WriteService(const components::ComponentConfig& config, const components::ComponentContext& component_context)
    : components::LoggableComponentBase(config, component_context)
    , metadata_repo_(component_context.FindComponent<MetadataRepo>(MetadataRepo::kName))
    , blob_repo_(component_context.FindComponent<BlobRepo>(BlobRepo::kName))
    , cache_purger_(component_context.FindComponentOptional<CachePurger>(CachePurger::kName))
{}

utils::expected<CreateUploadPresignedUrlResult, CreateUploadPresignedUrlError> WriteService::CreateUploadPresignedUrl(
    std::string user_id, UploadPasteLifetime lifetime) const {
    auto expires_in = ToDuration(lifetime);
    if (!expires_in)
        return {CreateUploadPresignedUrlError::kInvalidLifetimeParam};

    auto now = std::chrono::system_clock::now();
    auto expires_at = now + *expires_in;
    PasteMetadata metadata{
        .id = {}, // is set below
        .owner_user_id = std::move(user_id),
        .created_at = storages::postgres::TimePointTz(now),
        .expires_at = storages::postgres::TimePointTz(expires_at),
        .size_bytes = 0, // is set when submit validations are done
    };

    for (int i = 0; i <= kIdCollisionRetries; ++i) {
        std::string paste_id = id_gen::GenId();
        tracing::Span::CurrentSpan().AddTag("paste_id", paste_id); // distributed tracing

        auto presigned_url = blob_repo_.CreatePresignedPut(paste_id, kPresignedPutUrlTtl);

        metadata.id = std::move(paste_id);
        auto metadata_err = metadata_repo_.CreatePendingUpload(metadata);
        if (metadata_err) {
            switch (*metadata_err) {
                case CreatePendingUploadError::kUserRateLimitExceeded:
                    return {CreateUploadPresignedUrlError::kUserRateLimitExceeded};
                case CreatePendingUploadError::kIdCollision:
                    continue;
                default:
                    return {CreateUploadPresignedUrlError::kDbError};
            }
        }

        return dto::CreateUploadPresignedUrlResult{
            .presigned_url = std::move(presigned_url)
        };
    }
    
    LOG_WARNING() << "Upload id gen exceeded max number of retries";
    return {CreateUploadPresignedUrlError::kIdCollisionRetryExceeded};
}

userver::utils::expected<SubmitUploadResult, SubmitUploadError>
    WriteService::SubmitUpload(std::string paste_id, std::string_view user_id) const {

    auto blob_s3_meta = userver::utils::Async("get_blob_metadata", [this, &paste_id] {
        return blob_repo_.GetPendingBlobMetadataBlocking(paste_id);
    }).Get();
    if (!blob_s3_meta) {
        switch (blob_s3_meta.error()) {
            case GetPendingBlobMetadataError::kNotFound: {
                LOG_DEBUG() << "GetPendingBlobMetadataError NotFound paste_id=" << paste_id << " user_id=" << user_id;
                return {SubmitUploadError::kBlobNotExists};
            }
            default: return {SubmitUploadError::kDbError};
        }
    }
    if (blob_s3_meta.value().size_bytes == 0) 
        return {SubmitUploadError::kBadBlobContent};
    if (blob_s3_meta.value().size_bytes > kMaxBlobSizeBytes)
        return {SubmitUploadError::kBlobTooLarge};

    auto blob_submit = userver::utils::Async("submit_blob", [this, &paste_id, &version_id = blob_s3_meta.value().version_id] {
        return blob_repo_.SubmitBlobBlocking(paste_id, std::move(version_id));
    }).Get();
    if (!blob_submit) {
        switch (blob_submit.error()) {
            case SubmitBlobError::kNotFound: {
                LOG_DEBUG() << "SubmitBlobError NotFound paste_id=" << paste_id << " user_id=" << user_id
                    << " version_id=" << blob_s3_meta.value().version_id;
                return {SubmitUploadError::kBlobNotExists};
            }
            default: return {SubmitUploadError::kDbError};
        }
    }

    auto metadata_err = metadata_repo_.SubmitUpload(
        std::move(paste_id),
        std::move(blob_submit.value().version_id),
        user_id,
        blob_s3_meta.value().size_bytes
    );
    if (metadata_err) {
        switch (*metadata_err) {
            case SubmitUploadMetadataError::kConflict: {
                LOG_DEBUG() << "SubmitUploadMetadataError Conflict paste_id=" << paste_id << " user_id=" << user_id;
                return {SubmitUploadError::kConflict};
            }
            default: return {SubmitUploadError::kDbError};
        }
    }

    return SubmitUploadResult{};
}

utils::expected<DeletePasteResult, DeletePasteError> WriteService::DeletePaste(const std::string_view& id,
    const std::string_view& user_id) const {
    if (id.empty() || id.size() > 128)
        return {DeletePasteError::kInvalidId};

    auto metadata_err = metadata_repo_.DeletePasteMetadata(id, user_id);
    if (metadata_err) {
        switch (*metadata_err) {
            case DeletePasteMetadataError::kUnauthorized: return {DeletePasteError::kUnauthorized};
            case DeletePasteMetadataError::kAlreadySoftDeleted: return {DeletePasteError::kAlreadySoftDeleted};
            case DeletePasteMetadataError::kNotExists: return {DeletePasteError::kNotExists};
            default: return {DeletePasteError::kDbError};
        }
    }

    // Background nginx cache purging
    if (cache_purger_) {
        background_tasks_.AsyncDetach(
            "cache_purge",
            [cache_purger = cache_purger_,
                id = std::string(id)]() {
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