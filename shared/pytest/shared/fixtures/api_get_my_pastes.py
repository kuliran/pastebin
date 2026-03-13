import pytest
from dataclasses import dataclass
from dateutil.parser import isoparse
from datetime import datetime
from shared.utils.client import Client

@dataclass
class UserPaste:
    id: str
    visibility: str
    created_at_utc: datetime

@pytest.fixture
async def api_get_my_pastes(auth_client, endpoints):
    async def _get(*, client: Client = auth_client) -> list[UserPaste]:
        response = await client.get(endpoints['get_my_pastes'])
        assert response.status == 200
        assert 'application/json' in response.headers['Content-Type']

        json = response.json()
        assert type(json) is list

        pastes = []
        for paste in json:
            pastes.append(UserPaste(
                id = paste['id'],
                visibility = paste['visibility'],
                created_at_utc = isoparse(paste['created_at']),
            ))
        return pastes
    return _get
