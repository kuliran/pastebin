import pytest
from fixtures.auth import get_cookie, Client

# ============================================
async def test_signup_and_refresh(auth_client, auth_refresh_raw):
    r = await auth_refresh_raw(auth_client._refresh_tk)
    assert r.status == 200
    new_refresh_tk = get_cookie(r, 'refresh_tk')

    r2 = await auth_refresh_raw('garbage')
    assert r2.status == 401

    r3 = await auth_refresh_raw(new_refresh_tk)
    assert r3.status == 200
    auth_client._refresh_tk = get_cookie(r3, 'refresh_tk')
    auth_client._access_tk = r3.json()['access_tk']

async def test_signup_login_refresh(auth_client, login, auth_refresh_raw):
    # 2 sessions
    session = await login(auth_client._username, auth_client._password)

    # refresh both
    s2 = await auth_refresh_raw(auth_client._refresh_tk)
    assert s2.status == 200
    tk1 = get_cookie(s2, 'refresh_tk')
    assert tk1

    l2 = await auth_refresh_raw(session._refresh_tk)
    assert l2.status == 200
    tk2 = get_cookie(l2, 'refresh_tk')
    assert tk2

    # old tokens fail
    assert (await auth_refresh_raw(session._refresh_tk)).status == 401
    assert (await auth_refresh_raw(auth_client._refresh_tk)).status == 401

    # double refresh
    s3 = await auth_refresh_raw(tk1)
    assert s3.status == 200
    auth_client._refresh_tk = get_cookie(s3, 'refresh_tk')
    auth_client._access_tk = s3.json()['access_tk']
    assert (await auth_refresh_raw(tk2)).status == 200

async def test_username_conflict(new_auth_client, signup_raw):
    username = 'test_username_conflict'

    await new_auth_client(username, username)
    r = await signup_raw(username, 'abcxyzZz1_')
    assert r.status == 409

async def can_login_other_username_if_pwd_same(api, new_auth_client, login, api_get_friends, api_get_friends_raw):
    username = 'same_password'
    pwd = 'Same_pwd1'

    client1 = new_auth_client(f'{username}_1', pwd)
    client2 = new_auth_client(f'{username}_2', pwd)

    await login(client2._username, pwd)
    await login(client1._username, pwd)

    # test if same passwords give different jwt access tokens
    r = await api_get_friends_raw(client=Client(
        _client=api,
        _user_id=client1._user_id,
        _access_tk=client2._access_tk,
        _refresh_tk=client1._refresh_tk,
    ))
    assert r.status in (401, 403)
    r = await api_get_friends_raw(client=Client(
        _client=api,
        _user_id=client2._user_id,
        _access_tk=client1._access_tk,
        _refresh_tk=client2._refresh_tk,
    ))
    assert r.status in (401, 403)

    # but normal requests are still authorized
    await api_get_friends(client=client1)
    await api_get_friends(client=client2)