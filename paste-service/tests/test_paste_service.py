# Start the tests via `make test-debug` or `make test-release`

from testsuite.databases import mongo
from datetime import datetime, timezone

async def test_get_paste(service_client, pgsql, mongodb):
    paste_id = 'abc123'
    paste_text = 'Hello, world!'

    pg_cursor = pgsql['db_1'].cursor()
    pg_cursor.execute("""
        INSERT INTO pastes.metadata(id, created_at, expires_at, size_bytes, delete_key)
        VALUES (%s, NOW(), NOW() + INTERVAL '1 day', 10, '_')
        RETURNING created_at, expires_at
    """, (paste_id,))
    created_at, expires_at = pg_cursor.fetchone()

    blobs_db = mongodb['pastes_db_1']
    assert blobs_db.count_documents({}) == 0
    blobs_db.insert_one({
        '_id': paste_id,
        'text': paste_text
    })

    response = await service_client.get(f'/api/v1/{paste_id}')
    assert response.status == 200
    assert 'application/json' in response.headers['Content-Type']
    assert blobs_db.count_documents({}) == 1

    json = response.json()
    json_created_at = datetime.fromisoformat(json['created_at'])
    json_expires_at = datetime.fromisoformat(json['expires_at'])
    created_at_utc = created_at.astimezone(timezone.utc)
    expires_at_utc = expires_at.astimezone(timezone.utc)

    assert json_created_at == created_at_utc
    assert json_expires_at == expires_at_utc
    assert json['size_bytes'] == 10
    assert json['text'] == paste_text

