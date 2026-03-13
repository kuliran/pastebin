import pytest

@pytest.fixture
def raw_patch_paste(pg_cursor):
    async def _patch(
        paste_id: str,
        *,
        visibility: str = None,
        private_perms_add: list[str] = None,
        private_perms_rm: list[str] = None,
    ):
        if visibility is not None:
            pg_cursor.execute(
                """
                UPDATE pastes.metadata
                SET visibility = %s
                WHERE id = %s
                """,
                (visibility, paste_id)
            )
        if private_perms_rm is not None:
            pg_cursor.execute(
                """
                DELETE FROM pastes.private_permissions
                WHERE paste_id = %s AND user_id = ANY(%s::TEXT[])
                """,
                (paste_id, private_perms_rm)
            )
        if private_perms_add is not None:
            pg_cursor.execute(
                """
                INSERT INTO pastes.private_permissions (paste_id, user_id)
                SELECT %s, unnest(%s::TEXT[])
                """,
                (paste_id, private_perms_add)
            )
    return _patch