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

TIMEOUT = 10 # seconds for any request

# ============================================
class TestHealth:
    def test_ping(self, base_url, headers):
        r = requests.get(f"{base_url}/ping", headers=headers, timeout=TIMEOUT)
        assert r.status_code == 200

# ============================================
class TestUploadPaste:
    def test_upload_basic(self, upload_paste):
        r = upload_paste("Hello, World!")
        assert len(r.paste_id) > 0

    def test_upload_empty_text_fails(self, upload_paste_raw):
        r = upload_paste_raw("")
        assert r.status_code == 400

    def test_upload_without_text_field_fails(self, base_url, headers):
        r = requests.post(
            f"{base_url}/api/v1/paste/",
            headers=headers,
            json={},
            timeout=TIMEOUT,
        )
        assert r.status_code == 400

    def test_upload_1MB(self, upload_paste):
        content = "x" * (1024*1024)
        upload_paste(content)

    def test_upload_too_large_fails(self, upload_paste_raw):
        content = "x" * (1024*1024+1)
        r = upload_paste_raw(content)
        assert r.status_code == 413

    def test_upload_utf8(self, upload_paste, get_paste_raw):
        text = "Привет мир! 🌍 こんにちは"

        r = upload_paste(text)
        r2 = get_paste_raw(r.paste_id)
        assert r2.json()['text'] == text


# ============================================
class TestGetPaste:
    def test_get_nonexistent(self, get_paste_raw):
        r = get_paste_raw("nonexistent_id_xyz")
        assert r.status_code == 404

# ============================================
class TestDeletePaste:
    def test_delete_existing(self, upload_paste, get_paste_raw, delete_paste_raw):
        r = upload_paste("to be deleted")
        r2 = delete_paste_raw(r.paste_id, r.delete_key)
        assert r2.status_code == 204
        # Waiting for async purge
        time.sleep(0.5)
        r3 = get_paste_raw(r.paste_id)
        assert r3.status_code == 404

    def test_delete_nonexistent(self, delete_paste_raw):
        r = delete_paste_raw("nonexistent_id_xyz", "abc")
        assert r.status_code == 204

    def test_double_delete(self, upload_paste, delete_paste_raw):
        r = upload_paste("double delete test")

        r1 = delete_paste_raw(r.paste_id, r.delete_key)
        assert r1.status_code == 204
        r2 = delete_paste_raw(r.paste_id, r.delete_key)
        assert r2.status_code == 204

# ============================================
class TestCache:
    def test_cache_hit_on_second_request(self, upload_paste, get_paste_raw):
        r = upload_paste("cache test")

        r2 = get_paste_raw(r.paste_id)
        assert r2.status_code == 200
        assert r2.headers.get("X-Cache-Status") in ("MISS", None)

        r3 = get_paste_raw(r.paste_id)
        assert r3.status_code == 200
        assert r3.headers.get("X-Cache-Status") == "HIT"

    def test_cache_invalidated_after_delete(self, upload_paste, get_paste_raw, delete_paste_raw):
        r = upload_paste("cache invalidation test")

        get_paste_raw(r.paste_id) # MISS
        r2 = get_paste_raw(r.paste_id)
        assert r2.headers.get("X-Cache-Status") == "HIT"
        delete_paste_raw(r.paste_id, r.delete_key)
        # Waiting for async purge
        time.sleep(0.5)
        r3 = get_paste_raw(r.paste_id)
        assert r3.status_code == 404
        assert r3.headers.get("X-Cache-Status") != "HIT"

# ============================================
class TestRateLimiting:
    def test_upload_rate_limit(self, upload_paste_raw):
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
def upload_paste(upload_paste_raw):
    def _upload_paste(text: str, **kwargs) -> UploadPasteResult:
        r = upload_paste_raw(text, **kwargs)
        assert r.status_code == 200
        assert r.headers["Content-Type"].startswith("application/json")
        json = r.json()

        return UploadPasteResult(
            paste_id=json['id'],
            delete_key=json['delete_key'],
        )
    return _upload_paste

@pytest.fixture(scope="session")
def upload_paste_raw(base_url, headers):
    def _upload_paste_raw(text: str, **kwargs) -> requests.Response:
        payload = {"text": text, **kwargs}
        return requests.post(
            f"{base_url}/api/v1/paste/",
            headers=headers,
            json=payload,
            timeout=TIMEOUT,
        )
    return _upload_paste_raw

@pytest.fixture(scope="session")
def get_paste_raw(base_url, headers):
    def _get_paste_raw(paste_id: str, **kwargs) -> requests.Response:
        return requests.get(
            f"{base_url}/api/v1/{paste_id}",
            headers=headers,
            timeout=TIMEOUT,
            **kwargs
        )
    return _get_paste_raw

@pytest.fixture(scope="session")
def delete_paste_raw(base_url, headers):
    def _delete_paste_raw(paste_id: str, delete_key: str, **kwargs) -> requests.Response:
        payload = {"delete_key": delete_key, **kwargs}
        return requests.delete(
            f"{base_url}/api/v1/delete/{paste_id}",
            headers=headers,
            json=payload,
            timeout=TIMEOUT,
        )
    return _delete_paste_raw