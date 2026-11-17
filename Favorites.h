#pragma once

#include <string>

class Favorites
{
public:
    static void saveRecipe(
        int id,
        const std::string& name);

    static void displayFavorites();
};
