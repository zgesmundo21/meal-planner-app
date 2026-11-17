#include "Favorites.h"

#include <fstream>
#include <iostream>

void Favorites::saveRecipe(
    int id,
    const std::string& name)
{
    std::ofstream file(
        "favorites.txt",
        std::ios::app);

    if (file.is_open())
    {
        file
            << id
            << ","
            << name
            << std::endl;
    }
}

void Favorites::displayFavorites()
{
    std::ifstream file(
        "favorites.txt");

    std::string line;

    std::cout
        << "\n================================="
        << "\nFAVORITES"
        << "\n=================================\n\n";

    while (getline(file, line))
    {
        std::cout
            << line
            << std::endl;
    }
}
