import pytest
from shared.utils.client import Client

@pytest.fixture
def api_patch_paste(auth_client):
    async def _patch(
        paste_id: str,
        *,
        visibility: str = 'public',
        private_perms_add: [str] = [],
        private_perms_rm: [str] = [],
        client: Client = auth_client,
    ):
        response = await client.patch(endpoints['patch_paste'] + f'/{paste_id}')
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']
    return _patch