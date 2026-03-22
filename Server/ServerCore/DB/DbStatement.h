#pragma once
#include <sql.h>
#include <sqltypes.h>

#include "DbConnection.h"
class DbStatement
{
    friend class DbConnection;
private:
    DbStatement(const DbConnection& db);

public:
    ~DbStatement()
    {
        if(_hStmt != SQL_NULL_HSTMT)
        {
            SQLFreeHandle(SQL_HANDLE_STMT, _hStmt);
        }
    }

    DbStatement(const DbStatement& other) = delete;
    DbStatement(DbStatement&& other) = delete;
    DbStatement& operator=(const DbStatement& other) = delete;
    DbStatement& operator=(DbStatement&& other) = delete;

public:
    bool Execute();
    bool Execute(const std::string& query);
    bool ExecuteInsert(const std::string& query);
    bool Prepare(const std::string& query);
    bool Bind(int32 index, int32& value);
    bool Bind(int32 index, std::string& value);


    bool Fetch();

    bool Get(int32 index, optional<int32>& value);
    bool Get(int32 index, optional<std::string>& value);

private:
    const DbConnection& _db;
    SQLHSTMT _hStmt = SQL_NULL_HSTMT;
};
