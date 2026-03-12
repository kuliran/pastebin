import pytest
from shared.utils.client import Client

@pytest.fixture
def api_patch_paste(auth_client, endpoints):
    async def _patch(
        paste_id: str,
        *,
        visibility: str = None,
        private_perms_add: list[str] = None,
        private_perms_rm: list[str] = None,
        client: Client = auth_client,
    ):
        json_body = {}
        if visibility is not None: json_body["visibility"] = visibility
        if private_perms_add is not None: json_body["private_perms_add"] = private_perms_add
        if private_perms_rm is not None: json_body["private_perms_rm"] = private_perms_rm

        response = await client.patch(endpoints['patch_paste'] + f'/{paste_id}', json=json_body)
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']
    return _patch