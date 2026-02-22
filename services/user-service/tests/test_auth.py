import pytest

# =========================================
# ================= TESTS =================
# =========================================
async def test_signup_and_refresh(signup, auth_refresh, auth_refresh_expect_fail):
    username = 'bob'
    password = 'bob123'
    r = await signup(username, password)

    r2 = await auth_refresh(r.refresh_tk)
    await auth_refresh_expect_fail(r.refresh_tk)
    await auth_refresh(r2.refresh_tk)