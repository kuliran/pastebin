import pytest
import shared.utils.auth as auth

@pytest.fixture
async def api_delete_paste(auth_client):
    async def _get(paste_id: str, delete_key: str, client: auth.AuthClient = auth_client):
        response = await client.delete(f'/api/v1/delete/{paste_id}', json={"delete_key": delete_key})
        assert response.status == 204
        assert 'application/json' in response.headers['Content-Type']
        assert response.text == ''
    return _get
