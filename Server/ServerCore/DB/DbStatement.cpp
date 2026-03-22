#include "DbStatement.h"

#include "DbConnection.h"

DbStatement::DbStatement(const DbConnection& db): _db(db)
{
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, db.GetHandle(), &_hStmt);
    if (ret != SQL_SUCCESS)
    {
        DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "DbStatement Init Fail");
    }
}

bool DbStatement::Execute()
{
    auto ret = SQLExecute(_hStmt);
    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        return true;
    }
    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "SQLExecute Fail");
    return false;
}

bool DbStatement::Execute(const std::string& query)
{

    SQLRETURN ret = SQLExecDirectA(_hStmt, (SQLCHAR*) query.c_str(), SQL_NTS);
    if(ret != SQL_SUCCESS)
    {
        DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "SQLExecDirectA Fail");
        return false;
    }

    return true;
}

bool DbStatement::ExecuteInsert(const std::string& query)
{
    auto ret = SQLExecDirectA(_hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        SQLLEN count{0};
        SQLRowCount(_hStmt, &count);
        LOG_INFO(DB, "Inserted Rows: {}", count);
        return true;
    }
    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "ExecuteInsert Fail");
    return false;
}

bool DbStatement::Prepare(const std::string& query)
{
    auto ret = SQLPrepareA(_hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        return true;
    }

    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "SQLPrepare Fail");
    return false;
}

bool DbStatement::Bind(int32 index, int32& value)
{
    static SQLLEN cbLen = 0;
    auto ret = SQLBindParameter(
        _hStmt,
        index,
        SQL_PARAM_INPUT,
        SQL_C_SLONG,
        SQL_INTEGER,
        0,
        0,
        &value,
        0,
        &cbLen);
    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        return true;
    }

    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "BindParameter Fail");
    return false;
}

bool DbStatement::Bind(int32 index, std::string& value)
{
    static SQLLEN cbLen = SQL_NTS;

    SQLRETURN ret = SQLBindParameter(
        _hStmt,
        index,
        SQL_PARAM_INPUT,
        SQL_C_CHAR,
        SQL_VARCHAR,
        value.size(),
        0,
        (SQLPOINTER)value.data(),
        0,
        &cbLen);

    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        return true;
    }

    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "BindParameter Fail");
    return false;
}

bool DbStatement::Fetch()
{
    SQLRETURN ret = SQLFetch(_hStmt);
    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) return true;
    if(ret == SQL_NO_DATA) return false;

    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "SQLFetch Fail");
    return false;
}

bool DbStatement::Get(int32 index, std::optional<int32>& value)
{
    SQLLEN len{0};
    int32 data{0};
    SQLRETURN ret = SQLGetData(_hStmt, index, SQL_C_SLONG, &data, 0, &len);

    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        if(len == SQL_NULL_DATA) value = std::nullopt;
        else value = data;
        return true;
    }

    value = std::nullopt;
    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "SQLGetData Fail");
    return false;
}

bool DbStatement::Get(int32 index, optional<std::string>& value)
{
    SQLLEN len{0};
    char buffer[256];
    SQLRETURN ret = SQLGetData(_hStmt, index, SQL_C_CHAR, buffer, 256, &len);

    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        if(len == SQL_NULL_DATA) value = std::nullopt;
        else value = std::make_optional<std::string>(buffer, len);
        return true;
    }

    value = std::nullopt;
    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "SQLGetData Fail");
    return false;
}
