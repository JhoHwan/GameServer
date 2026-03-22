#pragma once
#include <cstring>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <vector>

#include "DbConnection.h"
#include "Types.h"


template <typename T> struct DbTypeTraits;
template <> struct DbTypeTraits<int32>
{
    static constexpr SQLSMALLINT cType = SQL_C_SLONG;
    static constexpr SQLSMALLINT sqlType = SQL_INTEGER;
};

template <> struct DbTypeTraits<float>
{
    static constexpr SQLSMALLINT cType = SQL_C_FLOAT;
    static constexpr SQLSMALLINT sqlType = SQL_REAL;
};;

class PreparedStatement
{
    friend class DbConnection;
private:
    PreparedStatement(const DbConnection& conn, const std::string& query);

public:
    ~PreparedStatement();
    PreparedStatement(const PreparedStatement& other) = delete;
    PreparedStatement& operator=(const PreparedStatement& other) = delete;

    PreparedStatement(PreparedStatement&& other) noexcept;
    PreparedStatement& operator=(PreparedStatement&& other) = delete;

    [[nodiscard]] bool IsPrepared() const {return _isPrepared;}

    template <typename... Args>
    bool Execute(Args&... args);

    template<typename... Args>
    bool Fetch(Args&... args);

private:
    template<typename T>
    bool BindParam(SQLSMALLINT index, T& value);

    bool BindParam(SQLSMALLINT index, std::string& value);

    template<typename T>
    bool BindCol(SQLSMALLINT index, T& value);

    template<typename T>
    bool BindCol(SQLSMALLINT index, std::optional<T>& value);

    bool BindCol(SQLSMALLINT index, std::string& value, size_t maxLen = 256);

    template<typename T>
    void PostFetch(SQLSMALLINT index, T& value) {}

    template<typename T>
    void PostFetch(SQLSMALLINT index, std::optional<T>& value);

    void PostFetch(SQLSMALLINT index, std::string& value);

private:
    const DbConnection& _conn;
    SQLHSTMT _hStmt = SQL_NULL_HSTMT;
    bool _isPrepared = false;
    std::vector<SQLLEN> _paramLens;
    std::vector<SQLLEN> _fetchLens;
};

template<typename ... Args>
bool PreparedStatement::Execute(Args&... args)
{
    if(!_isPrepared) return false;

    SQLFreeStmt(_hStmt, SQL_CLOSE);
    SQLFreeStmt(_hStmt, SQL_RESET_PARAMS);

    size_t paramCount = sizeof...(Args);
    _paramLens.assign(paramCount, 0);

    SQLSMALLINT index = 1;

    if (!(BindParam(index++, args) && ...))
    {
        return false;
    }

    SQLRETURN ret = SQLExecute(_hStmt);
    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        return true;
    }

    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "PreparedStatement Execute Fail");
    return false;
}

template<typename ... Args>
bool PreparedStatement::Fetch(Args&... args)
{
    if(!_isPrepared) return false;

    size_t colCount = sizeof...(Args);
    _fetchLens.assign(colCount, 0);

    SQLSMALLINT index = 1;
    if(!(BindCol(index++, args) && ...))
    {
        return false;
    }

    SQLRETURN ret = SQLFetch(_hStmt);
    if(ret == SQL_NO_DATA)
    {
        return false;
    }

    if(ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        SQLSMALLINT postIndex = 1;
        (PostFetch(postIndex++, args), ...);
        return true;
    }

    DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "Fetch Fail");
    return false;
}

template <typename T>
bool PreparedStatement::BindParam(SQLSMALLINT index, T& value)
{
    SQLSMALLINT cType = DbTypeTraits<T>::cType;
    SQLSMALLINT sqlType = DbTypeTraits<T>::sqlType;
    _paramLens[index - 1] = 0;

    SQLRETURN ret = SQLBindParameter(
        _hStmt,
        index,
        SQL_PARAM_INPUT,
        cType,
        sqlType,
        0,
        0,
        &value,
        0,
        &_paramLens[index - 1]
    );

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "BindParameter Fail");
        return false;
    }

    return true;
}

inline bool PreparedStatement::BindParam(SQLSMALLINT index, std::string& value)
{
    SQLSMALLINT cType = SQL_C_CHAR;
    SQLSMALLINT sqlType = SQL_VARCHAR;

    _paramLens[index - 1] = SQL_NTS;

    SQLRETURN ret = SQLBindParameter(
        _hStmt,
        index,
        SQL_PARAM_INPUT,
        cType,
        sqlType,
        value.size(),
        0,
        (SQLPOINTER)value.c_str(),
        static_cast<SQLLEN>(value.size() + 1),
        &_paramLens[index - 1]
    );

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "BindParameter Fail");
        return false;
    }

    return true;
}

template<typename T>
bool PreparedStatement::BindCol(SQLSMALLINT index, T& value)
{
    SQLSMALLINT cType = DbTypeTraits<T>::cType;
    _fetchLens[index - 1] = 0;

    auto ret = SQLBindCol(
        _hStmt,
        index,
        cType,
        &value,
        0,
        &_fetchLens[index - 1]
    );

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "BindCol Fail");
        return false;
    }

    return true;
}

template<typename T>
bool PreparedStatement::BindCol(SQLSMALLINT index, optional<T>& value)
{
    if(!value.has_value())
    {
        value.emplace();
    }

    return BindCol(index, value.value());
}

inline bool PreparedStatement::BindCol(SQLSMALLINT index, std::string& value, size_t maxLen)
{
    SQLSMALLINT cType = SQL_C_CHAR;
    _fetchLens[index - 1] = 0;

    value.resize(maxLen);

    auto ret = SQLBindCol(
        _hStmt,
        index,
        cType,
        value.data(),
        (SQLLEN)maxLen,
        &_fetchLens[index - 1]
    );

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        DbConnection::PrintError(SQL_HANDLE_STMT, _hStmt, "BindCol Fail");
        return false;
    }

    return true;
}

template<typename T>
void PreparedStatement::PostFetch(SQLSMALLINT index, optional<T>& value)
{
    SQLLEN len  = _fetchLens[index - 1];
    if(len == SQL_NULL_DATA)
    {
        value.reset();
    }
    else
    {
        PostFetch(index, value.value());
    }
}

inline void PreparedStatement::PostFetch(SQLSMALLINT index, std::string& value)
{
    SQLLEN len  = _fetchLens[index - 1];
    if(len == SQL_NULL_DATA)
    {
        value.clear();
    }
    else if(len >= 0)
    {
        value.resize(len);
    }
}