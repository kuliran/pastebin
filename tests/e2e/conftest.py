import pytest
import time
import requests
import os

BASE_URL = os.getenv("E2E_BASE_URL", "http://localhost")

@pytest.fixture(scope="session")
def base_url():
    return BASE_URL

@pytest.fixture(scope="session", autouse=True)
def wait_for_service(base_url):
    max_retries = 15
    for i in range(max_retries):
        try:
            r = requests.get(f"{base_url}/ping", timeout=3)
            if r.status_code == 200:
                print(f"\nService ready at {base_url}")
                return
        except requests.ConnectionError:
            pass
        print(f"\tWaiting... ({i+1}/{max_retries})")
        time.sleep(1)
    pytest.fail(f"Service at {base_url} is not available")