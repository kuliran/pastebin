# Start the tests via `make test-debug` or `make test-release`

import pytest
from dateutil.parser import isoparse
from fixtures.api_upload_paste import UploadResult

# =========================================
# ================= TESTS =================
# =========================================
async def test_basic(api_upload_paste):
    paste_text = 'Hello, world!'
    await api_upload_paste(paste_text)

async def test_utf8(api_upload_paste):
    paste_text = 'Привет, Андрей!'
    await api_upload_paste(paste_text)

async def test_max_size(api_upload_paste):
    paste_text = "a" * (1024*1024)
    await api_upload_paste(paste_text)

async def test_too_large(service_client):
    paste_text = "a" * (1024*1024+1)
    response = await service_client.post(f'/api/v1/paste/', json={"text": paste_text})
    assert response.status == 413

async def test_upload_and_get(api_upload_and_get_paste):
    paste_text = 'Hello, world!'
    await api_upload_and_get_paste(paste_text)

async def test_upload_and_get_utf8(api_upload_and_get_paste, mongo_collection):
    paste_text = 'Здравствуй, abc=.\n\t- Федор Васильевич?'
    assert mongo_collection.count_documents({}) == 0
    await api_upload_and_get_paste(paste_text)
    assert mongo_collection.count_documents({}) == 1

async def test_upload_k_and_get(api_upload_paste, api_get_paste, mongo_collection):
    ## Test that uploading a new paste doesn't spoil data of other pastes
    paste_texts = ['Hello, world!', 'Some different text']

    upload_results = []
    for x in range(0, len(paste_texts)):
        assert mongo_collection.count_documents({}) == x

        upload_results.append(await api_upload_paste(paste_texts[x]))
        assert mongo_collection.count_documents({}) == x+1

    for x in range(0, len(paste_texts)):
        get_response = await api_get_paste(upload_results[x].paste_id)
        assert_upload_and_get_results(upload_results[x], get_response, paste_texts[x])

# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
@pytest.fixture
async def api_upload_and_get_paste(api_upload_paste, api_get_paste):
    async def _upload(paste_text: str) -> UploadResult:
        upload_result = await api_upload_paste(paste_text)
        get_response = await api_get_paste(upload_result.paste_id)
        assert_upload_and_get_results(upload_result, get_response, paste_text)

        return upload_result
    return _upload

def assert_upload_and_get_results(upload_result, get_response, paste_text):
    assert get_response.created_at_utc == upload_result.pg_created_at_utc
    assert get_response.expires_at_utc == upload_result.pg_expires_at_utc
    assert get_response.size_bytes == len(upload_result.paste_text_utf)
    assert get_response.text == paste_text