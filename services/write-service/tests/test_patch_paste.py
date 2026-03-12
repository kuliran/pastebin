# Start the tests via `make test-debug` or `make test-release`

import pytest
from shared.utils.upload_paste import UploadPasteResult
from shared.utils.client import Client

# =========================================
# ================= TESTS =================
# =========================================
async def test_visibility(api_upload_paste, api_patch_paste, raw_get_paste):
    upload = await api_upload_paste('Hello, world!', visibility='private')
    g = await raw_get_paste(upload.paste_id)
    assert g.visibility == 'private'
    
    await api_patch_paste(upload.paste_id, visibility='public')
    g = await raw_get_paste(upload.paste_id)
    assert g.visibility == 'public'

async def test_private_perms(api_upload_paste, api_patch_paste, raw_get_paste_private_perms):
    users = ['user-id-1', 'user-id-2']

    upload = await api_upload_paste('Hello, world!', visibility='private', private_perms_add=users)
    assert (await raw_get_paste_private_perms(upload.paste_id)) == users

    users.remove('user-id-1')
    await api_patch_paste(upload.paste_id, private_perms_rm=['user-id-1'])
    assert (await raw_get_paste_private_perms(upload.paste_id)) == users

    users.append('user-id-3')
    await api_patch_paste(upload.paste_id, private_perms_add=['user-id-3'])
    assert (await raw_get_paste_private_perms(upload.paste_id)) == users

# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
