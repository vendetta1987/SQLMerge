#include "ThingTable.hpp"
#include <iostream>
#include <sstream>
#include <ctime>

const std::string ThingTable::TABLE_BASE_NAME = "item";
bool ThingTable::m_outputEnabled = false;

ThingTable::ThingTable(unsigned int ID, std::string name, sqlite3 *db):
    m_DB(db), m_ID(ID), m_name(name), m_valueType(ValueType::UNDEFINED), m_valueTypeSize(-1)
{
    m_entries = new std::map<DBTime*, std::string*>();
    getDataType();
}

ThingTable::~ThingTable()
{
    for (std::map<DBTime*, std::string*>::iterator i = m_entries->begin(); i != m_entries->end(); ++i)
    {
        delete i->first;
        delete i->second;
    }
    delete m_entries;
}

ThingTable* ThingTable::merge(ThingTable *a, ThingTable *b)
{
    ThingTable *merged = new ThingTable(a->m_ID, a->getName(), nullptr);
    for (std::map<DBTime*, std::string*>::iterator i = a->m_entries->begin(); i != a->m_entries->end(); ++i)
    {
        merged->m_entries->insert(std::make_pair(new DBTime(*i->first), new std::string(*i->second)));
    }

    for (std::map<DBTime*, std::string*>::iterator i = b->m_entries->begin(); i != b->m_entries->end(); ++i)
    {
        merged->m_entries->insert(std::make_pair(new DBTime(*i->first), new std::string(*i->second)));
    }

    return merged;
}

std::string ThingTable::getName() const
{
    return m_name;
}

std::string ThingTable::getSQLValueType()const
{
    std::string type;

    switch (m_valueType)
    {
        case DOUBLE:
            type = VALUE_TYPE_NAMES[m_valueType];
            break;
        case VARCHAR:
            type = VALUE_TYPE_NAMES[m_valueType];
            type += "(" + std::to_string(m_valueTypeSize) + ")";
            break;
        default:
            type = VALUE_TYPE_NAMES[ValueType::VARCHAR] + "(100)";
            break;
    }

    return type;
}

bool ThingTable::read()
{
    bool returnValue = false;

    if (m_DB != nullptr)
    {
        char *error;
        char **results;
        int rowCount, colCount;

        std::string tableName = convertToTableName(m_ID);
        std::string sqlQuery = "select * from " + tableName + ";";

        if (sqlite3_get_table(m_DB, sqlQuery.c_str(), &results, &rowCount, &colCount, &error) == SQLITE_OK)
        {
            if (m_outputEnabled)
            {
                std::cout << "reading data of " << m_name << " (" << tableName << ")" << std::endl;
            }
            //skip first row with table header
            for (int row = 1; row <= rowCount; row++)
            {
                unsigned int rowIndex = row*colCount;
                DBTime *date = parseDate(results[rowIndex]);
                std::string *value = new std::string(results[rowIndex + 1]);

                m_entries->insert(std::make_pair(date, value));

                if (m_outputEnabled)
                {
                    std::cout << date << " " << *value << std::endl;
                }
            }

            sqlite3_free_table(results);
            returnValue = true;
        }

        sqlite3_free(error);
    }

    return returnValue;
}

bool ThingTable::saveToDB(sqlite3* db)
{
    bool result = true;
    int createResult;
    char *error = nullptr;

    std::string createStatement = getSQLCreateStatement();
    createResult = sqlite3_exec(db, createStatement.c_str(), nullptr, nullptr, &error);

    std::string tableName = convertToTableName(m_ID);

    if (createResult == SQLITE_OK)
    {
        sqlite3_exec(db, "BEGIN", nullptr, nullptr, &error);

        for (std::map<DBTime*, std::string*>::iterator i = m_entries->begin(); i != m_entries->end(); ++i)
        {
            std::string insertStatement =
                "insert into " + tableName +
                "(time, value) values('" + serialiseDate(i->first) + "','" + *i->second + "')";
            createResult = sqlite3_exec(db, insertStatement.c_str(), nullptr, nullptr, &error);

            if (createResult != SQLITE_OK)
            {
                result = false;
                std::cout << "problem inserting into item table:\n\t" << error << std::endl;
            }
        }

        sqlite3_exec(db, "COMMIT", nullptr, nullptr, &error);
    } else
    {
        result = false;
        std::cout << "problem creating item table:\n\t" << error << std::endl;
    }

    sqlite3_free(error);

    return result;
}

std::string ThingTable::convertToTableName(unsigned int ID)
{
    std::stringstream tableNr;

    tableNr.width(4);
    tableNr.fill('0');

    tableNr << ID;

    return TABLE_BASE_NAME + tableNr.str();
}

DBTime* ThingTable::parseDate(std::string dateString)
{
    tm parsedTime;

    parsedTime.tm_year = std::stoi(dateString.substr(0, 4)) - 1900;
    parsedTime.tm_mon = std::stoi(dateString.substr(5, 2)) - 1;
    parsedTime.tm_mday = std::stoi(dateString.substr(8, 2));
    parsedTime.tm_hour = std::stoi(dateString.substr(11, 2));
    parsedTime.tm_min = std::stoi(dateString.substr(14, 2));
    parsedTime.tm_sec = std::stoi(dateString.substr(17, 2));

    parsedTime.tm_isdst = 0;
    parsedTime.tm_wday = 0;
    parsedTime.tm_yday = 0;

    time_t time = mktime(&parsedTime);

    DBTime* result = new DBTime();

    result->unixTime = static_cast<unsigned long long>(time);

    result->ms = static_cast<unsigned int>(std::stoi(dateString.substr(20, 3)));

    return result;
}

std::string ThingTable::serialiseDate(DBTime *date)
{
    time_t timeT = static_cast<time_t>(date->unixTime);
    tm *timeTM = localtime(&timeT);

    char *dummy = new char[25];
    strftime(dummy, 25, "%Y-%m-%d %R:%S", timeTM);

    std::string result(dummy);

    std::stringstream msStr;

    msStr.width(3);
    msStr.fill('0');

    msStr << date->ms;

    result += "." + msStr.str();

    delete[] dummy;
    return result;
}

void ThingTable::getDataType()
{
    if (m_DB != nullptr)
    {
        const char *type, *seq;
        int notNull, primKey, autoInc;
        std::string tableName = convertToTableName(m_ID);

        sqlite3_table_column_metadata(
            m_DB, nullptr, tableName.c_str(), "value", &type, &seq, &notNull, &primKey, &autoInc);

        std::string strType(type);

        if (strType.find(VALUE_TYPE_NAMES[ValueType::DOUBLE], 0) != std::string::npos)
        {
            m_valueType = ValueType::DOUBLE;
        } else if (strType.find(VALUE_TYPE_NAMES[ValueType::VARCHAR], 0) != std::string::npos)
        {
            m_valueType = ValueType::VARCHAR;

            unsigned int openPos = strType.find("(", 0) + 1;
            unsigned int closePos = strType.find(")", 0);

            strType = strType.substr(openPos, closePos - openPos);

            m_valueTypeSize = std::stoi(strType);
        } else
        {
            std::cout << "unknown data type in value column!" << std::endl;
        }
    }
}

std::string ThingTable::getSQLCreateStatement()
{
    return "CREATE TABLE " + convertToTableName(m_ID) + "(time TIMESTAMP NOT NULL, value " + getSQLValueType() + ", PRIMARY KEY(time))";
}
