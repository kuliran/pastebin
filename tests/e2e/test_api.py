"""
e2e tests
Run from project root dir with:
    make e2e

optionally, change the E2E_HOST (must match the one in nginx/conf.d/paste-service.conf) and E2E_URL:
    E2E_URL=http://localhost  E2E_HOST=pastebin.io  make e2e
""" 

import time
from dataclasses import dataclass
import pytest
import requests
from dateutil.parser import isoparse
from datetime import datetime, timezone
from conftest import ApiSession

# ============================================
# AuthClient
# ============================================
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

    def refresh(self, *, refresh_tk=None):
        tk = refresh_tk or self._refresh_tk
        return self._api.post(
            '/api/v2/auth/refresh',
            cookies={'refresh_tk': tk},
        )
    
    def _auth_headers(self):
        return {'Authorization': f'Bearer {self._access_tk}'}

@pytest.fixture(scope='session')
def auth_client(api):
    def _auth_client(username, password) -> AuthClient:
        r = api.post('/api/v2/auth/signup', json={'username': username, 'password': password})
        assert r.status_code == 201
        json = r.json()
        return AuthClient(
            api=api,
            access_tk=json['access_tk'],
            refresh_tk=r.cookies['refresh_tk'],
        )
    return _auth_client

# ============================================
# Context
# ============================================
@dataclass
class Context:
    client: AuthClient

@pytest.fixture(scope='session')
def ctx(auth_client) -> Context:
    return Context(client=auth_client('testuser', 'testpass'))

# ============================================
class TestAuth:
    def test_signup_and_refresh(self, ctx):
        r = ctx.client.refresh()
        assert r.status_code == 200
        new_refresh_tk = r.cookies['refresh_tk']

        r2 = ctx.client.refresh(refresh_tk='garbage')
        assert r2.status_code == 401

        ctx.client._refresh_tk = new_refresh_tk
        r3 = ctx.client.refresh()
        assert r3.status_code == 200

# ============================================
class TestUploadPaste:
    def test_upload_basic(self, ctx, upload_paste):
        r = upload_paste(ctx, "Hello, World!")
        assert len(r.paste_id) > 0

    def test_upload_empty_text_fails(self, ctx, upload_paste_raw):
        r = upload_paste_raw(ctx, "")
        assert r.status_code == 400

    def test_upload_without_text_field_fails(self, ctx, upload_paste_raw):
        r = upload_paste_raw(ctx, json={})
        assert r.status_code == 400

    def test_upload_1MB(self, ctx, upload_paste):
        content = "x" * (1024 * 1024)
        upload_paste(ctx, content)

    def test_upload_too_large_fails(self, ctx, upload_paste_raw):
        content = "x" * (1024 * 1024 + 1)
        r = upload_paste_raw(ctx, content)
        assert r.status_code == 413

    def test_upload_utf8(self, ctx, upload_paste, get_paste_raw):
        text = "Привет мир! 🌍 こんにちは"
        r = upload_paste(ctx, text)
        r2 = get_paste_raw(ctx, r.paste_id)
        assert r2.json()['text'] == text

    def test_expires_in_field(self, ctx, upload_paste, get_paste_raw):
        now = datetime.now(timezone.utc)
        r = upload_paste(ctx, "Hello, world!", "3_month")
        r2 = get_paste_raw(ctx, r.paste_id)
        assert r2.status_code == 200

        created_at = isoparse(r2.json()['created_at'])
        expires_at = isoparse(r2.json()['expires_at'])
        assert (expires_at - created_at).total_seconds() == 60 * 60 * 24 * 30 * 3
        assert (created_at - now).total_seconds() <= 2

# ============================================
class TestGetPaste:
    def test_get_nonexistent(self, ctx, get_paste_raw):
        r = get_paste_raw(ctx, "nonexistent_id_xyz")
        assert r.status_code == 404

# ============================================
class TestDeletePaste:
    def test_delete_existing(self, ctx, upload_paste, get_paste_raw, delete_paste_raw):
        r = upload_paste(ctx, "to be deleted")
        r2 = delete_paste_raw(ctx, r.paste_id, r.delete_key)
        assert r2.status_code == 204
        time.sleep(0.5)
        r3 = get_paste_raw(ctx, r.paste_id)
        assert r3.status_code == 404

    def test_delete_nonexistent(self, ctx, delete_paste_raw):
        r = delete_paste_raw(ctx, "nonexistent_id_xyz", "abc")
        assert r.status_code == 204

    def test_double_delete(self, ctx, upload_paste, delete_paste_raw):
        r = upload_paste(ctx, "double delete test")
        r1 = delete_paste_raw(ctx, r.paste_id, r.delete_key)
        assert r1.status_code == 204
        r2 = delete_paste_raw(ctx, r.paste_id, r.delete_key)
        assert r2.status_code == 204

# ============================================
class TestCache:
    def test_cache_hit_on_second_request(self, ctx, upload_paste, get_paste_raw):
        r = upload_paste(ctx, "cache test")

        r2 = get_paste_raw(ctx, r.paste_id)
        assert r2.status_code == 200
        assert r2.headers.get("X-Cache-Status") in ("MISS", None)

        r3 = get_paste_raw(ctx, r.paste_id)
        assert r3.status_code == 200
        assert r3.headers.get("X-Cache-Status") == "HIT"

    def test_cache_invalidated_after_delete(self, ctx, upload_paste, get_paste_raw, delete_paste_raw):
        r = upload_paste(ctx, "cache invalidation test")

        get_paste_raw(ctx, r.paste_id)  # MISS
        r2 = get_paste_raw(ctx, r.paste_id)
        assert r2.headers.get("X-Cache-Status") == "HIT"
        delete_paste_raw(ctx, r.paste_id, r.delete_key)
        time.sleep(0.5)
        r3 = get_paste_raw(ctx, r.paste_id)
        assert r3.status_code == 404
        assert r3.headers.get("X-Cache-Status") != "HIT"

# ============================================
class TestRateLimiting:
    def test_upload_rate_limit(self, ctx, upload_paste_raw):
        responses = []
        for _ in range(50):
            r = upload_paste_raw(ctx, "rate limit test")
            responses.append(r.status_code)

        assert all(s in (200, 429) for s in responses)

# ============================================
# Helpers
# ============================================
@dataclass
class UploadPasteResult:
    paste_id: str
    delete_key: str

@pytest.fixture(scope="session")
def upload_paste(upload_paste_raw):
    def _upload_paste(ctx: Context, text: str = None, expires_in: str = None, **kwargs) -> UploadPasteResult:
        r = upload_paste_raw(ctx, text, expires_in, **kwargs)
        assert r.status_code == 200
        assert r.headers["Content-Type"].startswith("application/json")
        json = r.json()
        return UploadPasteResult(
            paste_id=json['id'],
            delete_key=json['delete_key'],
        )
    return _upload_paste

@pytest.fixture(scope="session")
def upload_paste_raw():
    def _upload_paste_raw(ctx: Context, text: str = None, expires_in: str = None, **kwargs) -> requests.Response:
        payload = {**kwargs}
        if text is not None:
            payload["text"] = text
        if expires_in is not None:
            payload["expires_in"] = expires_in
        return ctx.client.post('/api/v1/paste/', json=payload)
    return _upload_paste_raw

@pytest.fixture(scope="session")
def get_paste_raw():
    def _get_paste_raw(ctx: Context, paste_id: str, **kwargs) -> requests.Response:
        return ctx.client.get(f'/api/v1/{paste_id}', **kwargs)
    return _get_paste_raw

@pytest.fixture(scope="session")
def delete_paste_raw():
    def _delete_paste_raw(ctx: Context, paste_id: str, delete_key: str, **kwargs) -> requests.Response:
        payload = {"delete_key": delete_key, **kwargs}
        return ctx.client.delete(f'/api/v1/delete/{paste_id}', json=payload)
    return _delete_paste_raw