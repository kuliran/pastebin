/*
When a user wants to upload:
    - User requests /create-url    
        - Generate a random paste_id (cheap)
        - Run a DB transaction:
            - Check user's rate limit, increment rate count, create a new paste with 'pending' status in the metadata DB
            - Apply user-provided privacy settings (rollback if input is invalid)
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
{}

utils::expected<CreateUploadPresignedUrlResult, CreateUploadPresignedUrlError> WriteService::CreateUploadPresignedUrl(
    std::string user_id, UploadPasteLifetime lifetime, PastePrivacySettings privacy) const
{
    auto expires_in = ToDuration(lifetime);
    if (!expires_in)
        return {CreateUploadPresignedUrlError::kInvalidLifetimeParam};

    PasteVisibility visibility = privacy.visibility ? *privacy.visibility : PasteVisibility::kPublic;

    auto now = std::chrono::system_clock::now();
    auto expires_at = now + *expires_in;
    PasteMetadata metadata{
        .id = {}, // is set below
        .owner_user_id = std::move(user_id),
        .created_at = storages::postgres::TimePointTz(now),
        .expires_at = storages::postgres::TimePointTz(expires_at),
        .visibility = visibility,
        .size_bytes = 0, // is set when submit validations are done
    };

    for (int i = 0; i <= kIdCollisionRetries; ++i) {
        metadata.id = id_gen::GenId();
        tracing::Span::CurrentSpan().AddTag("paste_id", metadata.id); // distributed tracing

        auto unit = metadata_repo_.BeginUnit();

        auto metadata_err = metadata_repo_.CreatePendingUpload(unit, metadata);
        if (metadata_err) {
            switch (*metadata_err) {
                case CreatePendingUploadError::kUserRateLimitExceeded:
                    return {CreateUploadPresignedUrlError::kUserRateLimitExceeded};
                case CreatePendingUploadError::kIdCollision: continue;
                case CreatePendingUploadError::kDbError: return {CreateUploadPresignedUrlError::kDbError};
            }
        }

        if (visibility == PasteVisibility::kPrivate && privacy.private_perms_add && !privacy.private_perms_add->empty()) {
            auto err = metadata_repo_.PastePrivatePermsAdd(unit, metadata.id , std::move(*privacy.private_perms_add));
            if (err) {
                switch (*err) {
                    case PastePrivatePermsAddRepoError::kDbError: return {CreateUploadPresignedUrlError::kDbError};
                }
            }
        }

        auto presigned_url = blob_repo_.CreatePresignedPut(metadata.id, kPresignedPutUrlTtl);
        unit.Commit();

        return dto::CreateUploadPresignedUrlResult{
            .presigned_url = std::move(presigned_url)
        };
    }
    
    LOG_WARNING() << "Upload id gen exceeded max number of retries";
    return {CreateUploadPresignedUrlError::kIdCollisionRetryExceeded};
}

userver::utils::expected<SubmitUploadResult, SubmitUploadError>
WriteService::SubmitUpload(std::string paste_id, std::string_view user_id) const
{
    auto blob_s3_meta = userver::utils::Async("get_blob_metadata", [this, &paste_id] {
        return blob_repo_.GetPendingBlobMetadataBlocking(paste_id);
    }).Get();
    if (!blob_s3_meta) {
        switch (blob_s3_meta.error()) {
            case GetPendingBlobMetadataError::kNotFound:
                LOG_DEBUG() << "GetPendingBlobMetadataError NotFound paste_id=" << paste_id << " user_id=" << user_id;
                return {SubmitUploadError::kBlobNotExists};
            case GetPendingBlobMetadataError::kDbError: return {SubmitUploadError::kDbError};
        }
    }

    if (blob_s3_meta.value().size_bytes == 0) return {SubmitUploadError::kBadBlobContent};
    if (blob_s3_meta.value().size_bytes > kMaxBlobSizeBytes) return {SubmitUploadError::kBlobTooLarge};

    auto blob_submit = userver::utils::Async("submit_blob", [this, &paste_id, &version_id = blob_s3_meta.value().version_id] {
        return blob_repo_.SubmitBlobBlocking(paste_id, std::move(version_id));
    }).Get();
    if (!blob_submit) {
        switch (blob_submit.error()) {
            case SubmitBlobError::kNotFound:
                LOG_DEBUG()
                    << "SubmitBlobError NotFound paste_id=" << paste_id << " user_id=" << user_id
                    << " version_id=" << blob_s3_meta.value().version_id;
                return {SubmitUploadError::kBlobNotExists};
            case SubmitBlobError::kDbError: return {SubmitUploadError::kDbError};
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
            case SubmitUploadMetadataError::kConflict:
                LOG_DEBUG() << "SubmitUploadMetadataError Conflict paste_id=" << paste_id << " user_id=" << user_id;
                return {SubmitUploadError::kConflict};
            case SubmitUploadMetadataError::kDbError: return {SubmitUploadError::kDbError};
        }
    }

    return SubmitUploadResult{};
}

utils::expected<DeletePasteResult, DeletePasteError>
WriteService::DeletePaste(std::string_view paste_id, std::string_view user_id) const
{
    auto is_owner_err = metadata_repo_.IsPasteOwner(paste_id, user_id);
    if (is_owner_err) {
        switch (*is_owner_err) {
            case IsPasteOwnerError::kNotExists: return {DeletePasteError::kNotExists};
            case IsPasteOwnerError::kNotOwner: return {DeletePasteError::kUnauthorized};
            case IsPasteOwnerError::kDbError: return {DeletePasteError::kDbError};
        }
    }

    auto metadata_err = metadata_repo_.DeletePasteMetadata(paste_id);
    if (metadata_err) {
        switch (*metadata_err) {
            case DeletePasteMetadataError::kAlreadySoftDeleted: return {DeletePasteError::kAlreadySoftDeleted};
            case DeletePasteMetadataError::kDbError: return {DeletePasteError::kDbError};
        }
    }

    return dto::DeletePasteResult();
}

userver::utils::expected<PatchPastePrivacyResult, PatchPastePrivacyError>
WriteService::PatchPastePrivacy(
    std::string_view paste_id, std::string_view user_id, PastePrivacySettings privacy) const
{
    if (!privacy.visibility
        && (!privacy.private_perms_add || privacy.private_perms_add->empty())
        && (!privacy.private_perms_rm || privacy.private_perms_rm->empty())) {
        return {PatchPastePrivacyError::kEmptyRequest};
    }

    auto is_owner_err = metadata_repo_.IsPasteOwner(paste_id, user_id);
    if (is_owner_err) {
        switch (*is_owner_err) {
            case IsPasteOwnerError::kNotExists: return {PatchPastePrivacyError::kNotExists};
            case IsPasteOwnerError::kNotOwner: return {PatchPastePrivacyError::kUnauthorized};
            case IsPasteOwnerError::kDbError: return {PatchPastePrivacyError::kDbError};
        }
    }

    auto unit = metadata_repo_.BeginUnit();
    if (privacy.visibility) {
        auto patch_err = metadata_repo_.PatchPasteVisibility(unit, paste_id, *privacy.visibility);
        if (patch_err) {
            switch (*patch_err) {
                case PatchPasteVisibilityRepoError::kNotExists: return {PatchPastePrivacyError::kNotExists};
                case PatchPasteVisibilityRepoError::kDbError: return {PatchPastePrivacyError::kDbError};
            }
        }
    }

    if (!privacy.visibility || *privacy.visibility == PasteVisibility::kPrivate) {
        if (privacy.private_perms_rm && !privacy.private_perms_rm->empty()) {
            auto patch_err = metadata_repo_.PastePrivatePermsRm(unit, paste_id, std::move(*privacy.private_perms_rm));
            if (patch_err) {
                switch (*patch_err) {
                    case PastePrivatePermsAddRepoError::kDbError: return {PatchPastePrivacyError::kDbError};
                }
            }
        }
        if (privacy.private_perms_add && !privacy.private_perms_add->empty()) {
            auto patch_err = metadata_repo_.PastePrivatePermsAdd(unit, paste_id, std::move(*privacy.private_perms_add));
            if (patch_err) {
                switch (*patch_err) {
                    case PastePrivatePermsAddRepoError::kDbError: return {PatchPastePrivacyError::kDbError};
                }
            }
        }
    }

    unit.Commit();
    return dto::PatchPastePrivacyResult{};
}

}