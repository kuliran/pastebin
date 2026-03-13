import pytest
from datetime import datetime, timezone

# =========================================
# ================= TESTS =================
# =========================================
async def test_basic(api_get_my_pastes):
    await api_get_my_pastes()

async def test_visibility(raw_insert_paste, api_get_paste, api_get_my_pastes):
    paste_text = 'Hello, world!'

    await raw_insert_paste('abc123', paste_text, visibility='private')

    now = datetime.now(timezone.utc)
    pastes = await api_get_my_pastes()
    assert pastes
    assert pastes[0].visibility == 'private'
    assert (now - pastes[0].created_at_utc).seconds <= 1

    get = await api_get_paste(pastes[0].id)
    assert get.data == paste_text.encode()

async def test_many(raw_insert_paste, raw_delete_paste, api_get_my_pastes):
    for x in range(0,3):
        await raw_insert_paste(f'abc-{x}', 'Hello, world!')

    pastes = await api_get_my_pastes()
    assert len(pastes) == 3

    del_id = pastes[0].id
    await raw_delete_paste(del_id)
    pastes = await api_get_my_pastes()
    assert len(pastes) == 2
    assert del_id not in pastes

async def test_2_users(raw_insert_paste, api_get_my_pastes, new_auth_client):
    diff = new_auth_client('diff-user-id')

    await raw_insert_paste('abc123', 'Hello, world!', client=diff)

    pastes = await api_get_my_pastes()
    assert len(pastes) == 0

    pastes = await api_get_my_pastes(client=diff)
    assert len(pastes) == 1

# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
