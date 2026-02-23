import pytest
from utils.api import ApiSession
import re

def get_cookie(response, name: str) -> str:
    set_cookie = response.headers.get('Set-Cookie', '')
    match = re.search(rf'{name}=([^;]+)', set_cookie)
    if not match:
        raise KeyError(f'Cookie {name!r} not found in Set-Cookie header')
    return match.group(1)


class AuthClient:
    def __init__(self, api: ApiSession, access_tk: str, refresh_tk: str):
        self._api = api
        self._access_tk = access_tk
        self._refresh_tk = refresh_tk

    def get(self, path, **kwargs):
        return self._api.get(path, headers=self._auth_headers(), **kwargs)

    def post(self, path, **kwargs):
        return self._api.post(path, headers=self._auth_headers(), **kwargs)

    def delete(self, path, **kwargs):
        return self._api.delete(path, headers=self._auth_headers(), **kwargs)

    def _auth_headers(self):
        return {'Authorization': f'Bearer {self._access_tk}'}

@pytest.fixture(scope='session')
def auth_client(api, endpoints):
    def _auth_client(username, password) -> AuthClient:
        r = api.post(endpoints['auth_signup'], json={'username': username, 'password': password})
        assert r.status_code == 201

        json = r.json()
        return AuthClient(
            api=api,
            access_tk=json['access_tk'],
            refresh_tk=get_cookie(r, 'refresh_tk'),
        )
    return _auth_client

@pytest.fixture(scope='session')
def login(api, login_raw):
    def _login(username, password) -> AuthClient:
        r = login_raw(username, password)
        assert r.status_code == 200

        json = r.json()
        return AuthClient(
            api=api,
            access_tk=json['access_tk'],
            refresh_tk=r.cookies['refresh_tk'],
        )
    return _login

@pytest.fixture(scope='session')
def login_raw(api, endpoints):
    def _login_raw(username, password):
        api.clear_cookies()
        return api.post(
            endpoints['auth_login'],
            json={'username': username, 'password': password},
        )
    return _login_raw

@pytest.fixture(scope='session')
def auth_refresh_raw(api, endpoints):
    def _auth_refresh_raw(refresh_tk):
        cookies = {}
        if refresh_tk is not None:
            cookies['refresh_tk'] = refresh_tk

        api.clear_cookies()
        return api.post(
            endpoints['auth_refresh'],
            cookies=cookies,
        )
    return _auth_refresh_raw