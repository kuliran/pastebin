import pytest
import time
from dateutil.parser import isoparse
from datetime import datetime, timezone
from shared.utils.client import Client
from shared.utils.upload_paste import get_paste_id

# ============================================
async def test_upload_basic(api_upload_paste):
    r = await api_upload_paste("Hello, World!")
    assert len(r.paste_id) > 0

async def test_upload_empty_text_fails(api_upload_create_url_raw, s3_upload, api_upload_submit_raw):
    create_url = await api_upload_create_url_raw()
    presigned_url = create_url.json()['presigned_url']
    paste_id = get_paste_id(presigned_url)
    await s3_upload(presigned_url, "")
    r = await api_upload_submit_raw(paste_id)
    assert r.status == 409

async def test_upload_1MB(api_upload_paste):
    text = "x" * (1024 * 1024)
    await api_upload_paste(text)

async def test_upload_too_large_fails(api_upload_create_url_raw, s3_upload, api_upload_submit_raw):
    text = "x" * (1024 * 1024 + 1)

    create_url = await api_upload_create_url_raw()
    presigned_url = create_url.json()['presigned_url']
    paste_id = get_paste_id(presigned_url)
    await s3_upload(presigned_url, text)
    r = await api_upload_submit_raw(paste_id)
    assert r.status == 413

async def test_upload_utf8(api_upload_paste, api_get_paste):
    text = "Привет мир! 🌍 こんにちは"
    r = await api_upload_paste(text)
    r2 = await api_get_paste(r.paste_id)
    assert r2.data == r.data

async def test_expires_in_field(api_upload_paste, api_get_paste):
    now = datetime.now(timezone.utc)
    r = await api_upload_paste("Hello, world!", expires_in="3_month")
    r2 = await api_get_paste(r.paste_id)

    assert (r2.expires_at_utc - r2.created_at_utc).total_seconds() == 60 * 60 * 24 * 30 * 3
    assert (r2.created_at_utc - now).total_seconds() <= 2

async def test_unauthorized_upload_fails(api_upload_create_url_raw, new_unauth_client):
    unauth = new_unauth_client()
    r = await api_upload_create_url_raw(client=unauth)
    assert r.status in (400, 401, 403)

async def test_upload_twice_and_get(api_upload_create_url_raw, s3_upload, api_upload_submit_raw, api_get_paste):
    paste_text = 'Hello, world!'

    create_url = await api_upload_create_url_raw()
    presigned_url = create_url.json()['presigned_url']
    paste_id = get_paste_id(presigned_url)
    first_upload = await s3_upload(presigned_url, paste_text)
    r = await api_upload_submit_raw(paste_id)
    assert r.status == 200
    await s3_upload(presigned_url, 'Other text')

    get = await api_get_paste(paste_id)
    assert get.data == first_upload.data

# # ============================================
async def test_get_nonexistent(api_get_paste_expect_none):
    await api_get_paste_expect_none("nonexistent_id_xyz")

# # ============================================
async def test_delete_existing(api_upload_paste, api_get_paste_expect_none, api_delete_paste):
    r = await api_upload_paste("to be deleted")
    await api_delete_paste(r.paste_id)
    await api_get_paste_expect_none(r.paste_id)

async def test_delete_nonexistent(api_delete_paste_raw):
    r = await api_delete_paste_raw("nonexistent_id_xyz")
    assert r.status == 204

async def test_double_delete(api_upload_paste, api_delete_paste):
    r = await api_upload_paste("double delete test")
    await api_delete_paste(r.paste_id)
    await api_delete_paste(r.paste_id)

async def test_unauthorized_delete_fails(api_upload_paste, api_delete_paste_raw, new_unauth_client):
    r = await api_upload_paste("Hello, world!")

    unath = new_unauth_client()
    r2 = await api_delete_paste_raw(r.paste_id, client=unath)
    assert r2.status in (400, 401, 403)

async def test_diff_user_delete_fails(api_upload_paste, api_delete_paste_raw, new_auth_client):
    r = await api_upload_paste("Hello, world!")
    
    diff = await new_auth_client("diff_user_delete_test", "diff_user_delete_test")
    r2 = await api_delete_paste_raw(r.paste_id, client=diff)
    assert r2.status in (401, 403)

# # ============================================
async def test_upload_rate_limit(api_upload_create_url_raw):
    responses = []
    for _ in range(50):
        r = await api_upload_create_url_raw()
        responses.append(r.status)

    assert all(s in (201, 429) for s in responses)
