#include "main.hpp"
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "please provide at least 2 databases!" << std::endl;
        exit(-1);
    }

    std::vector<OpenHABDB*> openHABDBs;

    for (int i = 1; i < argc; i++)
    {
        OpenHABDB *db = new OpenHABDB(argv[i]);

        if (db->dataAvailable())
        {
            openHABDBs.push_back(db);
        } else
        {
            delete db;
        }
    }

    if (openHABDBs.size() > 1)
    {
        OpenHABDB *merged = OpenHABDB::merge(openHABDBs[0], openHABDBs[1]);

        delete openHABDBs[0];
        openHABDBs[0] = nullptr;

        delete openHABDBs[1];
        openHABDBs[1] = nullptr;

        if (openHABDBs.size() > 2)
        {
            for (unsigned int i = 2; i < openHABDBs.size(); i++)
            {
                OpenHABDB *newMerge = OpenHABDB::merge(merged, openHABDBs[i]);

                delete openHABDBs[i];
                openHABDBs[i] = nullptr;

                delete merged;
                merged = newMerge;
            }
        }

        merged->saveToFile();
        delete merged;
    }

    for (unsigned int i = 0; i < openHABDBs.size(); i++)
    {
        delete openHABDBs[i];
    }
}
