import pytest
from dataclasses import dataclass
from datetime import datetime

@dataclass
class GetPasteResult:
    data: str
    size_bytes: int
    visibility: str
    created_at_utc: datetime
    expires_at_utc: datetime

@pytest.fixture
def raw_get_paste(pg_cursor, minio_server) -> GetPasteResult:
    async def _get(paste_id: str):
        pg_cursor.execute("""
            SELECT visibility, created_at, expires_at, size_bytes, s3_version_id
            FROM pastes.metadata
            WHERE id = %s
        """, (paste_id,))
        visibility, created_at, expires_at, size_bytes, pg_s3_version_id = pg_cursor.fetchone()
        assert created_at is not None

        key = 'submitted/' + paste_id

        s3 = minio_server["client"]
        response = s3.get_object(Bucket=minio_server['bucket'], Key=key)
        data = response["Body"].read()
        head = s3.head_object(Bucket=minio_server['bucket'], Key=key)
        assert size_bytes == head["ContentLength"]

        return GetPasteResult(
            data=data,
            size_bytes=size_bytes,
            visibility=visibility,
            created_at_utc=created_at,
            expires_at_utc=expires_at,
        )
    return _get

@pytest.fixture
def raw_get_paste_expect_none(pg_cursor):
    async def _get(paste_id: str):
        pg_cursor.execute("""
            SELECT status
            FROM pastes.metadata
            WHERE id = %s
        """, (paste_id,))
        row = pg_cursor.fetchone()

        if row is not None:
            (status,) = row
            assert status == 'deleted'
    return _get

@pytest.fixture
def raw_get_paste_private_perms(pg_cursor):
    async def _get(paste_id: str) -> list[str]:
        pg_cursor.execute("""
            SELECT user_id
            FROM pastes.private_permissions
            WHERE paste_id = %s
        """, (paste_id,))
        rows = pg_cursor.fetchall()
        return [row[0] for row in rows]
    return _get