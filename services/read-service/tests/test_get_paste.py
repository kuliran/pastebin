import pytest
from shared.fixtures.raw_insert_paste import RawInsertResult
from shared.utils.client import Client

# =========================================
# ================= TESTS =================
# =========================================
async def test_basic(raw_insert_and_get):
    paste_text = 'Hello, world!'
    await raw_insert_and_get(paste_text)

async def test_utf8(raw_insert_and_get):
    paste_text = 'Привет мир! 🌍 こんにちは'
    await raw_insert_and_get(paste_text)

async def test_non_existent(api_get_paste_expect_none):
    paste_id = 'abc123'
    await api_get_paste_expect_none(paste_id)

async def test_expired(api_get_paste_expect_none, raw_insert_paste):
    paste_id = 'abc123'
    paste_text = 'abc'

    await raw_insert_paste(paste_id, paste_text, expires_in='-1 second')
    await api_get_paste_expect_none(paste_id)

async def test_private_visibility(api_get_paste_expect_unauth, api_get_paste, raw_insert_paste, raw_patch_paste, new_auth_client):
    other = new_auth_client('other-user-id')

    res = await raw_insert_paste('abc123', 'abc', visibility='private')
    await api_get_paste(res.paste_id)
    await api_get_paste_expect_unauth(res.paste_id, client=other)
    await raw_patch_paste(res.paste_id, visibility='public')
    await api_get_paste(res.paste_id, client=other)

    await raw_patch_paste(res.paste_id, visibility='private')
    await api_get_paste_expect_unauth(res.paste_id, client=other)

    await raw_patch_paste(res.paste_id, private_perms_add=[other._user_id])
    await api_get_paste(res.paste_id, client=other)

async def test_get_friend(mock_is_friend, raw_insert_paste, api_get_paste, new_auth_client):
    mock_is_friend(result=True)

    other = new_auth_client('other-user-id')

    res = await raw_insert_paste('abc123', 'abc', visibility='friends')
    r1 = await api_get_paste(res.paste_id)
    r2 = await api_get_paste(res.paste_id, client=other)

    import dataclasses
    for field in dataclasses.fields(r1):
        assert getattr(r1, field.name) == getattr(r2, field.name)

async def test_get_not_friend(mock_is_friend, raw_insert_paste, api_get_paste, api_get_paste_expect_unauth, new_auth_client):
    mock_is_friend(result=False)

    other = new_auth_client('other-user-id')

    res = await raw_insert_paste('abc123', 'abc', visibility='friends')
    r1 = await api_get_paste(res.paste_id)
    await api_get_paste_expect_unauth(res.paste_id, client=other)

# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
@pytest.fixture
async def raw_insert_and_get(api_get_paste, raw_insert_paste, auth_client):
    async def _insert_and_get(paste_text: str, *, client: Client = auth_client) -> RawInsertResult:
        paste_id = "abc123"

        raw_insert_res = await raw_insert_paste(paste_id, paste_text, client=client)
        get = await api_get_paste(paste_id)

        assert get.created_at_utc == raw_insert_res.created_at_utc
        assert get.expires_at_utc == raw_insert_res.expires_at_utc
        assert get.size_bytes == raw_insert_res.size_bytes
        assert get.data == raw_insert_res.data

        return raw_insert_res
    return _insert_and_get
