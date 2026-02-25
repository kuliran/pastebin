import pytest
from dataclasses import dataclass
from datetime import datetime

@dataclass
class RawInsertResult:
    paste_id: str
    version_id: str
    size_bytes: int
    pg_created_at_utc: datetime
    pg_expires_at_utc: datetime

@pytest.fixture
def raw_insert_paste(pg_cursor, minio_server, auth_client):
    async def _insert(
        paste_id: str,
        paste_text: str,
        expires_in: str = '24 hours',
        owner_user_id: str = auth_client._user_id
    ) -> RawInsertResult:
        s3 = minio_server["client"]
        content = paste_text.encode("utf-8")

        response = s3.put_object(
            Bucket=minio_server["bucket"],
            Key=f"submitted/{paste_id}",
            Body=content,
        )
        version_id = response["VersionId"]
        size_bytes = len(content)

        pg_cursor.execute(
            """
            INSERT INTO pastes.metadata (id, owner_user_id, status, s3_version_id, size_bytes, created_at, expires_at)
            VALUES (%s, %s, 'submitted', %s, %s, NOW(), NOW() + %s::interval)
            RETURNING created_at, expires_at
            """,
            (paste_id, owner_user_id, version_id, size_bytes, expires_in)
        )
        pg_created_at, pg_expires_at = pg_cursor.fetchone()

        return RawInsertResult(
            paste_id=paste_id,
            version_id=version_id,
            size_bytes=size_bytes,
            pg_created_at_utc=pg_created_at,
            pg_expires_at_utc=pg_expires_at,
        )
    return _insert
