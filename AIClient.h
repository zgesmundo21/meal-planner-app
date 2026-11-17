#pragma once

#include <string>
#include <vector>

class AIClient
{
public:
    std::string generateMealPlan(
        const std::string& goal,
        int calories,
        int days);

    std::string generateMealPlanFromRecipes(
        const std::string& goal,
        int calories,
        const std::vector<std::string>& recipes);

    std::string askQuestion(
        const std::string& question);

};
