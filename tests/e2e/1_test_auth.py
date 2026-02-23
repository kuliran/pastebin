import pytest
from dataclasses import dataclass
from utils.auth import Client, get_cookie

USERNAME = 'test_auth'
PASSWORD = 'test_auth'

@dataclass
class Context:
    client: Client

@pytest.fixture(scope='session')
def ctx(auth_client) -> Context:
    return Context(client=auth_client(USERNAME, PASSWORD))

# ============================================
def test_signup_and_refresh(ctx, auth_refresh_raw):
    r = auth_refresh_raw(ctx.client._refresh_tk)
    assert r.status_code == 200
    new_refresh_tk = get_cookie(r, 'refresh_tk')

    r2 = auth_refresh_raw('garbage')
    assert r2.status_code == 401

    r3 = auth_refresh_raw(new_refresh_tk)
    assert r3.status_code == 200
    ctx.client._refresh_tk = get_cookie(r3, 'refresh_tk')
    ctx.client._access_tk = r3.json()['access_tk']

def test_signup_login_refresh(ctx, login, auth_refresh_raw):
    # 2 sessions
    session = login(USERNAME, PASSWORD)

    # refresh both
    s2 = auth_refresh_raw(ctx.client._refresh_tk)
    assert s2.status_code == 200
    tk1 = get_cookie(s2, 'refresh_tk')
    assert tk1

    l2 = auth_refresh_raw(session._refresh_tk)
    assert l2.status_code == 200
    tk2 = get_cookie(l2, 'refresh_tk')
    assert tk2

    # old tokens fail
    assert auth_refresh_raw(session._refresh_tk).status_code == 401
    assert auth_refresh_raw(ctx.client._refresh_tk).status_code == 401

    # double refresh
    s3 = auth_refresh_raw(tk1)
    assert s3.status_code == 200
    ctx.client._refresh_tk = get_cookie(s3, 'refresh_tk')
    ctx.client._access_tk = s3.json()['access_tk']
    assert auth_refresh_raw(tk2).status_code == 200