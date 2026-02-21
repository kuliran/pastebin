import pytest
from dataclasses import dataclass

@dataclass
class DeletePasteResult:
    pass

@pytest.fixture
async def api_delete_paste(service_client) -> DeletePasteResult:
    async def _get(paste_id: str, delete_key: str):
        response = await service_client.delete(f'/api/v1/delete/{paste_id}', json={"delete_key": delete_key})
        assert response.status == 204
        assert 'application/json' in response.headers['Content-Type']
        assert response.text == ''

        return DeletePasteResult()
    return _get
