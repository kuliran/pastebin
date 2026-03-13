import pytest

# =========================================
# ================= TESTS =================
# =========================================
async def test_basic(raw_insert_paste, api_get_paste, api_get_paste_details):
    paste_id = 'abc123'

    await raw_insert_paste(paste_id, 'Hello, world!', visibility='private')
    await raw_insert_paste('other-paste-id', 'Hello, world2!') # test that it doesn't interfere
    
    details = await api_get_paste_details(paste_id)
    get = await api_get_paste(paste_id)

    assert details.visibility == 'private'
    assert details.size_bytes == get.size_bytes
    assert details.created_at_utc == get.created_at_utc
    assert details.expires_at_utc == get.expires_at_utc
    assert details.private_perms_user_ids == []

async def test_private_perms(raw_insert_paste, raw_patch_paste, raw_get_paste, raw_get_paste_private_perms, api_get_paste, api_get_paste_details):
    paste_id = 'abc123'
    diff_user_id = 'diff-user-id'

    await raw_insert_paste(paste_id, 'Hello, world!', visibility='private')
    await raw_patch_paste(paste_id, private_perms_add=[diff_user_id])
    assert (await raw_get_paste(paste_id)).visibility == 'private'
    assert (await raw_get_paste_private_perms(paste_id)) == [diff_user_id]

    details = await api_get_paste_details(paste_id)
    assert details.visibility == 'private'
    assert details.private_perms_user_ids == [diff_user_id]

    await raw_patch_paste(paste_id, private_perms_rm=[diff_user_id])
    details = await api_get_paste_details(paste_id)
    assert details.private_perms_user_ids == []


# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
