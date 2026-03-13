import pytest
import utils.auth as auth

@pytest.fixture
def auth_refresh(endpoints, service_client):
    async def _auth_refresh(refresh_tk: str) -> auth.JwtTokens:
        response = await service_client.post(endpoints['auth_refresh'](), cookies={"refresh_tk": refresh_tk})
        assert response.status == 200
        return auth.validate_response_tokens(response)
    return _auth_refresh

@pytest.fixture
def auth_refresh_expect_fail(endpoints, service_client):
    async def _auth_refresh_expect_fail(refresh_tk: str):
        response = await service_client.post(endpoints['auth_refresh'](), cookies={"refresh_tk": refresh_tk})
        assert response.status == 401
    return _auth_refresh_expect_fail
