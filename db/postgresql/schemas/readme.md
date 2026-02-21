This directory has init schemas that should represent the final DB state after all migrations **FOR TESTING ONLY**.<br>
That's the only way `testsuite` wants to work

Update with:
```bash
# start DB and then:
pg_dump --schema-only -d YOUR_DB_NAME > tests/schemas/YOUR_TEST_DB_NAME_SUFFIX.sql
```

For production, use `../migrations`