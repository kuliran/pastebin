# Start the tests via `make test-debug` or `make test-release`

import pytest
import asyncio

# =========================================
# ================= TESTS =================
# =========================================
async def test_raw(raw_insert_paste, api_delete_paste, mongo_collection):
    paste_id = "xyz456"
    paste_text = 'Hello, world!'
    delete_key = "aaa-bb5-c2c-uuid-test"

    assert mongo_collection.count_documents({}) == 0
    await raw_insert_paste(paste_id, paste_text, delete_key)
    assert mongo_collection.count_documents({}) == 1

    await api_delete_paste(paste_id, delete_key)
    assert await wait_for_empty_mongo_collection(mongo_collection, 5, 0.1) == True, "Mongo blob collection is not empty after deletion"

async def test_upload_and_delete(api_upload_paste, api_delete_paste):
    paste_text = 'Hello, world!'

    upload_result = await api_upload_paste(paste_text)
    await api_delete_paste(upload_result.paste_id, upload_result.delete_key)

async def test_upload_get_delete_get(api_upload_paste, api_get_paste, api_delete_paste, api_get_paste_expect_404):
    paste_text = 'Hello, world!'
    upload_result = await api_upload_paste(paste_text)
    get_result = await api_get_paste(upload_result.paste_id)
    await api_delete_paste(upload_result.paste_id, upload_result.delete_key)
    await api_get_paste_expect_404(upload_result.paste_id)

async def test_delete_non_existent(api_delete_paste):
    paste_id = "xyz456"
    delete_key = "aaa-bb5-c2c-uuid-test"

    # Should also return 204 regardless
    await api_delete_paste(paste_id, delete_key)

async def test_upload_2_and_delete_1(api_upload_paste, api_delete_paste, api_get_paste, api_get_paste_expect_404):
    paste_text = 'Hello, world!'
    paste_text2 = 'Good]]\tmorning'

    upload_result = await api_upload_paste(paste_text)
    upload_result2 = await api_upload_paste(paste_text2)
    await api_delete_paste(upload_result.paste_id, upload_result.delete_key)
    await api_get_paste_expect_404(upload_result.paste_id)
    await api_get_paste(upload_result2.paste_id)

# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
async def wait_for_empty_mongo_collection(mongo_collection, timeout=5, interval=0.1):
    total_wait = 0
    while total_wait < timeout:
        if mongo_collection.count_documents({}) == 0:
            return True
        await asyncio.sleep(interval)
        total_wait += interval
    return False
