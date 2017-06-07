#pragma once
#include <string>
#include <vector>

#include "sqlite/sqlite3.h"
#include "ItemsTable.hpp"
#include "ThingTable.hpp"

class OpenHABDB
{
public:
    OpenHABDB(std::string fileName);
    virtual ~OpenHABDB();

    static OpenHABDB* merge(OpenHABDB *a, OpenHABDB *b);

    bool dataAvailable() const;
    bool saveToFile(std::string fileName = "merged.sqlite.db");

private:
    OpenHABDB();
    int getThingTableIndex(std::string name) const;

    sqlite3* openDB(std::string fileName);
    ItemsTable* readItemsTable(sqlite3 *db);
    std::vector<ThingTable*>* readThingTables(sqlite3 *DB, ItemsTable *itemsTable);

    sqlite3 *m_db;
    ItemsTable *m_itemsTable;
    std::vector<ThingTable*> *m_thingTables;
};
