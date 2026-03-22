#include "DbConnection.h"

#include "DbStatement.h"
#include "PreparedStatement.h"

DbConnection::DbConnection()
{
    // 1. Environment Handle 할당 및 버전 세팅
    if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &_hEnv) != SQL_SUCCESS)
    {
        LOG_ERROR(DB ,"Failed to allocate environment handle.");
        return;
    }
    SQLSetEnvAttr(_hEnv, SQL_ATTR_ODBC_VERSION, reinterpret_cast<SQLPOINTER>(SQL_OV_ODBC3), 0);

    // 2. Connection Handle 할당
    if (SQLAllocHandle(SQL_HANDLE_DBC, _hEnv, &_hDbc) != SQL_SUCCESS)
    {
        PrintError(SQL_HANDLE_ENV, _hEnv, "Failed to allocate connection handle.");
        return;
    }
}

DbConnection::~DbConnection()
{
    if (_hDbc != SQL_NULL_HANDLE)
    {
        SQLDisconnect(_hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, _hDbc);
    }
    if (_hEnv != SQL_NULL_HANDLE)
    {
        SQLFreeHandle(SQL_HANDLE_ENV, _hEnv);
    }
}

bool DbConnection::Connect(const std::string& connStr)
{
    if(_hEnv == SQL_NULL_HANDLE || _hDbc == SQL_NULL_HANDLE) return false;

    SQLCHAR outConnStr[1024];
    SQLSMALLINT outConnStrLen;

    SQLRETURN ret = SQLDriverConnectA(_hDbc, nullptr,
                                      (SQLCHAR*)connStr.c_str(), SQL_NTS,
                                      outConnStr, sizeof(outConnStr), &outConnStrLen,
                                      SQL_DRIVER_NOPROMPT);

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        LOG_INFO(DB, "DB Connected Successfully!");
        return true;
    }

    PrintError(SQL_HANDLE_DBC, _hDbc, "Connection Failed.");
    return false;
}

PreparedStatement DbConnection::CreatePreparedStatement(const std::string& query) const
{
    PreparedStatement ret(*this, query);
    return ret;
}

void DbConnection::PrintError(SQLSMALLINT handleType, SQLHANDLE handle, const std::string& msg)
{
    SQLCHAR sqlState[6], dbErrorMsg[SQL_MAX_MESSAGE_LENGTH];
    SQLINTEGER nativeError;
    SQLSMALLINT textLength;

    LOG_ERROR(DB, "[ODBC Error] ", msg);
    SQLSMALLINT i = 1;
    while (SQLGetDiagRecA(handleType, handle, i, sqlState, &nativeError,
                          dbErrorMsg, sizeof(dbErrorMsg), &textLength) == SQL_SUCCESS)
    {
        std::string_view state(reinterpret_cast<const char*>(sqlState), 5);
        std::string_view errMsg(reinterpret_cast<const char*>(dbErrorMsg), textLength);
        LOG_ERROR(DB, "State: {}, Error: {}, Message: {}" ,state, nativeError, errMsg);
        i++;
    }
}
