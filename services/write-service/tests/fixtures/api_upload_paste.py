import pytest
from dataclasses import dataclass
from datetime import datetime, timezone

@dataclass
class UploadResult:
    paste_id: str
    paste_text_utf: str
    delete_key: str
    pg_created_at_utc: datetime
    pg_expires_at_utc: datetime
    pg_size_bytes: int

@pytest.fixture
async def api_upload_paste(service_client, pg_cursor, mongo_collection) -> UploadResult:
    async def _upload(paste_text: str, expires_in: str = None) -> UploadResult:
        # Preparation
        utf_text = paste_text.encode('utf-8')
        utf_len = len(utf_text)
        now_utc = datetime.now(timezone.utc)

        # API call
        request_json = {"text": paste_text}
        if expires_in is not None:
            request_json["expires_in"] = expires_in
        response = await service_client.post(f'/api/v1/paste/', json=request_json)
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
        pg_created_at = pg_created_at.astimezone(timezone.utc)
        pg_expires_at = pg_expires_at.astimezone(timezone.utc)

        lifetime_seconds = 0
        if expires_in is None or expires_in == '1_week': lifetime_seconds = 60*60*24*7
        elif expires_in == '1_hour': lifetime_seconds = 60*60
        elif expires_in == '1_day': lifetime_seconds = 60*60*24
        elif expires_in == '1_month': lifetime_seconds = 60*60*24*30
        elif expires_in == '3_month': lifetime_seconds = 60*60*24*30*3

        assert abs((pg_created_at - now_utc).total_seconds()) <= 1
        assert (pg_expires_at - pg_created_at).total_seconds() == lifetime_seconds, f"incorrect pg expires_at with param: {expires_in}"
        assert pg_size_bytes == utf_len
        assert pg_delete_key == delete_key

        # Mongo validation
        blob = mongo_collection.find_one({"_id": paste_id})
        assert blob is not None
        assert blob["_id"] == paste_id
        assert blob["text"] == paste_text
        assert abs(blob["expire_at"].astimezone(timezone.utc) - pg_expires_at).total_seconds() < 0.5

        return UploadResult(
            paste_id=paste_id,
            paste_text_utf=utf_text,
            delete_key=delete_key,
            pg_created_at_utc=pg_created_at,
            pg_expires_at_utc=pg_expires_at,
            pg_size_bytes=pg_size_bytes
        )
    return _upload
