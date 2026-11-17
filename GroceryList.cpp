#include "GroceryList.h"

#include <fstream>
#include <iostream>
#include <set>

void GroceryList::saveList(
    const std::vector<std::string>& ingredients)
{
    std::set<std::string> uniqueIngredients;

    std::ifstream input(
        "grocerylist.txt");

    std::string line;

    while (getline(input, line))
    {
        uniqueIngredients.insert(line);
    }

    input.close();

    for (const auto& ingredient : ingredients)
    {
        uniqueIngredients.insert(ingredient);
    }

    std::ofstream output(
        "grocerylist.txt");

    for (const auto& ingredient : uniqueIngredients)
    {
        output
            << ingredient
            << std::endl;
    }

    output.close();
}

void GroceryList::viewList()
{
    std::ifstream file(
        "grocerylist.txt");

    std::string line;

    std::cout
        << "\n================================="
        << "\nGROCERY LIST"
        << "\n=================================\n\n";

    while (getline(file, line))
    {
        std::cout
            << "- "
            << line
            << std::endl;
    }

    file.close();
}

void GroceryList::removeItem(
    const std::string& itemToRemove)
{
    std::ifstream input(
        "grocerylist.txt");

    std::vector<std::string> ingredients;

    std::string line;

    while (getline(input, line))
    {
        if (line != itemToRemove)
        {
            ingredients.push_back(line);
        }
    }

    input.close();

    std::ofstream output(
        "grocerylist.txt");

    for (const auto& ingredient : ingredients)
    {
        output
            << ingredient
            << std::endl;
    }

    output.close();
}
