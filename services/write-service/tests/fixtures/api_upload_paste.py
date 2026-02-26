import pytest
from dataclasses import dataclass
from datetime import datetime, timezone
from urllib.parse import urlparse
import shared.utils.auth as auth

@dataclass
class UploadCreateUrlResult:
    presigned_url: str
    paste_id: str
    created_at_utc: datetime
    expires_at_utc: datetime

@dataclass
class UploadS3Result:
    data: bytes
    version_id: str
    size_bytes: int

@dataclass
class UploadSubmitResult:
    created_at_utc: datetime
    expires_at_utc: datetime
    size_bytes: int

@dataclass
class UploadPasteResult:
    paste_id: str
    presigned_url: str
    version_id: str
    data: str
    created_at_utc: datetime
    expires_at_utc: datetime
    size_bytes: int


@pytest.fixture
async def api_upload_create_url(pg_cursor, auth_client, api_upload_create_url_raw):
    async def impl(expires_in: str = None, *, client: auth.Client = auth_client) -> UploadCreateUrlResult:
        now_utc = datetime.now(timezone.utc)

        response = await api_upload_create_url_raw(expires_in, client=client)
        assert response.status == 201
        assert 'application/json' in response.headers['Content-Type']

        json = response.json()
        presigned_url = json['presigned_url']
        assert type(presigned_url) is str

        parsed = urlparse(presigned_url)
        paste_id = parsed.path.lstrip("/").split("/")[-1]
        assert 1 <= len(paste_id) <= 64

        # Postgres validation
        pg_cursor.execute("""
            SELECT owner_user_id, created_at, expires_at, size_bytes, status
            FROM pastes.metadata
            WHERE id = %s
        """, (paste_id,))
        pg_owner_user_id, pg_created_at, pg_expires_at, pg_size_bytes, pg_status = pg_cursor.fetchone()
        pg_created_at = pg_created_at.astimezone(timezone.utc)
        pg_expires_at = pg_expires_at.astimezone(timezone.utc)

        lifetime_seconds = 0
        if expires_in is None or expires_in == '1_week': lifetime_seconds = 60*60*24*7
        elif expires_in == '1_hour': lifetime_seconds = 60*60
        elif expires_in == '1_day': lifetime_seconds = 60*60*24
        elif expires_in == '1_month': lifetime_seconds = 60*60*24*30
        elif expires_in == '3_month': lifetime_seconds = 60*60*24*30*3

        assert pg_status == 'pending'
        assert pg_size_bytes == 0
        assert pg_owner_user_id == client._user_id
        assert abs((pg_created_at - now_utc).total_seconds()) <= 1
        assert (pg_expires_at - pg_created_at).total_seconds() == lifetime_seconds, f"incorrect pg expires_at with param: {expires_in}"

        return UploadCreateUrlResult(
            presigned_url=presigned_url,
            paste_id=paste_id,
            created_at_utc=pg_created_at,
            expires_at_utc=pg_expires_at,
        )
    return impl

@pytest.fixture
async def api_upload_create_url_raw(pg_cursor, endpoints, auth_client):
    async def impl(expires_in: str = None, *, client: auth.Client = auth_client):
        request_json = {}
        if expires_in is not None:
            request_json["expires_in"] = expires_in
        return await client.post(endpoints['upload_paste_create_url'], json=request_json)
    return impl


@pytest.fixture
async def s3_upload(minio_server):
    async def impl(presigned_url: str, text: str) -> UploadS3Result:
        data = text.encode("utf-8")

        import requests
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
async def api_upload_submit(pg_cursor, api_upload_submit_raw, auth_client):
    async def impl(paste_id: str, *, client: auth.Client = auth_client) -> UploadSubmitResult:
        response = await api_upload_submit_raw(paste_id, client=client)
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']

        # Postgres validation
        pg_cursor.execute("""
            SELECT owner_user_id, created_at, expires_at, size_bytes, status
            FROM pastes.metadata
            WHERE id = %s
        """, (paste_id,))
        pg_owner_user_id, pg_created_at, pg_expires_at, pg_size_bytes, pg_status = pg_cursor.fetchone()
        pg_created_at = pg_created_at.astimezone(timezone.utc)
        pg_expires_at = pg_expires_at.astimezone(timezone.utc)

        assert pg_status == 'submitted'
        assert pg_owner_user_id == client._user_id

        return UploadSubmitResult(
            created_at_utc=pg_created_at,
            expires_at_utc=pg_expires_at,
            size_bytes=pg_size_bytes,
        )
    return impl

@pytest.fixture
async def api_upload_submit_raw(endpoints, auth_client):
    async def impl(paste_id: str, *, client: auth.Client = auth_client):
        request_json = {}
        request_json["paste_id"] = paste_id
        return await client.post(endpoints['upload_paste_submit'], json=request_json)
    return impl


@pytest.fixture
async def api_upload_paste(api_upload_create_url, api_upload_submit, s3_upload, auth_client):
    async def impl(text: str, expires_in: str = None, *, client: auth.Client = auth_client) -> UploadPasteResult:
        # Preparation
        create_url_res = await api_upload_create_url(expires_in, client=client)
        s3_upload_res = await s3_upload(create_url_res.presigned_url, text)
        submit_res = await api_upload_submit(create_url_res.paste_id, client=client)

        assert submit_res.size_bytes == s3_upload_res.size_bytes
        assert create_url_res.created_at_utc == submit_res.created_at_utc
        assert create_url_res.expires_at_utc == submit_res.expires_at_utc

        return UploadPasteResult(
            paste_id=create_url_res.paste_id,
            presigned_url=create_url_res.presigned_url,
            version_id=s3_upload_res.version_id,
            data=s3_upload_res.data,
            created_at_utc=submit_res.created_at_utc,
            expires_at_utc=submit_res.expires_at_utc,
            size_bytes=s3_upload_res.size_bytes
        )
    return impl
