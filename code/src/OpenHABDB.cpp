#include "OpenHABDB.hpp"
#include <iostream>

OpenHABDB::OpenHABDB():m_db(nullptr)
{
    m_itemsTable = new ItemsTable(nullptr);
    m_thingTables = new std::vector<ThingTable*>();
}

OpenHABDB::OpenHABDB(std::string fileName)
{
    m_db = openDB(fileName);

    if (m_db != nullptr)
    {
        m_itemsTable = readItemsTable(m_db);

        if (m_itemsTable != nullptr)
        {
            m_thingTables = readThingTables(m_db, m_itemsTable);
        }
    }
}

OpenHABDB::~OpenHABDB()
{
    for (unsigned int i = 0; i < m_thingTables->size(); i++)
    {
        delete m_thingTables->at(i);
    }
    delete m_thingTables;

    delete m_itemsTable;
    sqlite3_close(m_db);
}

bool OpenHABDB::dataAvailable() const
{
    return m_db != nullptr;
}

bool OpenHABDB::saveToFile(std::string fileName)
{
    sqlite3 *db;
    bool result = true;
    int openResult = sqlite3_open(fileName.c_str(), &db);

    if (openResult == SQLITE_OK)
    {
        if (m_itemsTable->saveToDB(db))
        {
            for (unsigned int i = 0; i < m_thingTables->size(); i++)
            {
                std::cout << "saving thing " << (i + 1) << " of " << m_thingTables->size() << std::endl;
                result &= m_thingTables->at(i)->saveToDB(db);
            }
        } else
        {
            result = false;
        }
    } else
    {
        result = false;
        std::cout << "problem opening output database!" << std::endl;
    }

    sqlite3_close(db);

    return result;
}

OpenHABDB* OpenHABDB::merge(OpenHABDB *a, OpenHABDB *b)
{
    OpenHABDB *mergedDB = new OpenHABDB();

    for (unsigned int i = 0; i < a->m_itemsTable->getItemCount(); i++)
    {
        mergedDB->m_itemsTable->addAdditionalItem(a->m_itemsTable->getItemName(i));
    }

    for (unsigned int i = 0; i < b->m_itemsTable->getItemCount(); i++)
    {
        mergedDB->m_itemsTable->addAdditionalItem(b->m_itemsTable->getItemName(i));
    }

    for (unsigned int i = 0; i < mergedDB->m_itemsTable->getItemCount(); i++)
    {
        std::string thingName = mergedDB->m_itemsTable->getItemName(i);

        ThingTable *mergedThing = new ThingTable(i + 1, thingName, nullptr), *dummy;

        int aThingIndex = a->getThingTableIndex(thingName);
        int bThingIndex = b->getThingTableIndex(thingName);

        if (aThingIndex != -1)
        {
            dummy = ThingTable::merge(mergedThing, a->m_thingTables->at(aThingIndex));
            delete mergedThing;
            mergedThing = dummy;
        }

        if (bThingIndex != -1)
        {
            dummy = ThingTable::merge(mergedThing, b->m_thingTables->at(bThingIndex));
            delete mergedThing;
            mergedThing = dummy;
        }

        mergedDB->m_thingTables->push_back(mergedThing);
    }

    return mergedDB;
}

int OpenHABDB::getThingTableIndex(std::string name) const
{
    int result = -1;

    for (unsigned int i = 0; i < m_thingTables->size(); i++)
    {
        if (m_thingTables->at(i)->getName() == name)
        {
            result = static_cast<int>(i);
        }
    }

    return result;
}

sqlite3* OpenHABDB::openDB(std::string fileName)
{
    sqlite3 *db = nullptr;

    FILE *file = fopen(fileName.c_str(), "r");

    if (file != nullptr)
    {
        fclose(file);

        int result = sqlite3_open(fileName.c_str(), &db);

        if (result != SQLITE_OK)
        {
            std::cout << "can't open " << fileName << std::endl;
            sqlite3_close(db);
        }
    }

    return db;
}

ItemsTable* OpenHABDB::readItemsTable(sqlite3 *db)
{
    std::cout << "reading table items.. ";

    ItemsTable *itemsTable = new ItemsTable(db);

    if (itemsTable->read())
    {
        std::cout << "done";
    } else
    {
        delete itemsTable;
        itemsTable = nullptr;

        std::cout << "problem reading DB!";
    }

    std::cout << std::endl;

    return itemsTable;
}

std::vector<ThingTable*>* OpenHABDB::readThingTables(sqlite3 *DB, ItemsTable *itemsTable)
{
    std::vector<ThingTable*> *result = new std::vector<ThingTable*>();

    for (unsigned int i = 0; i < itemsTable->getItemCount(); i++)
    {
        ThingTable *thingTable = new ThingTable(i + 1, itemsTable->getItemName(i), DB);

        if (thingTable->read())
        {
            result->push_back(thingTable);
        } else
        {
            delete thingTable;
        }
    }

    return result;
}
