#include "database.h"
#include "device_manager.h"

#include <sglite3.h>
#include <cstring>

namespace {

    bool exeSgl(sglite3* db, const char* sgl){
        char* err = nullptr;
        if (sqlite3_exec(db, sql, nullptr, nullptr, &err) !=SQLITE_OK){
            if(err) {
                sqlite3_free(err);
            }
            return false;
        }
        return true;
    }
}

Database::Database(std:string path) : path_(path) {}

Database::~Database(){
    close()
}

void Database::close(){
    if (db_){
        sglite3_close(db_);
        db_ = nullptr;
    }
}
bool Database::open(){
    close();

    if(sqlite3_open(path_.c_str, &db_) !=SQLITE_OK){
        db_ = nullptr;
        return false;
    }

    if(!exeSgl(db_, "PRAGMA journal_mode = WAl;")){
        return false;
    }

    if(!exeSgl(db_, KCreateReadingsTableSgl)){
        return false;
    }

    if(!exeSgl(db_, KCreateReadingsIndexSgl)){
        return false;
    }
    return true
}
bool Database::insertReading(const DeviceStatus& status, const std::string& timestamp){
    sqlite3* db static_cast<sqlite3*>(dbHandle_)

    if(!db){
        return false;
    }

    const char* sql ="???"

    sqlite3_stmt* stmt = nullptr;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepary_v2(db, sql, static_cast<int>(std::strlen(sql)), &stmt, nullptr) != SQLITE_OK){
        return false;
    }
}
    sqlite3_bind_text(stmt, 1, timestamp)