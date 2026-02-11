import pytest
from dataclasses import dataclass
from datetime import datetime, timezone

DEFAULT_PASTE_LIFETIME_SECONDS = 60*60*24*7 # 1 week

@dataclass
class UploadResult:
    paste_id: str
    paste_text_utf: str
    delete_key: str
    pg_created_at_utc: datetime
    pg_expires_at_utc: datetime
    pg_size_bytes: int

@pytest.fixture
async def api_upload_fixture(service_client, pg_cursor, mongo_collection) -> UploadResult:
    async def _upload(paste_text: str) -> UploadResult:
        # Preparation
        utf_text = paste_text.encode('utf-8')
        utf_len = len(utf_text)
        now_utc = datetime.now(timezone.utc)

        # API call
        response = await service_client.post(f'/api/v1/paste/', json={"text": paste_text})
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']

        json = response.json()
        paste_id = json['id']
        assert type(paste_id) is str
        assert 1 <= len(paste_id) <= 64
        delete_key = json['delete_key']
        assert type(delete_key) is str

        # Postgres validation
        pg_cursor.execute("""
            SELECT created_at, expires_at, size_bytes, delete_key
            FROM pastes.metadata
            WHERE id = %s
        """, (paste_id,))
        pg_created_at, pg_expires_at, pg_size_bytes, pg_delete_key = pg_cursor.fetchone()
        assert abs((pg_created_at - now_utc).total_seconds()) <= 1
        assert (pg_expires_at - pg_created_at).total_seconds() == DEFAULT_PASTE_LIFETIME_SECONDS
        assert pg_size_bytes == utf_len
        assert pg_delete_key == delete_key

        # Mongo validation
        blob = mongo_collection.find_one({"_id": paste_id})
        assert blob is not None
        assert blob["_id"] == paste_id
        assert blob["text"] == paste_text

        return UploadResult(
            paste_id=paste_id,
            paste_text_utf=utf_text,
            delete_key=delete_key,
            pg_created_at_utc=pg_created_at,
            pg_expires_at_utc=pg_expires_at,
            pg_size_bytes=pg_size_bytes
        )
    return _upload
