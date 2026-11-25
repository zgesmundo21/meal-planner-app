#include "SpoonacularClient.h"

#include <iostream>
#include <regex>
#include <stdexcept>
#include <cstdlib>
#include <string>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace
{
    std::string readSpoonacularApiKey()
    {
        const char* value =
            std::getenv("SPOONACULAR_API_KEY");

        if (value == nullptr || value[0] == '\0')
        {
            return {};
        }

        return std::string(value);
    }
}

std::vector<Meal> SpoonacularClient::searchMeals(
    const std::string& query)
{
    std::vector<Meal> meals;

    std::string apiKey = readSpoonacularApiKey();

    if (apiKey.empty())
    {
        std::cerr << "Spoonacular API key not found."
            << std::endl;
        return meals;
    }

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

    // Check for network or connection errors
    if (response.error)
    {
        std::cerr
            << "Spoonacular network error: "
            << response.error.message
            << std::endl;

        return meals;
    }

    // Check the HTTP response status
    if (response.status_code != 200)
    {
        std::cerr
            << "\nSpoonacular API Error"
            << "\nHTTP Status Code: "
            << response.status_code
            << "\nResponse: "
            << response.text
            << std::endl;

        if (response.status_code == 401)
        {
            std::cerr
                << "Authentication failed. Check your API key."
                << std::endl;
        }
        else if (response.status_code == 402)
        {
            std::cerr
                << "Spoonacular quota or payment limit reached."
                << std::endl;
        }
        else if (response.status_code == 429)
        {
            std::cerr
                << "Too many requests. Please try again later."
                << std::endl;
        }
        else if (response.status_code >= 500)
        {
            std::cerr
                << "Spoonacular server error. Please try again later."
                << std::endl;
        }

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
    std::string apiKey = readSpoonacularApiKey();

    if (apiKey.empty())
    {
        std::cerr << "Spoonacular API key not found."
            << std::endl;
        return;
    }

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
        const auto& nutrients =
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
        const auto& steps =
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

    std::string apiKey = readSpoonacularApiKey();

    if (apiKey.empty())
    {
        std::cerr << "Spoonacular API key not found."
            << std::endl;
        return meals;
    }

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

    std::string apiKey = readSpoonacularApiKey();

    if (apiKey.empty())
    {
        std::cerr << "Spoonacular API key not found."
            << std::endl;
        return ingredients;
    }

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

    std::string apiKey = readSpoonacularApiKey();

    if (apiKey.empty())
    {
        std::cerr << "Spoonacular API key not found."
            << std::endl;
        return equipmentList;
    }

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
        const auto& steps =
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

// ==========================================
// WEBSITE RECIPE SEARCH
// ==========================================

nlohmann::json SpoonacularClient::searchRecipesJson(
    const std::string& query,
    int number,
    int offset)
{
    std::string apiKey = readSpoonacularApiKey();

    if (apiKey.empty())
    {
        throw std::runtime_error(
            "SPOONACULAR_API_KEY is not configured."
        );
    }

    auto response = cpr::Get(
        cpr::Url{
            "https://api.spoonacular.com/recipes/complexSearch"
        },
        cpr::Parameters{
            { "query", query },
            { "number", std::to_string(number) },
            { "offset", std::to_string(offset) }
        },
        cpr::Header{
            { "x-api-key", apiKey }
        },
        cpr::Timeout{ 15000 }
    );

    if (response.error)
    {
        throw std::runtime_error(
            "Could not connect to Spoonacular: " +
            response.error.message
        );
    }

    // Handle API errors.
    if (response.status_code != 200)
    {
        throw std::runtime_error(
            "Spoonacular returned HTTP " +
            std::to_string(response.status_code)
        );
    }

    // Parse the response.
    json data = json::parse(response.text);

    if (!data.contains("results") ||
        !data["results"].is_array())
    {
        throw std::runtime_error(
            "Unexpected Spoonacular response format."
        );
    }

    // Build the JSON structure expected by the frontend.
    json recipes = json::array();

    for (const auto& recipe : data["results"])
    {
        if (!recipe.is_object() ||
            !recipe.contains("id") ||
            !recipe["id"].is_number_integer())
        {
            continue;
        }

        recipes.push_back({
            { "id", recipe["id"] },
            { "title", recipe.value("title", "") },
            { "image", recipe.value("image", "") }
            });
    }

    int totalResults = data.at("totalResults").get<int>();

    int returnedCount =
        static_cast<int>(data["results"].size());

    int nextOffset = offset + returnedCount;

    bool hasMore =
        returnedCount > 0 &&
        nextOffset < totalResults &&
        nextOffset <= 900;

    return {
        { "recipes", recipes },
        { "totalResults", totalResults },
        { "number", number },
        { "offset", offset },
        { "nextOffset", nextOffset },
        { "hasMore", hasMore }
    };
}

nlohmann::json SpoonacularClient::getRecipeDetailsJson(int recipeId)
{
    if (recipeId <= 0)
    {
        throw std::invalid_argument("Invalid recipe ID.");
    }

    std::string apiKey = readSpoonacularApiKey();

    if (apiKey.empty())
    {
        throw std::runtime_error(
            "SPOONACULAR_API_KEY is missing."
        );
    }

    auto response = cpr::Get(
        cpr::Url{
            "https://api.spoonacular.com/recipes/" +
            std::to_string(recipeId) +
            "/information"
        },
        cpr::Parameters{
            { "includeNutrition", "true" }
        },
        cpr::Header{
            { "x-api-key", apiKey }
        },
        cpr::Timeout{ 15000 }
    );

    if (response.error)
    {
        throw std::runtime_error(
            "Spoonacular connection error: " +
            response.error.message
        );
    }

    if (response.status_code != 200)
    {
        throw std::runtime_error(
            "Spoonacular returned HTTP " +
            std::to_string(response.status_code)
        );
    }

    nlohmann::json data =
        nlohmann::json::parse(response.text);

    // Return the Spoonacular recipe information.
    return data;
}