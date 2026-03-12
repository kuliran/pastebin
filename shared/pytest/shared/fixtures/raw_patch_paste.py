import pytest

@pytest.fixture
def raw_patch_paste(pg_cursor):
    async def _patch(
        paste_id: str,
        *,
        visibility: str = 'public',
        private_perms_add: [str] = [],
        private_perms_rm: [str] = [],
    ):
        pg_cursor.execute(
            """
            UPDATE pastes.metadata
            SET visibility = %s
            WHERE id = %s
            """,
            (visibility, paste_id)
        )

        for user_id in private_perms_rm:
            pg_cursor.execute(
                """
                DELETE FROM pastes.private_permissions
                WHERE paste_id = %s AND user_id = %s
                """,
                (paste_id, user_id)
            )
        for user_id in private_perms_add:
            pg_cursor.execute(
                """
                INSERT INTO pastes.private_permissions
                (paste_id, user_id) VALUES (%s, %s)
                """,
                (paste_id, user_id)
            )
    return _patch