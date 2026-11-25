#pragma once

#include <vector>
#include <string>

#include "Meal.h"
#include <nlohmann/json.hpp>

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
      
    static nlohmann::json searchRecipesJson(
        const std::string& query,
        int number = 12,
        int offset = 0);

    static nlohmann::json getRecipeDetailsJson(
        int recipeId);
};