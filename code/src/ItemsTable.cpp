#include "ItemsTable.hpp"
#include <iostream>

bool ItemsTable::m_outputEnabled = false;

ItemsTable::ItemsTable(sqlite3 *db): m_DB(db)
{
    m_items = new std::map<unsigned int, std::string*>();
    m_itemNames = new std::set<std::string>();
}

ItemsTable::~ItemsTable()
{
    delete m_itemNames;

    for (std::map<unsigned int, std::string*>::iterator i = m_items->begin(); i != m_items->end(); ++i)
    {
        delete i->second;
    }
    delete m_items;
}

std::string ItemsTable::getSQLCreateStatement()
{
    return "CREATE TABLE items (ItemId INTEGER PRIMARY KEY AUTOINCREMENT, itemname VARCHAR(500) NOT NULL)";
}

unsigned int ItemsTable::getItemCount() const
{
    return m_items->size();
}

std::string ItemsTable::getItemName(unsigned int ID) const
{
    if (m_items->count(ID) != 0)
    {
        return *m_items->find(ID)->second;
    }

    return "";
}

bool ItemsTable::read()
{
    bool returnValue = false;

    if (m_DB != nullptr)
    {
        const char *sqlSelectAll = "select * from items;";
        char *error;
        char **results;
        int rowCount, colCount;

        if (sqlite3_get_table(m_DB, sqlSelectAll, &results, &rowCount, &colCount, &error) == SQLITE_OK)
        {
            if (m_outputEnabled)
            {
                std::cout << "content of table items:" << std::endl;
            }
            //skip first row with table header
            for (int row = 1; row <= rowCount; row++)
            {
                unsigned int rowIndex = row*colCount;
                int itemID = std::stoi(results[rowIndex]) - 1;
                std::string tableEntry = results[rowIndex + 1];

                addAdditionalItem(tableEntry, itemID);

                if (m_outputEnabled)
                {
                    std::cout << itemID << " " << tableEntry << std::endl;
                }
            }

            sqlite3_free_table(results);
            returnValue = true;
        }

        sqlite3_free(error);
    }

    return returnValue;
}

bool ItemsTable::addAdditionalItem(std::string name, int ID)
{
    unsigned int itemID;

    if (ID < 0)
    {
        itemID = m_items->size();
    } else
    {
        itemID = static_cast<unsigned int>(ID);
    }

    if (m_itemNames->count(name) == 0)
    {
        m_itemNames->insert(name);
        return m_items->insert(std::make_pair(itemID, new std::string(name))).second;
    } else
    {
        return false;
    }
}

bool ItemsTable::saveToDB(sqlite3* db)
{
    bool result = true;
    int createResult;
    char *error = nullptr;

    std::string createStatement = ItemsTable::getSQLCreateStatement();
    createResult = sqlite3_exec(db, createStatement.c_str(), nullptr, nullptr, &error);

    if (createResult == SQLITE_OK)
    {
        sqlite3_exec(db, "BEGIN", nullptr, nullptr, &error);

        for (std::map<unsigned int, std::string*>::iterator i = m_items->begin(); i != m_items->end(); ++i)
        {
            std::string insertStatement = "insert into items(itemname) values('" + *i->second + "')";
            createResult = sqlite3_exec(db, insertStatement.c_str(), nullptr, nullptr, &error);

            if (createResult != SQLITE_OK)
            {
                result = false;
                std::cout << "problem inserting into items table:\n\t" << error << std::endl;
            }
        }

        sqlite3_exec(db, "COMMIT", nullptr, nullptr, &error);
    } else
    {
        result = false;
        std::cout << "problem creating items table:\n\t" << error << std::endl;
    }

    sqlite3_free(error);

    return result;
}
