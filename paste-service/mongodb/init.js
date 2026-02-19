const dbConn = db.getSiblingDB("pastes_db_1");

if (!dbConn.schema_migrations.findOne({ version: 1 })) {
  dbConn.pastes.createIndex(
    { expire_at: 1 },
    { expireAfterSeconds: 0, name: "expire_at_ttl" }
  );
  dbConn.schema_migrations.insertOne({ version: 1 });
}
