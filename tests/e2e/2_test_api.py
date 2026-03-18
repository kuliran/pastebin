import pytest
import time
from dateutil.parser import isoparse
from datetime import datetime, timezone
from shared.utils.client import Client
from shared.utils.upload_paste import get_paste_id

# ============================================
async def test_upload_basic(api_upload_paste):
    r = await api_upload_paste("Hello, World!")
    assert len(r.paste_id) > 0

async def test_upload_empty_text_fails(api_upload_create_url_raw, s3_upload, api_upload_submit_raw):
    create_url = await api_upload_create_url_raw()
    presigned_url = create_url.json()['presigned_url']
    paste_id = get_paste_id(presigned_url)
    await s3_upload(presigned_url, "")
    r = await api_upload_submit_raw(paste_id)
    assert r.status == 409

async def test_upload_1MB(api_upload_paste):
    text = "x" * (1024 * 1024)
    await api_upload_paste(text)

async def test_upload_too_large_fails(api_upload_create_url_raw, s3_upload, api_upload_submit_raw):
    text = "x" * (1024 * 1024 + 1)

    create_url = await api_upload_create_url_raw()
    presigned_url = create_url.json()['presigned_url']
    paste_id = get_paste_id(presigned_url)
    await s3_upload(presigned_url, text)
    r = await api_upload_submit_raw(paste_id)
    assert r.status == 413

async def test_upload_utf8(api_upload_paste, api_get_paste):
    text = "Привет мир! 🌍 こんにちは"
    r = await api_upload_paste(text)
    r2 = await api_get_paste(r.paste_id)
    assert r2.data == r.data

async def test_expires_in_field(api_upload_paste, api_get_paste):
    now = datetime.now(timezone.utc)
    r = await api_upload_paste("Hello, world!", expires_in="3_month")
    r2 = await api_get_paste(r.paste_id)

    assert (r2.expires_at_utc - r2.created_at_utc).total_seconds() == 60 * 60 * 24 * 30 * 3
    assert (r2.created_at_utc - now).total_seconds() <= 2

async def test_unauthorized_upload_fails(api_upload_create_url_raw, new_unauth_client):
    unauth = new_unauth_client()
    r = await api_upload_create_url_raw(client=unauth)
    assert r.status in (400, 401, 403)

async def test_upload_twice_and_get(api_upload_create_url_raw, s3_upload, api_upload_submit_raw, api_get_paste):
    paste_text = 'Hello, world!'

    create_url = await api_upload_create_url_raw()
    presigned_url = create_url.json()['presigned_url']
    paste_id = get_paste_id(presigned_url)
    first_upload = await s3_upload(presigned_url, paste_text)
    r = await api_upload_submit_raw(paste_id)
    assert r.status == 200
    await s3_upload(presigned_url, 'Other text')

    get = await api_get_paste(paste_id)
    assert get.data == first_upload.data

# ============================================
async def test_get_nonexistent(api_get_paste_expect_none):
    await api_get_paste_expect_none("nonexistent_id_xyz")

# ============================================
async def test_delete_existing(api_upload_paste, api_get_paste_expect_none, api_delete_paste):
    r = await api_upload_paste("to be deleted")
    await api_delete_paste(r.paste_id)
    await api_get_paste_expect_none(r.paste_id)

async def test_delete_nonexistent(api_delete_paste_raw):
    r = await api_delete_paste_raw("nonexistent_id_xyz")
    assert r.status == 204

async def test_double_delete(api_upload_paste, api_delete_paste):
    r = await api_upload_paste("double delete test")
    await api_delete_paste(r.paste_id)
    await api_delete_paste(r.paste_id)

async def test_unauthorized_delete_fails(api_upload_paste, api_delete_paste_raw, new_unauth_client):
    r = await api_upload_paste("Hello, world!")

    unath = new_unauth_client()
    r2 = await api_delete_paste_raw(r.paste_id, client=unath)
    assert r2.status in (400, 401, 403)

async def test_diff_user_delete_fails(api_upload_paste, api_delete_paste_raw, new_auth_client):
    r = await api_upload_paste("Hello, world!")
    
    diff = await new_auth_client("diff_user_delete_test", "diff_user_delete_test")
    r2 = await api_delete_paste_raw(r.paste_id, client=diff)
    assert r2.status in (401, 403)

# ============================================
async def test_private_visibility(api_upload_paste, api_patch_paste, api_get_paste, api_get_paste_expect_unauth, new_auth_client):
    upload = await api_upload_paste('Hello, world!', visibility='private')
    diff = await new_auth_client('diff_user_visibility_test', 'diff_user_visibility_test')

    await api_get_paste(upload.paste_id)
    await api_get_paste_expect_unauth(upload.paste_id, client=diff)

    await api_patch_paste(upload.paste_id, visibility='public')
    await api_get_paste(upload.paste_id, client=diff)
    await api_get_paste(upload.paste_id)

async def test_private_perms(api_upload_paste, api_patch_paste, api_get_paste, api_get_paste_expect_unauth, new_auth_client, auth_client):
    diff = await new_auth_client('diff_user_private_perms_test', 'diff_user_private_perms_test')
    users = [diff._user_id, auth_client._user_id]

    upload = await api_upload_paste('Hello, world!', visibility='private', private_perms_add=users, client=auth_client)
    await api_get_paste(upload.paste_id, client=diff)
    await api_get_paste(upload.paste_id, client=auth_client)

    await api_patch_paste(upload.paste_id, private_perms_rm=[auth_client._user_id], client=auth_client)
    await api_get_paste(upload.paste_id, client=auth_client)
    await api_get_paste(upload.paste_id, client=diff)

    await api_patch_paste(upload.paste_id, private_perms_rm=[diff._user_id], client=auth_client)
    await api_get_paste_expect_unauth(upload.paste_id, client=diff)

    await api_patch_paste(upload.paste_id, visibility='public', client=auth_client)
    await api_get_paste(upload.paste_id, client=diff)

    await api_patch_paste(upload.paste_id, visibility='private', client=auth_client)
    await api_get_paste_expect_unauth(upload.paste_id, client=diff)
    await api_get_paste(upload.paste_id, client=auth_client)

# ============================================
async def test_get_paste_details(api_upload_paste, api_delete_paste, api_get_paste_details, api_get_paste_details_raw, new_auth_client):
    upload = await api_upload_paste("Hello, world!", visibility='private')

    diff = await new_auth_client("diff_user_get_paste_details_test", "diff_user_get_paste_details_test")
    r = await api_get_paste_details_raw(upload.paste_id, client=diff)
    assert r.status == 403
    details = await api_get_paste_details(upload.paste_id)
    assert details.visibility == 'private'

    await api_delete_paste(upload.paste_id)
    r = await api_get_paste_details_raw(upload.paste_id)
    assert r.status == 404
    r = await api_get_paste_details_raw(upload.paste_id, client=diff)
    assert r.status == 404

async def test_get_my_pastes(api_upload_paste, api_delete_paste, api_get_my_pastes, api_patch_paste, new_auth_client):
    upload = await api_upload_paste("Hello, world!", visibility='private')

    diff = await new_auth_client("diff_user_get_my_pastes_test", "diff_user_get_my_pastes_test")
    assert len(await api_get_my_pastes(client=diff)) == 0

    r = await api_get_my_pastes()
    assert len(r) == 1
    assert r[0].id == upload.paste_id
    assert r[0].visibility == 'private'

    await api_patch_paste(upload.paste_id, visibility='public')
    r = await api_get_my_pastes()
    assert len(r) == 1
    assert r[0].id == upload.paste_id
    assert r[0].visibility == 'public'
    
    assert len(await api_get_my_pastes(client=diff)) == 0

    upload2 = await api_upload_paste("Hello, world!", visibility='private', private_perms_add=[diff._user_id])
    assert len(await api_get_my_pastes()) == 2
    assert len(await api_get_my_pastes(client=diff)) == 0

    await api_delete_paste(upload.paste_id)
    r = await api_get_my_pastes()
    assert len(r) == 1
    assert r[0].id == upload2.paste_id
    assert r[0].visibility == 'private'

# ============================================
async def test_upload_rate_limit(api_upload_create_url_raw):
    responses = []
    for _ in range(50):
        r = await api_upload_create_url_raw()
        responses.append(r.status)

    assert all(s in (201, 429) for s in responses)

# ============================================
async def test_get_friends_empty(api_get_friends, api_get_friends_raw, new_unauth_client):
    unauth = new_unauth_client()
    
    friends = await api_get_friends()
    assert len(friends) == 0

    r = await api_get_friends_raw(client=unauth)
    assert r.status in (400, 401, 403)

async def test_add_rm_friend(api_get_friends, api_add_friend, api_rm_friend, new_auth_client):
    diff = await new_auth_client('test_add_rm_friend', 'test_add_rm_friend')
    
    assert len(await api_get_friends()) == 0

    async def add_friend_expect_one():
        await api_add_friend(diff._user_id)
        friends = await api_get_friends()
        assert len(friends) == 1
        assert friends[0] == diff._user_id
        assert len(await api_get_friends(client=diff)) == 0
    await add_friend_expect_one()
    await add_friend_expect_one() # add same friend - no changes

    await api_rm_friend(diff._user_id)
    assert len(await api_get_friends()) == 0

    await add_friend_expect_one()

async def test_rm_non_existent_friend(api_get_friends, api_rm_friend, new_auth_client):
    diff = await new_auth_client('test_rm_non_existent_friend', 'test_rm_non_existent_friend')

    assert len(await api_get_friends()) == 0

    await api_rm_friend(diff._user_id)
    assert len(await api_get_friends()) == 0
    await api_rm_friend('-non-existent-user-id-abcxyz-')
    assert len(await api_get_friends()) == 0

async def test_many_friends(api_get_friends, api_add_friend, api_rm_friend, new_auth_client):
    username_prefix = 'test_many_friends_'

    # add friends to a diff user
    diff_user = await new_auth_client(f'{username_prefix}diff', f'{username_prefix}1Z')
    diff_friends_to_add = []
    for i in range(0, 3):
        client = new_auth_client(f'{username_prefix}{i+100}', f'{username_prefix}1Z')
        diff_friends_to_add.append(client)

    # add our friends
    friends_to_add = []
    for i in range(0, 3):
        client = new_auth_client(f'{username_prefix}{i}', f'{username_prefix}1Z')
        friends_to_add.append(client)

    assert len(await api_get_friends()) == 0

    for i in range(0, len(friends_to_add)):
        await api_add_friend(friends_to_add[i])
    friends = await api_get_friends()
    assert len(friends) == len(friends_to_add)
    for i in range(0, len(friends_to_add)):
        assert friends[i] == friends_to_add[i]


    # check that friends of a diff user are not affected
    async def check_diff_friends():
        for i in range(0, len(friends_to_add)):
            await api_add_friend(diff_friends_to_add[i])

        diff_friends = await api_get_friends(client=diff_user)
        assert len(diff_friends) == len(diff_friends_to_add)

        for i in range(0, len(diff_friends_to_add)):
            assert diff_friends[i] == diff_friends_to_add[i]
    await check_diff_friends()


    while friends_to_add:
        await api_rm_friend(friends_to_add[0])
        friends_to_add.remove(friends_to_add[0])

        friends_left = await api_get_friends()
        assert len(friends_left) == len(friends_to_add)
        for j in range(0, len(friends_to_add)):
            friends_left[j] == friends_to_add[j]

    await check_diff_friends()

async def test_friends_visibility(
        api_add_friend, api_rm_friend,
        api_upload_paste, new_auth_client,
        api_get_paste, api_get_paste_expect_unauth
):
    diff = await new_auth_client('test_friends_visibility', 'test_friends_visibility')
    
    upload = await api_upload_paste('some text')

    await api_get_paste_expect_unauth(upload.paste_id, client=diff._user_id)
    await api_add_friend(diff._user_id)
    get = await api_get_paste(upload.paste_id, client=diff._user_id)
    assert get.data == upload.data

    await api_rm_friend(diff._user_id)
    await api_get_paste_expect_unauth(upload.paste_id, client=diff._user_id)

# TODO friend limit