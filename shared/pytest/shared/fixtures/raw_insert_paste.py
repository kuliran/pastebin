import pytest
from dataclasses import dataclass
from datetime import datetime
from shared.utils.client import Client

@dataclass
class RawInsertResult:
    paste_id: str
    version_id: str
    size_bytes: int
    data: bytes
    created_at_utc: datetime
    expires_at_utc: datetime

@pytest.fixture
def raw_insert_paste(pg_cursor, minio_server, auth_client):
    async def _insert(
        paste_id: str,
        paste_text: str,
        *,
        expires_in: str = '24 hours',
        visibility: str = 'public',
        client: Client = auth_client
    ) -> RawInsertResult:
        data = paste_text.encode("utf-8")

        response = minio_server["client"].put_object(
            Bucket=minio_server["bucket"],
            Key=f"pending/{paste_id}",
            Body=data,
        )
        version_id = response["VersionId"]
        size_bytes = len(data)

        minio_server["client"].copy_object(
            Bucket=minio_server["bucket"],
            CopySource={
                'Bucket': minio_server["bucket"],
                'Key': f'pending/{paste_id}',
                'VersionId': version_id,
            },
            Key=f'submitted/{paste_id}',
        )

        pg_cursor.execute(
            """
            INSERT INTO pastes.metadata (id, owner_user_id, visibility, status, s3_version_id, size_bytes, created_at, expires_at)
            VALUES (%s, %s, %s, 'submitted', %s, %s, NOW(), NOW() + %s::interval)
            RETURNING created_at, expires_at
            """,
            (paste_id, client._user_id, visibility, version_id, size_bytes, expires_in)
        )
        pg_created_at, pg_expires_at = pg_cursor.fetchone()

        return RawInsertResult(
            paste_id=paste_id,
            version_id=version_id,
            size_bytes=size_bytes,
            data=data,
            created_at_utc=pg_created_at,
            expires_at_utc=pg_expires_at,
        )
    return _insert
