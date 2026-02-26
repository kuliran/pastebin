import pytest
from dataclasses import dataclass
from datetime import datetime

@dataclass
class GetPasteResult:
    data: str
    version_id: str
    size_bytes: int
    created_at_utc: datetime
    expires_at_utc: datetime

@pytest.fixture
def raw_get_paste(pg_cursor, minio_server) -> GetPasteResult:
    async def _get(paste_id: str, *, expect_version_id: str = None):
        pg_cursor.execute("""
            SELECT created_at, expires_at, size_bytes, s3_version_id
            FROM pastes.metadata
            WHERE id = %s
        """, (paste_id,))
        created_at, expires_at, size_bytes, pg_s3_version_id = pg_cursor.fetchone()
        assert created_at is not None
        if expect_version_id is not None:
            assert pg_s3_version_id == expect_version_id

        key = 'pending/' + paste_id

        s3 = minio_server["client"]
        response = s3.get_object(Bucket=minio_server['bucket'], Key=key, VersionId=pg_s3_version_id)
        data = response["Body"].read()
        head = s3.head_object(Bucket=minio_server['bucket'], Key=key, VersionId=pg_s3_version_id)
        version_id = head["VersionId"]
        assert size_bytes == head["ContentLength"]

        return GetPasteResult(
            data=data,
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
            SELECT status
            FROM pastes.metadata
            WHERE id = %s
        """, (paste_id,))
        (status,) = pg_cursor.fetchone()

        assert status is None or status == 'deleted'
    return _get
