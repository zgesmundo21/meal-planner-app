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
        "eaac0fe3cdfa4a6eaffb356434af39aa";

    auto response =
        cpr::Get(
            cpr::Url{
                "https://api.spoonacular.com/recipes/complexSearch"
            },
            cpr::Parameters{
                { "query", query },
                { "number", "10" },
                { "apiKey", apiKey }
            });

    if (response.status_code != 200)
    {
        std::cout
            << "Failed to connect to Spoonacular."
            << std::endl;

        return meals;
    }

    json data;

    try
    {
        data =
            json::parse(response.text);
    }
    catch (...)
    {
        std::cout
            << "Failed to parse API response."
            << std::endl;

        return meals;
    }

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
        "eaac0fe3cdfa4a6eaffb356434af39aa";

    std::string url =
        "https://api.spoonacular.com/recipes/"
        + std::to_string(recipeId)
        + "/information";

    auto response =
        cpr::Get(
            cpr::Url{ url },
            cpr::Parameters{
                { "includeNutrition", "true" },
                { "apiKey", apiKey }
            });

    if (response.status_code != 200)
    {
        std::cout
            << "Failed to retrieve recipe details."
            << std::endl;

        return;
    }

    json data;

    try
    {
        data =
            json::parse(response.text);
    }
    catch (...)
    {
        std::cout
            << "Failed to parse recipe data."
            << std::endl;

        return;
    }

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

    if (data.contains("readyInMinutes"))
    {
        std::cout
            << "Ready In: "
            << data["readyInMinutes"]
            << " minutes"
            << std::endl;
    }

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
                << ":"
                << std::endl;

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

std::vector<Meal> SpoonacularClient::getHealthyRecipes(
    const std::string& goal)
{
    std::vector<Meal> meals;

    std::string apiKey =
        "eaac0fe3cdfa4a6eaffb356434af39aa";

    auto response =
        cpr::Get(
            cpr::Url{
                "https://api.spoonacular.com/recipes/complexSearch"
            },
            cpr::Parameters{
                { "query", goal + " meal"},
                { "number", "10" },
                { "apiKey", apiKey }
            });

    std::cout
        << "\nSTATUS CODE: "
        << response.status_code
        << std::endl;

    std::cout
        << "\nRESPONSE:\n"
        << response.text
        << std::endl;

    if (response.status_code != 200)
    {
        return meals;
    }

    json data =
        json::parse(response.text);

    if (data.contains("results"))
    {
        for (auto& recipe : data["results"])
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


std::vector<std::string> SpoonacularClient::getIngredients(
    int recipeId)
{
    std::vector<std::string> ingredients;

    std::string apiKey =
        "eaac0fe3cdfa4a6eaffb356434af39aa";

    std::string url =
        "https://api.spoonacular.com/recipes/"
        + std::to_string(recipeId)
        + "/information";

    auto response =
        cpr::Get(
            cpr::Url{ url },
            cpr::Parameters{
                { "apiKey", apiKey }
            });

    if (response.status_code != 200)
    {
        return ingredients;
    }

    json data =
        json::parse(response.text);

    if (data.contains("extendedIngredients"))
    {
        for (auto& ingredient :
            data["extendedIngredients"])
        {
            ingredients.push_back(
                ingredient["name"]);
        }
    }

    return ingredients;
}
std::vector<std::string> SpoonacularClient::getEquipment(
    int recipeId)
{
    std::vector<std::string> equipmentList;

    std::string apiKey =
        "eaac0fe3cdfa4a6eaffb356434af39aa";

    std::string url =
        "https://api.spoonacular.com/recipes/"
        + std::to_string(recipeId)
        + "/information";

    auto response =
        cpr::Get(
            cpr::Url{ url },
            cpr::Parameters{
                { "apiKey", apiKey }
            });

    if (response.status_code != 200)
    {
        return equipmentList;
    }

    json data =
        json::parse(response.text);

    if (data.contains("analyzedInstructions") &&
        !data["analyzedInstructions"].empty())
    {
        auto steps =
            data["analyzedInstructions"][0]["steps"];

        for (auto& step : steps)
        {
            if (step.contains("equipment"))
            {
                for (auto& equipment :
                    step["equipment"])
                {
                    equipmentList.push_back(
                        equipment["name"]);
                }
            }
        }
    }

    return equipmentList;
}
