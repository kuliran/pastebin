import pytest
import shared.utils.auth as auth

@pytest.fixture
def auth_refresh(service_client):
    async def _auth_refresh(refresh_tk: str) -> auth.JwtTokens:
        response = await service_client.post(auth.refresh_path(), headers={"Cookie": f"refresh_tk={refresh_tk}"})
        assert response.status == 200
        
        return auth.validate_response_tokens(response)
    return _auth_refresh

@pytest.fixture
def auth_refresh_expect_fail(service_client):
    async def _auth_refresh_expect_fail(refresh_tk: str):
        response = await service_client.post(auth.refresh_path(), headers={"Cookie": f"refresh_tk={refresh_tk}"})
        assert response.status == 401
    return _auth_refresh_expect_fail
