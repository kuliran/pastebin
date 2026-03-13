import pytest

@pytest.fixture
def raw_delete_paste(pg_cursor, minio_server, auth_client):
    async def _delete(paste_id: str):
        pg_cursor.execute(
            """
            UPDATE pastes.metadata
            SET status = 'deleted'
            WHERE id = %s
            """,
            (paste_id,)
        )
    return _delete
