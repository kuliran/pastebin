import pytest
from dataclasses import dataclass
from datetime import datetime

@dataclass
class GetPasteResult:
    text: str
    version_id: str
    size_bytes: int
    created_at_utc: datetime
    expires_at_utc: datetime

@pytest.fixture
def raw_get_paste(pg_cursor, minio_server) -> GetPasteResult:
    async def _get(paste_id: str):
        pg_cursor.execute("""
            SELECT created_at, expires_at, size_bytes
            FROM pastes.metadata
            WHERE id = %s
        """, (paste_id,))
        created_at, expires_at, size_bytes = pg_cursor.fetchone()
        assert created_at is not None

        key = 'pending/' + paste_id

        s3 = minio_server["client"]
        response = s3.get_object(Bucket=minio_server['bucket'], Key=key)
        content = response["Body"].read()
        head = s3.head_object(Bucket=minio_server['bucket'], Key=key)
        version_id = head["VersionId"]
        assert size_bytes == head["ContentLength"]

        return GetPasteResult(
            text=content,
            version_id=version_id,
            size_bytes=size_bytes,
            created_at_utc=created_at,
            expires_at_utc=expires_at,
        )
    return _get

@pytest.fixture
def raw_get_paste_expect_none(pg_cursor):
    async def _get(paste_id: str):
        pg_cursor.execute("""
            SELECT created_at
            FROM pastes.metadata
            WHERE id = %s
        """, (paste_id,))
        created_at = pg_cursor.fetchone()

        assert created_at is None
    return _get
