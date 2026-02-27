import pytest
import re
from shared.utils.client import Client

def get_cookie(response, name: str) -> str:
    set_cookie = response.headers.get('Set-Cookie', '')
    match = re.search(rf'{name}=([^;]+)', set_cookie)
    if not match:
        raise KeyError(f'Cookie {name!r} not found in Set-Cookie header')
    return match.group(1)


@pytest.fixture
async def auth_client(new_auth_client):
    return await new_auth_client('test-username', 'test-pass')

@pytest.fixture(scope='session')
def new_auth_client(api, endpoints):
    async def _auth_client(username: str, password: str) -> Client:
        api.clear_cookies()
        r = await api.post(endpoints['auth_signup'], json={'username': username, 'password': password})
        assert r.status == 201

        json = r.json()
        return Client(
            _client=api,
            _user_id=json['user_id'],
            _access_tk=json['access_tk'],
            _refresh_tk=get_cookie(r, 'refresh_tk'),
            _username=username,
            _password=password,
        )
    return _auth_client

@pytest.fixture(scope='session')
def new_unauth_client(api):
    def _auth_client() -> Client:
        return Client(
            _client=api,
            _user_id="unauth-user-id",
            _access_tk=None,
            _refresh_tk=None,
        )
    return _auth_client

@pytest.fixture(scope='session')
def login(api, login_raw):
    async def _login(username: str, password: str) -> Client:
        r = await login_raw(username, password)
        assert r.status == 200

        json = r.json()
        return Client(
            _client=api,
            _user_id=json['user_id'],
            _access_tk=json['access_tk'],
            _refresh_tk=r.cookies['refresh_tk'],
        )
    return _login

@pytest.fixture(scope='session')
def login_raw(api, endpoints):
    async def _login_raw(username: str, password: str):
        api.clear_cookies()
        return await api.post(
            endpoints['auth_login'],
            json={'username': username, 'password': password},
        )
    return _login_raw

@pytest.fixture(scope='session')
def auth_refresh_raw(api, endpoints):
    async def _auth_refresh_raw(refresh_tk: str):
        api.clear_cookies()
        if refresh_tk is not None:
            api.set_cookie('refresh_tk', refresh_tk)
        return await api.post(
            endpoints['auth_refresh'],
        )
    return _auth_refresh_raw