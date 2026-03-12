import pytest
from shared.utils.upload_paste import *
from shared.utils.client import Client

@pytest.fixture
def api_upload_paste(api_upload_create_url_raw, s3_upload, api_upload_submit_raw, auth_client):
    async def _impl(paste_text: str, *,
            expires_in: str = None,
            visibility: str = None,
            private_perms_add: list[str] = None,
            client: Client = auth_client
        ) -> UploadPasteResult:
        create_url = await api_upload_create_url_raw(
            expires_in=expires_in, visibility=visibility,
            private_perms_add=private_perms_add, client=client
        )
        assert create_url.status == 201
        json = create_url.json()
        presigned_url = json['presigned_url']
        paste_id = get_paste_id(presigned_url)
        
        upload = await s3_upload(presigned_url, paste_text)
        submit = await api_upload_submit_raw(paste_id)
        assert submit.status == 200

        return UploadPasteResult(
            paste_id=paste_id,
            presigned_url=presigned_url,
            version_id=upload.version_id,
            data=upload.data,
            created_at_utc=None,
            expires_at_utc=None,
            size_bytes=None,
        )
    return _impl