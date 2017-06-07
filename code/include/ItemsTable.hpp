#pragma once
#include <map>
#include <set>
#include <string>

#include "sqlite/sqlite3.h"

class ItemsTable
{
public:
    ItemsTable(sqlite3 *db);
    virtual ~ItemsTable();

    unsigned int getItemCount() const;
    std::string getItemName(unsigned int ID) const;

    bool read();
    bool addAdditionalItem(std::string name, int ID = -1);
    bool saveToDB(sqlite3* db);

private:
    static bool m_outputEnabled;

    static std::string getSQLCreateStatement();

    std::set<std::string> *m_itemNames;
    std::map<unsigned int, std::string*> *m_items;
    sqlite3 *m_DB;
};

