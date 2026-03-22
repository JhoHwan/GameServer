#pragma once
#include <sql.h>
#include <sqlext.h>

#include "LogManager.h"

class PreparedStatement;

class DbConnection
{
public:
    DbConnection();
    ~DbConnection();

    bool Connect(const std::string& connStr);

    [[nodiscard]] SQLHANDLE GetHandle() const {return _hDbc;}
    PreparedStatement CreatePreparedStatement(const std::string& query) const;

public:
    // ODBC 에러 메시지 추출 유틸리티
    static void PrintError(SQLSMALLINT handleType, SQLHANDLE handle, const std::string& msg);

private:
    SQLHENV _hEnv = SQL_NULL_HANDLE;
    SQLHDBC _hDbc = SQL_NULL_HANDLE;
};