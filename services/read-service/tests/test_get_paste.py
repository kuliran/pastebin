import pytest
from shared.fixtures.raw_insert_paste import RawInsertResult
import shared.utils.auth as auth

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

    await raw_insert_paste(paste_id, paste_text, '-1 second')
    await api_get_paste_expect_none(paste_id)

# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
@pytest.fixture
async def raw_insert_and_get(api_get_paste, raw_insert_paste, auth_client):
    async def _insert_and_get(paste_text: str, *, client: auth.Client = auth_client) -> RawInsertResult:
        paste_id = "abc123"

        raw_insert_res = await raw_insert_paste(paste_id, paste_text, client=client)
        get = await api_get_paste(paste_id)

        assert get.created_at_utc == raw_insert_res.created_at_utc
        assert get.expires_at_utc == raw_insert_res.expires_at_utc
        assert get.size_bytes == raw_insert_res.size_bytes
        assert get.data == raw_insert_res.data

        return raw_insert_res
    return _insert_and_get
