import pytest
from shared.utils.client import Client

@pytest.fixture
async def api_delete_paste(api_delete_paste_raw, auth_client):
    async def _delete(paste_id: str, client: Client = auth_client):
        response = await api_delete_paste_raw(paste_id, client=client)
        assert response.status == 204
        assert 'application/json' in response.headers['Content-Type']
        assert response.text == ''
    return _delete

@pytest.fixture
async def api_delete_paste_raw(auth_client, endpoints):
    async def _delete(paste_id: str, client: Client = auth_client):
        return await client.delete(endpoints['delete_paste'](paste_id))
    return _delete