#pragma once

#include <vector>
#include <string>

class GroceryList
{
public:
    static void saveList(
        const std::vector<std::string>& ingredients);

    static void viewList();

    static void removeItem(
        const std::string& itemToRemove);
};
