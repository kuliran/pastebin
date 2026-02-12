import pytest
from fixtures.raw_insert_paste import RawInsertResult

# =========================================
# ================= TESTS =================
# =========================================
async def test_basic(raw_insert_and_get, mongo_collection):
    paste_text = 'Hello, world!'

    assert mongo_collection.count_documents({}) == 0
    await raw_insert_and_get(paste_text)
    assert mongo_collection.count_documents({}) == 1

async def test_utf8(raw_insert_and_get, mongo_collection):
    paste_text = 'Привет, Андрей!'

    assert mongo_collection.count_documents({}) == 0
    await raw_insert_and_get(paste_text)
    assert mongo_collection.count_documents({}) == 1

# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
@pytest.fixture
async def raw_insert_and_get(api_get_paste, raw_insert_paste):
    async def _insert_and_get(paste_text: str) -> RawInsertResult:
        paste_id = "abc123"
        delete_key = "xyz"

        raw_insert_res = await raw_insert_paste(paste_id, paste_text, delete_key)
        get_res = await api_get_paste(paste_id)
        
        assert get_res.created_at_utc == raw_insert_res.pg_created_at_utc
        assert get_res.expires_at_utc == raw_insert_res.pg_expires_at_utc
        assert get_res.size_bytes == raw_insert_res.size_bytes
        assert get_res.text == paste_text

        return raw_insert_res
    return _insert_and_get
