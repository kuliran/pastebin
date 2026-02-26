# Start the tests via `make test-debug` or `make test-release`

import pytest

# =========================================
# ================= TESTS =================
# =========================================
async def test_raw(raw_insert_paste, api_delete_paste):
    paste_id = "xyz456"
    paste_text = 'Hello, world!'

    await raw_insert_paste(paste_id, paste_text)

    await api_delete_paste(paste_id)

async def test_upload_and_delete(api_upload_full, api_delete_paste):
    paste_text = 'Hello, world!'

    upload_result = await api_upload_full(paste_text)
    await api_delete_paste(upload_result.paste_id)

async def test_upload_get_delete_get(api_upload_full, raw_get_paste, api_delete_paste, raw_get_paste_expect_none):
    paste_text = 'Hello, world!'
    upload_result = await api_upload_full(paste_text)
    await raw_get_paste(upload_result.paste_id)
    await api_delete_paste(upload_result.paste_id)
    await raw_get_paste_expect_none(upload_result.paste_id)

async def test_delete_non_existent(api_delete_paste):
    paste_id = "xyz456"

    # Should return 204 regardless
    await api_delete_paste(paste_id)

async def test_upload_2_and_delete_1(api_upload_full, api_delete_paste, raw_get_paste, raw_get_paste_expect_none):
    ## Test that deleting 1 doesn't delete all
    paste_text = 'Hello, world!'
    paste_text2 = 'Good]]\tmorning'

    upload_result = await api_upload_full(paste_text)
    upload_result2 = await api_upload_full(paste_text2)
    await api_delete_paste(upload_result.paste_id)
    await raw_get_paste_expect_none(upload_result.paste_id)
    await raw_get_paste(upload_result2.paste_id)

async def test_delete_non_owner_fails(api_upload_full, api_delete_paste_raw, raw_get_paste, new_auth_client):
    paste_text = 'Hello, world!'
    
    res = await api_upload_full(paste_text)

    other_client = new_auth_client('other-user-id')
    delete = await api_delete_paste_raw(res.paste_id, client=other_client)
    assert delete.status == 403
    await raw_get_paste(res.paste_id)