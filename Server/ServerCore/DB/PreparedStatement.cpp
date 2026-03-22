#include "PreparedStatement.h"

#include "DbConnection.h"

PreparedStatement::PreparedStatement(const DbConnection& conn, const std::string& query) : _conn(conn)
{
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, _conn.GetHandle(), &_hStmt);
    if (ret != SQL_SUCCESS)
    {
        DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "PreparedStatement Init Fail");
        return;
    }

    ret = SQLPrepareA(_hStmt, (SQLCHAR*)query.c_str(), SQL_NTS);
    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        _isPrepared = true;
        return;
    }

    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "PreparedStatement Init Fail");
}

PreparedStatement::~PreparedStatement()
{
    if(_hStmt != SQL_NULL_HSTMT)
    {
        SQLFreeHandle(SQL_HANDLE_STMT, _hStmt);
    }
}

PreparedStatement::PreparedStatement(PreparedStatement&& other) noexcept
    : _conn(other._conn), _hStmt(other._hStmt), _isPrepared(other._isPrepared)
{
    other._isPrepared = false;
    other._hStmt = SQL_NULL_HSTMT;
}


