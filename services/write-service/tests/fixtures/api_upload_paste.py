import pytest
from datetime import datetime, timezone
from shared.utils.client import Client
from shared.utils.upload_paste import *


@pytest.fixture
async def api_upload_create_url(pg_cursor, auth_client, api_upload_create_url_raw):
    async def impl(*,
        expires_in: str = None,
        visibility: str = None,
        private_perms_add: list[str] = None,
        client: Client = auth_client
    ) -> UploadCreateUrlResult:
        now_utc = datetime.now(timezone.utc)

        response = await api_upload_create_url_raw(
            expires_in=expires_in, visibility=visibility,
            private_perms_add=private_perms_add, client=client
        )
        assert response.status == 201
        assert 'application/json' in response.headers['Content-Type']

        json = response.json()
        presigned_url = json['presigned_url']
        assert type(presigned_url) is str

        paste_id = json['paste_id']
        assert type(paste_id) is str
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
async def api_upload_submit(pg_cursor, minio_server, api_upload_submit_raw, auth_client):
    async def impl(paste_id: str, *, client: Client = auth_client) -> UploadSubmitResult:
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

        head = minio_server["client"].head_object(Bucket=minio_server["bucket"], Key=f"submitted/{paste_id}")
        assert head["ResponseMetadata"]["HTTPStatusCode"] == 200

        return UploadSubmitResult(
            created_at_utc=pg_created_at,
            expires_at_utc=pg_expires_at,
            size_bytes=pg_size_bytes,
        )
    return impl

@pytest.fixture
async def api_upload_paste(api_upload_create_url, api_upload_submit, s3_upload, auth_client):
    async def impl(text: str, *,
        expires_in: str = None,
        visibility: str = 'public',
        private_perms_add: list[str] = [],
        client: Client = auth_client,
    ) -> UploadPasteResult:
        # Preparation
        create_url_res = await api_upload_create_url(
            expires_in=expires_in, visibility=visibility,
            private_perms_add=private_perms_add, client=client
        )
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
