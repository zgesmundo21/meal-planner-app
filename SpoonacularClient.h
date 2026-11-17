#pragma once

#include <vector>
#include <string>

#include "Meal.h"

class SpoonacularClient
{
public:
    std::vector<Meal> searchMeals(
        const std::string& query);

    std::vector<Meal> getHealthyRecipes(
        const std::string& goal);

    std::vector<std::string> getIngredients(
        int recipeId);

    std::vector<std::string> getEquipment(
        int recipeId);

    void displayRecipeDetails(
        int recipeId);
};
