#pragma once

#include <string>
#include <vector>

class AIClient
{
public:
    std::string generateMealPlan(
        const std::string& goal,
        int calories,
        int days,
        const std::string& restrictions = "",
        int cookingTime = 0,
        const std::string& requirements = "");

    std::string generateMealPlanFromRecipes(
        const std::string& goal,
        int calories,
        const std::vector<std::string>& recipes);

    std::string askQuestion(
        const std::string& question);

    std::string adaptRecipe(
        const std::string& recipeDetails,
        const std::string& restrictions);

};
