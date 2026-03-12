import pytest

@pytest.fixture(scope='session')
def endpoints():
    return {
        'auth_signup': '/api/v2/auth/signup',
        'auth_login': '/api/v2/auth/login',
        'auth_refresh': '/api/v2/auth/refresh',
        'get_paste_presigned_url': '/api/v2/paste',
        'upload_paste_create_url': '/api/v2/paste/create-url',
        'upload_paste_submit': '/api/v2/paste/submit',
        'delete_paste': '/api/v2/paste/delete',
        'patch_paste': '/api/v2/paste/patch'
    }