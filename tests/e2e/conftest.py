"""
e2e tests
Run from project root dir with:
    make e2e

optionally, change the E2E_HOST (must match the one in nginx/conf.d/paste-service.conf) and E2E_URL:
    E2E_URL=http://localhost  E2E_HOST=pastebin.io  make e2e

All tests run with a clean pg users.accounts table
However, other DB data including S3 content, and nginx state are not reset
"""

import pytest
import sys
import pathlib
import os
import asyncpg
from utils.api import ApiSession

BASE_URL = os.getenv("E2E_BASE_URL", "http://localhost")
SERVICE_HOST = os.getenv("E2E_HOST", "pastebin.io")
POSTGRES_DSN = os.getenv("POSTGRES_CONNECTION")

TIMEOUT = 5  # seconds for each request

REPO_ROOT = pathlib.Path(__file__).parent.parent.parent
sys.path.insert(0, str(REPO_ROOT / 'shared' / 'pytest'))

pytest_plugins = [
    'shared.fixtures.endpoints',
    'shared.fixtures.api_get_paste',
    'shared.fixtures.api_upload_paste_raw',
    'shared.fixtures.api_delete_paste',
    'shared.fixtures.api_patch_paste',
    'shared.fixtures.api_get_paste_details',
    'shared.fixtures.api_get_my_pastes',
    'fixtures.auth',
    'fixtures.api_upload_paste',
]

@pytest.fixture(scope='session')
def api():
    return ApiSession(
        base_url=BASE_URL,
        host=SERVICE_HOST,
        timeout=TIMEOUT,
    )


@pytest.fixture(autouse=True)
async def clear_users_table():
    conn = await asyncpg.connect(dsn=POSTGRES_DSN)
    try:
        await conn.execute("TRUNCATE TABLE users.accounts RESTART IDENTITY CASCADE;")
    finally:
        await conn.close()