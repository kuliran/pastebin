# Start the tests via `make test-debug` or `make test-release`

import pytest
from fixtures.api_upload_paste import UploadFullResult

# =========================================
# ================= TESTS =================
# =========================================
async def test_basic(api_upload_full):
    paste_text = 'Hello, world!'
    await api_upload_full(paste_text)

async def test_upload_twice_overwrite_and_get(api_upload_create_url, s3_upload, api_upload_submit, api_upload_create_url_raw, raw_get_paste):
    paste_text = 'Hello, world!'

    create_url_res = await api_upload_create_url()
    await s3_upload(create_url_res.presigned_url, paste_text)
    second_upload = await s3_upload(create_url_res.presigned_url, 'Other text')
    await api_upload_submit(create_url_res.paste_id)

    get = await raw_get_paste(create_url_res.paste_id)
    assert get.text == second_upload.text_utf

async def test_create_url_limit(api_upload_create_url, s3_upload, api_upload_submit, api_upload_create_url_raw, api_upload_full, new_auth_client):
    paste_text = 'Hello, world!'

    diff_client = new_auth_client('abc-id')

    create_url_res = None
    for i in range(0,10):
        create_url_res = await api_upload_create_url()
    await s3_upload(create_url_res.presigned_url, paste_text)
    await api_upload_submit(create_url_res.paste_id)

    res = await api_upload_create_url_raw()
    assert res.status == 429

    await api_upload_full(paste_text, client=diff_client)

async def test_utf8(api_upload_full):
    paste_text = 'Привет, Андрей!'
    await api_upload_full(paste_text)

async def test_max_size(api_upload_full):
    paste_text = "a" * (1024*1024)
    await api_upload_full(paste_text, '1_week')

async def test_too_large(api_upload_create_url, s3_upload, api_upload_submit_raw):
    paste_text = "a" * (1024*1024+1)
    
    create_url_res = await api_upload_create_url()
    await s3_upload(create_url_res.presigned_url, paste_text)
    res = await api_upload_submit_raw(create_url_res.paste_id)
    assert res.status == 413

# async def test_upload_and_get(upload_and_get_paste):
#     paste_text = 'Hello, world!'
#     await upload_and_get_paste(paste_text)

# async def test_upload_and_get_utf8(upload_and_get_paste, mongo_collection):
#     paste_text = 'Здравствуй, abc=.\n\t- Федор Васильевич?'
#     assert mongo_collection.count_documents({}) == 0
#     await upload_and_get_paste(paste_text)
#     assert mongo_collection.count_documents({}) == 1

# async def test_upload_k_and_get(api_upload_paste, raw_get_paste, mongo_collection):
#     ## Test that uploading a new paste doesn't spoil data of other pastes
#     paste_texts = ['Hello, world!', 'Some different text']

#     upload_results = []
#     for x in range(0, len(paste_texts)):
#         assert mongo_collection.count_documents({}) == x

#         upload_results.append(await api_upload_paste(paste_texts[x]))
#         assert mongo_collection.count_documents({}) == x+1

#     for x in range(0, len(paste_texts)):
#         get_response = await raw_get_paste(upload_results[x].paste_id)
#         assert_upload_and_get_results(upload_results[x], get_response, paste_texts[x])

# async def test_lifetimes(api_upload_paste):
#     paste_text = 'Hello, world!'
#     lifetimes = [None, '1_hour', '1_day', '1_week', '1_month', '3_month']

#     for x in lifetimes:
#         await api_upload_paste(paste_text, x)

# =========================================
# ============= LOCAL FIXTURES ============
# =========================================
# @pytest.fixture
# async def upload_and_get_paste(api_upload_paste, raw_get_paste):
#     async def _upload(paste_text: str) -> UploadResult:
#         upload_result = await api_upload_paste(paste_text)
#         get_response = await raw_get_paste(upload_result.paste_id)
#         assert_upload_and_get_results(upload_result, get_response, paste_text)

#         return upload_result
#     return _upload

def assert_upload_and_get_results(upload_result, get_response, paste_text):
    assert get_response.created_at_utc == upload_result.pg_created_at_utc
    assert get_response.expires_at_utc == upload_result.pg_expires_at_utc
    assert get_response.size_bytes == len(upload_result.paste_text_utf)
    assert get_response.text == paste_text