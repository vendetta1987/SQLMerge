#pragma once
#include "sqlite/sqlite3.h"
#include <map>
#include <string>

enum ValueType
{
    DOUBLE,
    VARCHAR,

    UNDEFINED
};

static const std::string VALUE_TYPE_NAMES[] =
{
    "DOUBLE",
    "VARCHAR"
};

struct DBTime
{
    DBTime()
    {
        unixTime = 0;
        ms = 0;
    }

    DBTime(const DBTime &org)
    {
        this->unixTime = org.unixTime;
        this->ms = org.ms;
    }

    unsigned long long unixTime;
    unsigned int ms;
};

class ThingTable
{
public:
    ThingTable(unsigned int ID, std::string name, sqlite3 *db);
    virtual ~ThingTable();

    static ThingTable* merge(ThingTable *a, ThingTable *b);

    std::string getName() const;
    std::string getSQLValueType() const;

    bool read();
    bool saveToDB(sqlite3* db);

private:
    static std::string convertToTableName(unsigned int ID);
    static DBTime* parseDate(std::string dateString);
    static std::string serialiseDate(DBTime *date);

    void getDataType();
    std::string getSQLCreateStatement();

    static const std::string TABLE_BASE_NAME;
    static bool m_outputEnabled;

    sqlite3 *m_DB;
    std::map<DBTime*, std::string*> *m_entries;

    unsigned int m_ID;
    std::string m_name;
    ValueType m_valueType;
    unsigned int m_valueTypeSize;
};
