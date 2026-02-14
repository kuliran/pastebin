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

async def test_non_existent(api_get_paste_expect_404, mongo_collection):
    paste_id = 'abc123'

    assert mongo_collection.count_documents({}) == 0
    await api_get_paste_expect_404(paste_id)
    assert mongo_collection.count_documents({}) == 0

async def test_expired(api_get_paste_expect_404, raw_insert_paste, mongo_collection):
    paste_id = 'abc123'
    paste_text = 'abc'

    await raw_insert_paste(paste_id, paste_text, 'xyz', '-1 second')
    assert mongo_collection.count_documents({}) == 1
    await api_get_paste_expect_404(paste_id)

# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
@pytest.fixture
async def raw_insert_and_get(api_get_paste, raw_insert_paste):
    async def _insert_and_get(paste_text: str) -> RawInsertResult:
        paste_id = "abc123"
        delete_key = "xyz"

        raw_insert_res = await raw_insert_paste(paste_id, paste_text, delete_key)
        get_response = await api_get_paste(paste_id)
        
        assert get_response.created_at_utc == raw_insert_res.pg_created_at_utc
        assert get_response.expires_at_utc == raw_insert_res.pg_expires_at_utc
        assert get_response.size_bytes == raw_insert_res.size_bytes
        assert get_response.text == paste_text

        return raw_insert_res
    return _insert_and_get
