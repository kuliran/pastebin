# Start the tests via `make test-debug` or `make test-release`

import pytest
from shared.utils.upload_paste import UploadPasteResult
from shared.utils.client import Client

# =========================================
# ================= TESTS =================
# =========================================
async def test_basic(api_upload_paste):
    paste_text = 'Hello, world!'
    await api_upload_paste(paste_text)

async def test_upload_twice_overwrite_and_get(api_upload_create_url, s3_upload, api_upload_submit, raw_get_paste):
    paste_text = 'Hello, world!'

    create_url_res = await api_upload_create_url()
    await s3_upload(create_url_res.presigned_url, paste_text)
    second_upload = await s3_upload(create_url_res.presigned_url, 'Other text')
    await api_upload_submit(create_url_res.paste_id)

    get = await raw_get_paste(create_url_res.paste_id)
    assert get.data == second_upload.data

async def test_create_url_limit(api_upload_create_url, s3_upload, api_upload_submit,
    api_upload_create_url_raw, api_upload_paste, new_auth_client, service_static_config
):
    paste_text = 'Hello, world!'

    create_url_limit = service_static_config['components_manager']['components'] \
        .get('metadata-repo') \
        .get('upload_rate_limit_create_url_max')
    diff_client = new_auth_client('other-user-id')

    create_url_res = None
    for i in range(0,create_url_limit):
        create_url_res = await api_upload_create_url()
    await s3_upload(create_url_res.presigned_url, paste_text)
    await api_upload_submit(create_url_res.paste_id)

    res = await api_upload_create_url_raw()
    assert res.status == 429

    await api_upload_paste(paste_text, client=diff_client)

async def test_utf8(api_upload_paste):
    paste_text = 'Привет мир! 🌍 こんにちは'
    await api_upload_paste(paste_text)

async def test_max_size(api_upload_paste):
    paste_text = "a" * (1024*1024)
    await api_upload_paste(paste_text, expires_in='1_week')

async def test_too_large(api_upload_create_url, s3_upload, api_upload_submit_raw):
    paste_text = "a" * (1024*1024+1)
    
    create_url_res = await api_upload_create_url()
    await s3_upload(create_url_res.presigned_url, paste_text)
    res = await api_upload_submit_raw(create_url_res.paste_id)
    assert res.status == 413

async def test_upload_and_get(upload_and_get_paste):
    paste_text = 'Hello, world!'
    await upload_and_get_paste(paste_text)

async def test_upload_and_get_utf8(upload_and_get_paste):
    paste_text = 'Привет мир! 🌍 こんにちは'
    await upload_and_get_paste(paste_text)

async def test_upload_k_and_get(api_upload_paste, raw_get_paste):
    ## Test that uploading a new paste doesn't spoil data of other pastes
    paste_texts = ['Hello, world!', 'Some different text']

    upload_results = []
    for x in range(0, len(paste_texts)):
        upload_results.append(await api_upload_paste(paste_texts[x]))
        
    for x in range(0, len(paste_texts)):
        get = await raw_get_paste(upload_results[x].paste_id)
        assert_upload_and_get_results(upload_results[x], get, paste_texts[x])

async def test_lifetimes(api_upload_paste):
    paste_text = 'Hello, world!'
    lifetimes = [None, '1_hour', '1_day', '1_week', '1_month', '3_month']

    for x in lifetimes:
        await api_upload_paste(paste_text, expires_in=x)

# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
@pytest.fixture
async def upload_and_get_paste(api_upload_paste, raw_get_paste, auth_client):
    async def _upload(paste_text: str, *, client: Client = auth_client) -> UploadPasteResult:
        upload_res = await api_upload_paste(paste_text, client=client)
        get_res = await raw_get_paste(upload_res.paste_id)
        assert_upload_and_get_results(upload_res, get_res, paste_text)

        return upload_res
    return _upload

def assert_upload_and_get_results(upload_result, get_response, paste_text):
    assert get_response.created_at_utc == upload_result.created_at_utc
    assert get_response.expires_at_utc == upload_result.expires_at_utc
    assert get_response.size_bytes == upload_result.size_bytes
    assert get_response.data == paste_text.encode("utf-8")