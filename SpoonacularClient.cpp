#include "SpoonacularClient.h"

#include <iostream>
#include <regex>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<Meal> SpoonacularClient::searchMeals(
    const std::string& query)
{
    std::vector<Meal> meals;

    std::string apiKey =
        "YOUR_SPOONACULAR_API_KEY";

    auto response =
        cpr::Get(
            cpr::Url{
                "https://api.spoonacular.com/recipes/complexSearch"
            },
            cpr::Parameters{
                {"query", query},
                {"number", "10"},
                {"apiKey", apiKey}
            });

    json data =
        json::parse(response.text);

    if (data.contains("results"))
    {
        for (auto& recipe :
            data["results"])
        {
            meals.push_back(
                Meal(
                    recipe["id"],
                    recipe["title"]
                ));
        }
    }

    return meals;
}

void SpoonacularClient::displayRecipeDetails(
    int recipeId)
{
    std::string apiKey =
        "YOUR_SPOONACULAR_API_KEY";

    std::string url =
        "https://api.spoonacular.com/recipes/"
        + std::to_string(recipeId)
        + "/information";

    auto response =
        cpr::Get(
            cpr::Url{ url },
            cpr::Parameters{
                {"includeNutrition", "true"},
                {"apiKey", apiKey}
            });

    json data =
        json::parse(response.text);

    std::cout
        << "\n================================="
        << "\nRECIPE DETAILS"
        << "\n=================================\n\n";

    std::cout
        << "Name: "
        << data["title"]
        << std::endl;

    std::cout
        << "Servings: "
        << data["servings"]
        << std::endl;

    double calories = 0;
    double protein = 0;
    double carbs = 0;
    double fat = 0;

    if (data.contains("nutrition"))
    {
        auto nutrients =
            data["nutrition"]["nutrients"];

        for (auto& nutrient :
            nutrients)
        {
            std::string name =
                nutrient["name"];

            if (name == "Calories")
            {
                calories =
                    nutrient["amount"];
            }
            else if (name == "Protein")
            {
                protein =
                    nutrient["amount"];
            }
            else if (name == "Carbohydrates")
            {
                carbs =
                    nutrient["amount"];
            }
            else if (name == "Fat")
            {
                fat =
                    nutrient["amount"];
            }
        }

        std::cout
            << "\n================================="
            << "\nNUTRITION PER SERVING"
            << "\n=================================\n\n";

        std::cout
            << "Calories: "
            << calories
            << " kcal"
            << std::endl;

        std::cout
            << "Protein: "
            << protein
            << " g"
            << std::endl;

        std::cout
            << "Carbohydrates: "
            << carbs
            << " g"
            << std::endl;

        std::cout
            << "Fat: "
            << fat
            << " g"
            << std::endl;

        int servings =
            data["servings"];

        std::cout
            << "\n================================="
            << "\nTOTAL RECIPE NUTRITION"
            << "\n=================================\n\n";

        std::cout
            << "Calories: "
            << calories * servings
            << " kcal"
            << std::endl;

        std::cout
            << "Protein: "
            << protein * servings
            << " g"
            << std::endl;

        std::cout
            << "Carbohydrates: "
            << carbs * servings
            << " g"
            << std::endl;

        std::cout
            << "Fat: "
            << fat * servings
            << " g"
            << std::endl;
    }

    std::cout
        << "\n================================="
        << "\nINGREDIENTS"
        << "\n=================================\n\n";

    if (data.contains("extendedIngredients"))
    {
        for (auto& ingredient :
            data["extendedIngredients"])
        {
            std::cout
                << "- "
                << ingredient["original"]
                << std::endl;
        }
    }

    std::cout
        << "\n================================="
        << "\nINSTRUCTIONS"
        << "\n=================================\n\n";

    if (data.contains("analyzedInstructions") &&
        !data["analyzedInstructions"].empty())
    {
        auto steps =
            data["analyzedInstructions"][0]["steps"];

        for (auto& step :
            steps)
        {
            std::cout
                << "Step "
                << step["number"]
                << ":" << std::endl;

            std::cout
                << step["step"]
                << std::endl
                << std::endl;
        }
    }
    else if (data.contains("instructions") &&
        !data["instructions"].is_null())
    {
        std::string instructions =
            data["instructions"];

        instructions =
            std::regex_replace(
                instructions,
                std::regex("<[^>]*>"),
                "");

        std::cout
            << instructions
            << std::endl;
    }
    else if (data.contains("summary"))
    {
        std::string summary =
            data["summary"];

        summary =
            std::regex_replace(
                summary,
                std::regex("<[^>]*>"),
                "");

        std::cout
            << summary
            << std::endl;
    }
    else
    {
        std::cout
            << "No instructions available."
            << std::endl;
    }
}