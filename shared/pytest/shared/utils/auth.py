from dataclasses import dataclass

@dataclass
class JwtTokens:
    access_tk: str
    refresh_tk: str

def refresh_path():
    return '/api/v2/auth/refresh'

def validate_response_tokens(response) -> JwtTokens:
    assert 'application/json' in response.headers['Content-Type']

    json = response.json()
    assert type(json['access_tk']) is str

    assert response.cookies['refresh_tk']
    cookie = response.cookies.get('refresh_tk')
    assert cookie['httponly']
    assert cookie['path'] == refresh_path()

    return JwtTokens(
        access_tk=json['access_tk'],
        refresh_tk=response.cookies['refresh_tk'].value
    )