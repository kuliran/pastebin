import time
from dataclasses import dataclass
import pytest
import requests
from dateutil.parser import isoparse
from datetime import datetime, timezone
from utils.auth import AuthClient

USERNAME = 'test_api'
PASSWORD = 'test_api'

@dataclass
class Context:
    client: AuthClient

@pytest.fixture(scope='session')
def ctx(auth_client) -> Context:
    return Context(client=auth_client(USERNAME, PASSWORD))

# ============================================
def test_upload_basic(upload_paste):
    r = upload_paste("Hello, World!")
    assert len(r.paste_id) > 0

def test_upload_empty_text_fails(upload_paste_raw):
    r = upload_paste_raw("")
    assert r.status_code == 400

def test_upload_without_text_field_fails(upload_paste_raw):
    r = upload_paste_raw(json={})
    assert r.status_code == 400

def test_upload_1MB(upload_paste):
    content = "x" * (1024 * 1024)
    upload_paste(content)

def test_upload_too_large_fails(upload_paste_raw):
    content = "x" * (1024 * 1024 + 1)
    r = upload_paste_raw(content)
    assert r.status_code == 413

def test_upload_utf8(upload_paste, get_paste_raw):
    text = "Привет мир! 🌍 こんにちは"
    r = upload_paste(text)
    r2 = get_paste_raw(r.paste_id)
    assert r2.json()['text'] == text

def test_expires_in_field(upload_paste, get_paste_raw):
    now = datetime.now(timezone.utc)
    r = upload_paste("Hello, world!", "3_month")
    r2 = get_paste_raw(r.paste_id)
    assert r2.status_code == 200

    created_at = isoparse(r2.json()['created_at'])
    expires_at = isoparse(r2.json()['expires_at'])
    assert (expires_at - created_at).total_seconds() == 60 * 60 * 24 * 30 * 3
    assert (created_at - now).total_seconds() <= 2

# ============================================
def test_get_nonexistent(get_paste_raw):
    r = get_paste_raw("nonexistent_id_xyz")
    assert r.status_code == 404

# ============================================
def test_delete_existing(upload_paste, get_paste_raw, delete_paste_raw):
    r = upload_paste("to be deleted")
    r2 = delete_paste_raw(r.paste_id, r.delete_key)
    assert r2.status_code == 204
    time.sleep(0.5)
    r3 = get_paste_raw(r.paste_id)
    assert r3.status_code == 404

def test_delete_nonexistent(delete_paste_raw):
    r = delete_paste_raw("nonexistent_id_xyz", "abc")
    assert r.status_code == 204

def test_double_delete(upload_paste, delete_paste_raw):
    r = upload_paste("double delete test")
    r1 = delete_paste_raw(r.paste_id, r.delete_key)
    assert r1.status_code == 204
    r2 = delete_paste_raw(r.paste_id, r.delete_key)
    assert r2.status_code == 204

# ============================================
def test_cache_hit_on_second_request(upload_paste, get_paste_raw):
    r = upload_paste("cache test")

    r2 = get_paste_raw(r.paste_id)
    assert r2.status_code == 200
    assert r2.headers.get("X-Cache-Status") in ("MISS", None)

    r3 = get_paste_raw(r.paste_id)
    assert r3.status_code == 200
    assert r3.headers.get("X-Cache-Status") == "HIT"

def test_cache_invalidated_after_delete(upload_paste, get_paste_raw, delete_paste_raw):
    r = upload_paste("cache invalidation test")

    get_paste_raw(r.paste_id)  # MISS
    r2 = get_paste_raw(r.paste_id)
    assert r2.headers.get("X-Cache-Status") == "HIT"
    delete_paste_raw(r.paste_id, r.delete_key)
    time.sleep(0.5)
    r3 = get_paste_raw(r.paste_id)
    assert r3.status_code == 404
    assert r3.headers.get("X-Cache-Status") != "HIT"

# ============================================
def test_upload_rate_limit(upload_paste_raw):
    responses = []
    for _ in range(50):
        r = upload_paste_raw("rate limit test")
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
def upload_paste(upload_paste_raw, ctx):
    def _upload_paste(text: str = None, expires_in: str = None, ctx: Context = ctx, **kwargs) -> UploadPasteResult:
        r = upload_paste_raw(text, expires_in, ctx=ctx, **kwargs)
        assert r.status_code == 200
        assert r.headers["Content-Type"].startswith("application/json")
        json = r.json()
        return UploadPasteResult(
            paste_id=json['id'],
            delete_key=json['delete_key'],
        )
    return _upload_paste

@pytest.fixture(scope="session")
def upload_paste_raw(ctx):
    def _upload_paste_raw(text: str = None, expires_in: str = None, ctx: Context = ctx, **kwargs) -> requests.Response:
        payload = {**kwargs}
        if text is not None:
            payload["text"] = text
        if expires_in is not None:
            payload["expires_in"] = expires_in
        return ctx.client.post('/api/v1/paste/', json=payload)
    return _upload_paste_raw

@pytest.fixture(scope="session")
def get_paste_raw(ctx):
    def _get_paste_raw(paste_id: str, ctx: Context = ctx, **kwargs) -> requests.Response:
        return ctx.client.get(f'/api/v1/{paste_id}', **kwargs)
    return _get_paste_raw

@pytest.fixture(scope="session")
def delete_paste_raw(ctx):
    def _delete_paste_raw(paste_id: str, delete_key: str, ctx: Context = ctx, **kwargs) -> requests.Response:
        payload = {"delete_key": delete_key, **kwargs}
        return ctx.client.delete(f'/api/v1/delete/{paste_id}', json=payload)
    return _delete_paste_raw