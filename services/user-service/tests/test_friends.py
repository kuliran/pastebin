import pytest

# =========================================
# ================= TESTS =================
# =========================================
async def test_get_friends_unauth(api_get_friends_raw, new_unauth_client):
    unauth = new_unauth_client()

    r = await api_get_friends_raw(client=unauth)
    assert r.status in (400, 401, 403)

async def test_add_friend_unauth(api_add_friend_raw, auth_client, new_unauth_client):
    unauth = new_unauth_client()

    r = await api_add_friend_raw(auth_client._user_id, client=unauth)
    assert r.status in (400, 401, 403)

async def test_rm_friend_unauth(api_rm_friend_raw, auth_client, new_unauth_client):
    unauth = new_unauth_client()

    r = await api_rm_friend_raw(auth_client._user_id, client=unauth)
    assert r.status in (400, 401, 403)

async def test_friends(api_get_friends, api_add_friend, api_rm_friend, new_auth_client):
    diff_client = new_auth_client('diff-user-id')

    friends = await api_get_friends()
    assert len(friends) == 0

    async def add_friend_expect_one():
        await api_add_friend(diff_client._user_id)
        r = await api_get_friends()
        assert len(r) == 1
        assert r[0] == diff_client._user_id

    await add_friend_expect_one()  # add
    await add_friend_expect_one()  # add again - no changes

    # rm
    await api_rm_friend(diff_client._user_id)
    assert len(await api_get_friends()) == 0
    
    await add_friend_expect_one()  # add again
