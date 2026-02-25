import pytest
import shared.utils.auth as auth

@pytest.fixture
async def api_delete_paste(auth_client, endpoints):
    async def _get(paste_id: str, client: auth.Client = auth_client):
        response = await client.delete(endpoints['delete_paste'] + f'/{paste_id}')
        assert response.status == 204
        assert 'application/json' in response.headers['Content-Type']
        assert response.text == ''
    return _get
