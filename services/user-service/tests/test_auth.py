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

async def test_signup_login_refresh(signup, login, auth_refresh, auth_refresh_expect_fail):
    username = 'bob'
    password = 'bob123'

    # 2 sessions
    s = await signup(username, password)
    l = await login(username, password)

    # refresh both
    s2 = await auth_refresh(s.refresh_tk)
    l2 = await auth_refresh(l.refresh_tk)

    # old tokens fail
    await auth_refresh_expect_fail(l.refresh_tk)
    await auth_refresh_expect_fail(s.refresh_tk)

    # double refresh
    await auth_refresh(s2.refresh_tk)
    await auth_refresh(l2.refresh_tk)