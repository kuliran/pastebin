import pytest
import requests
from shared.utils.client import Client
from shared.utils.upload_paste import *

@pytest.fixture
async def api_upload_create_url_raw(endpoints, auth_client):
    async def impl(*,
        expires_in: str = None,
        visibility: str = None,
        private_perms_add: list[str] = None,
        client: Client = auth_client
    ):
        request_json = {}
        if expires_in is not None: request_json["expires_in"] = expires_in
        if visibility is not None: request_json["visibility"] = visibility
        if private_perms_add is not None: request_json["private_perms_add"] = private_perms_add

        return await client.post(endpoints['upload_paste_create_url'](), json=request_json)
    return impl

@pytest.fixture(scope='session')
async def s3_upload():
    async def impl(presigned_url: str, text: str) -> UploadS3Result:
        data = text.encode("utf-8")

        upload_response = requests.put(
            presigned_url,
            data=data,
            headers={"Content-Type": "application/octet-stream"},
        )
        assert upload_response.status_code == 200

        version_id = upload_response.headers.get("x-amz-version-id")
        assert version_id

        return UploadS3Result(
            data=data,
            version_id=version_id,
            size_bytes=len(data),
        )
    return impl

@pytest.fixture
async def api_upload_submit_raw(endpoints, auth_client):
    async def impl(paste_id: str, *, client: Client = auth_client):
        request_json = {}
        request_json["paste_id"] = paste_id
        return await client.post(endpoints['upload_paste_submit'](), json=request_json)
    return impl