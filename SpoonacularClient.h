#pragma once

#include <vector>
#include <string>

#include "Meal.h"

class SpoonacularClient
{
public:
    std::vector<Meal> searchMeals(
        const std::string& query);

    void displayRecipeDetails(
        int recipeId);
};