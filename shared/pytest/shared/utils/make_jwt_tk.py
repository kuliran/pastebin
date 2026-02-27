import pathlib
from datetime import datetime, timezone, timedelta
import jwt

SHARED_DIR = pathlib.Path(__file__).parent.parent.parent.parent
PRIVATE_KEY = pathlib.Path(str(SHARED_DIR / 'keys' / 'dev' / 'private.pem')).read_text()

def make_jwt_tk(user_id: str) -> str:
    return jwt.encode(
        {
            'sub': user_id,
            'iss': 'user-service',
            'iat': datetime.now(timezone.utc),
            'exp': datetime.now(timezone.utc) + timedelta(hours=24),
        },
        PRIVATE_KEY,
        algorithm='RS256'
    )