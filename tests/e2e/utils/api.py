import pytest
import requests
import os

BASE_URL = os.getenv("E2E_BASE_URL", "http://localhost")
SERVICE_HOST = os.getenv("E2E_HOST", "pastebin.io")

TIMEOUT = 10  # seconds for each request
ENDPOINTS = {
    'auth_signup': '/api/v2/auth/signup',
    'auth_login': '/api/v2/auth/login',
    'auth_refresh': '/api/v2/auth/refresh',
    'get_paste': '/api/v1/',
    'upload_paste': '/api/v1/paste',
    'delete_paste': '/api/v1/delete/',
}


@pytest.fixture(scope='session')
def endpoints():
    return ENDPOINTS

@pytest.fixture(scope="session")
def api():
    return ApiSession(BASE_URL, host=SERVICE_HOST)

class ApiSession:
    def __init__(self, base_url: str, host: str):
        self._base_url = base_url
        self._session = requests.Session()
        self._session.headers.update({'Host': host})

    def get(self, path, **kwargs):
        return self._session.get(f"{self._base_url}{path}", timeout=TIMEOUT, **kwargs)

    def post(self, path, **kwargs):
        return self._session.post(f"{self._base_url}{path}", timeout=TIMEOUT, **kwargs)

    def delete(self, path, **kwargs):
        return self._session.delete(f"{self._base_url}{path}", timeout=TIMEOUT, **kwargs)

    def clear_cookies(self):
        self._session.cookies.clear()