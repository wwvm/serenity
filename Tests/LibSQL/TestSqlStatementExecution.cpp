/*
 * Copyright (c) 2021, Jan de Visser <jan@de-visser.net>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <unistd.h>

#include <AK/ScopeGuard.h>
#include <LibSQL/AST/Parser.h>
#include <LibSQL/Database.h>
#include <LibSQL/Row.h>
#include <LibSQL/SQLResult.h>
#include <LibSQL/Value.h>
#include <LibTest/TestCase.h>

namespace {

constexpr const char* db_name = "/tmp/test.db";

RefPtr<SQL::SQLResult> execute(NonnullRefPtr<SQL::Database> database, String const& sql)
{
    auto parser = SQL::AST::Parser(SQL::AST::Lexer(sql));
    auto statement = parser.next_statement();
    EXPECT(!parser.has_errors());
    if (parser.has_errors()) {
        outln("{}", parser.errors()[0].to_string());
    }
    SQL::AST::ExecutionContext context { database };
    auto result = statement->execute(context);
    EXPECT(result->error().code == SQL::SQLErrorCode::NoError);
    return result;
}

void create_schema(NonnullRefPtr<SQL::Database> database)
{
    auto result = execute(database, "CREATE SCHEMA TestSchema;");
    EXPECT(result->inserted() == 1);
}

void create_table(NonnullRefPtr<SQL::Database> database)
{
    create_schema(database);
    auto result = execute(database, "CREATE TABLE TestSchema.TestTable ( TextColumn text, IntColumn integer );");
    EXPECT(result->inserted() == 1);
}

TEST_CASE(create_schema)
{
    ScopeGuard guard([]() { unlink(db_name); });
    auto database = SQL::Database::construct(db_name);
    create_schema(database);
    auto schema = database->get_schema("TESTSCHEMA");
    EXPECT(schema);
}

TEST_CASE(create_table)
{
    ScopeGuard guard([]() { unlink(db_name); });
    auto database = SQL::Database::construct(db_name);
    create_table(database);
    auto table = database->get_table("TESTSCHEMA", "TESTTABLE");
    EXPECT(table);
}

TEST_CASE(insert_into_table)
{
    ScopeGuard guard([]() { unlink(db_name); });
    auto database = SQL::Database::construct(db_name);
    create_table(database);
    auto result = execute(database, "INSERT INTO TestSchema.TestTable ( TextColumn, IntColumn ) VALUES ( 'Test', 42 );");
    EXPECT(result->inserted() == 1);

    auto table = database->get_table("TESTSCHEMA", "TESTTABLE");

    int count = 0;
    for (auto& row : database->select_all(*table)) {
        EXPECT_EQ(row["TEXTCOLUMN"].to_string(), "Test");
        EXPECT_EQ(row["INTCOLUMN"].to_int().value(), 42);
        count++;
    }
    EXPECT_EQ(count, 1);
}

TEST_CASE(select_from_table)
{
    ScopeGuard guard([]() { unlink(db_name); });
    auto database = SQL::Database::construct(db_name);
    create_table(database);
    auto result = execute(database, "INSERT INTO TestSchema.TestTable ( TextColumn, IntColumn ) VALUES ( 'Test_1', 42 ), ( 'Test_2', 43 );");
    EXPECT(result->inserted() == 2);
    result = execute(database, "INSERT INTO TestSchema.TestTable ( TextColumn, IntColumn ) VALUES ( 'Test_3', 44 ), ( 'Test_4', 45 );");
    EXPECT(result->inserted() == 2);
    result = execute(database, "INSERT INTO TestSchema.TestTable ( TextColumn, IntColumn ) VALUES ( 'Test_5', 46 );");
    EXPECT(result->inserted() == 1);
    result = execute(database, "SELECT * FROM TestSchema.TestTable;");
    EXPECT(result->has_results());
    EXPECT_EQ(result->results().size(), 5u);
}

}
