"""
e2e tests
Run from project root dir with:
    make e2e

optionally, change the E2E_HOST (must match the one in nginx/conf.d/paste-service.conf) and E2E_URL:
    E2E_URL=http://localhost  E2E_HOST=pastebin.io  make e2e
"""

import sys
import pathlib

REPO_ROOT = pathlib.Path(__file__).parent.parent.parent
sys.path.insert(0, str(REPO_ROOT / 'shared' / 'pytest'))

pytest_plugins = [
    'utils.api',
    'utils.auth',
]
