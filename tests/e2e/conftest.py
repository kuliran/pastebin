import pytest
import os
import sys
import pathlib
import requests

BASE_URL = os.getenv("E2E_BASE_URL", "http://localhost")
SERVICE_HOST = os.getenv("E2E_HOST", "pastebin.io")
TIMEOUT = 10  # seconds for each request

REPO_ROOT = pathlib.Path(__file__).parent.parent.parent
sys.path.insert(0, str(REPO_ROOT / 'shared' / 'pytest'))

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

@pytest.fixture(scope="session")
def api():
    return ApiSession(BASE_URL, host=SERVICE_HOST)