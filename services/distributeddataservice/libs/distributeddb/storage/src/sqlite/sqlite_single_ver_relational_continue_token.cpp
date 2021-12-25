/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef RELATIONAL_STORE
#include "sqlite_single_ver_relational_continue_token.h"
#include "sqlite_utils.h"

namespace DistributedDB {
SQLiteSingleVerRelationalContinueToken::SQLiteSingleVerRelationalContinueToken(
    const SyncTimeRange &timeRange, const QueryObject &object)
    : isGettingDeletedData_(false), queryObj_(object), tableName_(object.GetTableName()), timeRange_(timeRange)
{}

bool SQLiteSingleVerRelationalContinueToken::CheckValid() const
{
    bool isValid = (magicBegin_ == MAGIC_BEGIN && magicEnd_ == MAGIC_END);
    if (!isValid) {
        LOGE("Invalid continue token.");
    }
    return isValid;
}

int SQLiteSingleVerRelationalContinueToken::GetStatement(sqlite3 *db, sqlite3_stmt *&stmt, bool &isGettingDeletedData)
{
    isGettingDeletedData = isGettingDeletedData_;
    if (isGettingDeletedData) {
        return GetDeletedDataStmt(db, stmt);
    }
    return GetQuerySyncStatement(db, stmt);
}

void SQLiteSingleVerRelationalContinueToken::SetNextBeginTime(const DataItem &theLastItem)
{
    TimeStamp nextBeginTime = theLastItem.timeStamp + 1;
    if (nextBeginTime > INT64_MAX) {
        nextBeginTime = INT64_MAX;
    }
    if (!isGettingDeletedData_) {
        timeRange_.beginTime = nextBeginTime;
        return;
    }
    if ((theLastItem.flag & DataItem::DELETE_FLAG) != 0) {  // The last one could be non-deleted.
        timeRange_.deleteBeginTime = nextBeginTime;
    }
}

void SQLiteSingleVerRelationalContinueToken::FinishGetData()
{
    if (isGettingDeletedData_) {
        timeRange_.deleteEndTime = 0;
        return;
    }
    isGettingDeletedData_ = true;
    timeRange_.endTime = 0;
    return;
}

bool SQLiteSingleVerRelationalContinueToken::IsGetAllDataFinished() const
{
    return timeRange_.beginTime >= timeRange_.endTime && timeRange_.deleteBeginTime >= timeRange_.deleteEndTime;
}

int SQLiteSingleVerRelationalContinueToken::GetQuerySyncStatement(sqlite3 *db, sqlite3_stmt *&stmt)
{
    int errCode = E_OK;
    SqliteQueryHelper helper = queryObj_.GetQueryHelper(errCode);
    if (errCode != E_OK) {
        return errCode;
    }
    return helper.GetRelationalQueryStatement(db, timeRange_.beginTime, timeRange_.endTime, stmt);
}

int SQLiteSingleVerRelationalContinueToken::GetDeletedDataStmt(sqlite3 *db, sqlite3_stmt *&stmt) const
{
    // get stmt
    int errCode = E_OK;
    const std::string sql = GetDeletedDataSQL();
    errCode = SQLiteUtils::GetStatement(db, sql, stmt);
    if (errCode != E_OK) {
        goto ERROR;
    }

    // bind stmt
    errCode = SQLiteUtils::BindInt64ToStatement(stmt, 1, timeRange_.deleteBeginTime); // 1 means begin time
    if (errCode != E_OK) {
        goto ERROR;
    }
    errCode = SQLiteUtils::BindInt64ToStatement(stmt, 2, timeRange_.deleteEndTime); // 2 means end time
    if (errCode != E_OK) {
        goto ERROR;
    }
    return errCode;

ERROR:
    SQLiteUtils::ResetStatement(stmt, true, errCode);
    return errCode;
}

const QueryObject &SQLiteSingleVerRelationalContinueToken::GetQuery() const
{
    return queryObj_;
}

std::string SQLiteSingleVerRelationalContinueToken::GetDeletedDataSQL() const
{
    std::string tableName = DBConstant::RELATIONAL_PREFIX + tableName_ + "_log";
    return "SELECT * FROM " + tableName +
        " WHERE timestamp >= ? AND timestamp < ? AND (flag&0x03 = 0x03) ORDER BY timestamp ASC;";
}
}  // namespace DistributedDB
#endif