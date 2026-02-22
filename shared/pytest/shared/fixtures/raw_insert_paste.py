import pytest
from dataclasses import dataclass
from datetime import datetime

@dataclass
class RawInsertResult:
    size_bytes: int
    pg_created_at_utc: datetime
    pg_expires_at_utc: datetime

@pytest.fixture
def raw_insert_paste(pg_cursor, mongo_collection):
    async def _insert(paste_id: str, paste_text: str, delete_key: str, expires_in: str = '24 hours') -> RawInsertResult:
        text_utf = paste_text.encode('utf-8')
        utf_len = len(text_utf)

        # Insert into Postgres
        pg_cursor.execute("""
            INSERT INTO pastes.metadata(id, created_at, expires_at, size_bytes, delete_key)
            VALUES (%s, NOW(), NOW() + INTERVAL %s, %s, %s)
            RETURNING created_at, expires_at
        """, (paste_id, expires_in, utf_len, delete_key))
        pg_created_at, pg_expires_at = pg_cursor.fetchone()

        # Insert into Mongo
        mongo_collection.insert_one({
            '_id': paste_id,
            'text': paste_text,
            'expire_at': pg_expires_at
        })

        return RawInsertResult(
            size_bytes=utf_len,
            pg_created_at_utc=pg_created_at,
            pg_expires_at_utc=pg_expires_at,
        )
    return _insert
