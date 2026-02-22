import pytest

# =========================================
# ================= TESTS =================
# =========================================
async def test_signup(signup):
    username = 'bob'
    password = 'bob123'
    await signup(username, password)

