
#include <iostream>
#include <exception>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <filesystem>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "SpoonacularClient.h"
#include "AIClient.h"

using json = nlohmann::json;

namespace fs = std::filesystem;

static std::string locateFrontendFolder()
{
    fs::path current = fs::current_path();

    for (int level = 0; level < 6; ++level)
    {
        fs::path frontend = current / "Front-End";

        if (fs::is_directory(frontend) &&
            fs::is_regular_file(frontend / "Index.html"))
        {
            return frontend.string();
        }

        if (current == current.parent_path())
        {
            break;
        }

        current = current.parent_path();
    }

    return "";
}

int main()
{
    httplib::Server server;

    std::string frontendFolder = locateFrontendFolder();

    if (frontendFolder.empty())
    {
        std::cerr
            << "Unable to locate frontend folder."
            << std::endl;

        std::cerr
            << "Current working directory: "
            << fs::current_path()
            << std::endl;

        return 1;
    }

    std::cout
        << "Frontend folder located: "
        << frontendFolder
        << std::endl;

    if (!server.set_mount_point("/", frontendFolder))
    {
        std::cerr
            << "Unable to mount frontend folder."
            << std::endl;

        return 1;
    }

    server.Get("/",
        [](const httplib::Request& req,
            httplib::Response& res)
        {
            res.set_redirect("/Index.html");
        }
    );

    server.set_pre_routing_handler(
        [](const httplib::Request& req,
            httplib::Response& res)
        {            
            const std::string origin =
                req.get_header_value("Origin");

            if (origin == "http://localhost:5500" ||
                origin == "http://127.0.0.1:5500")
            {
                res.set_header(
                    "Access-Control-Allow-Origin", origin
                );
                res.set_header(
                    "Vary", "Origin"
                );
            }

            res.set_header(
                "Access-Control-Allow-Methods",
                "GET, POST, OPTIONS"
            );

            res.set_header(
                "Access-Control-Allow-Headers",
                "Content-Type"
            );

            if (req.method == "OPTIONS")
            {
                res.status = 204;
                return httplib::Server::HandlerResponse::Handled;
            }

            return httplib::Server::HandlerResponse::Unhandled;
        }
    );

    server.Get("/api/health",
        [](const httplib::Request& req,
            httplib::Response& res)
        {
            json result = {
                { "status", "ok" },
                { "message", "MealMate backend is running" }
            };

            res.set_content(
                result.dump(),
                "application/json"
            );
        }
    );

    server.Get("/api/recipes",
        [](const httplib::Request& req,
            httplib::Response& res)
        {            
            if (!req.has_param("query"))
            {
                res.status = 400;

                res.set_content(
                    json{
                        { "error", "Missing search query." }
                    }.dump(),
                    "application/json"
                );

                return;
            }

            std::string query =
                req.get_param_value("query");

            if (query.empty() || query.size() > 200)
            {
                res.status = 400;

                res.set_content(
                    json{
                        { "error", "Search query must be 1-200 characters." }
                    }.dump(),
                    "application/json"
                );

                return;
            }

            int number = 12;

            int offset = 0;

            try
            {
                if (req.has_param("number"))
                {
                    std::string value =
                        req.get_param_value("number");

                    size_t parsed = 0;

                    number = std::stoi(value, &parsed);

                    if (parsed != value.size())
                    {
                        throw std::invalid_argument("Invalid number");
                    }
                }

                if (req.has_param("offset"))
                {
                    std::string value =
                        req.get_param_value("offset");

                    size_t parsed = 0;

                    offset = std::stoi(value, &parsed);

                    if (parsed != value.size())
                    {
                        throw std::invalid_argument("Invalid offset");
                    }
                }

                if (number < 1 || number > 24 ||
                    offset < 0 || offset > 900)
                {
                    throw std::invalid_argument(
                        "Pagination values are out of range."
                    );
                }
            }
            catch (const std::exception&)
            {
                res.status = 400;

                res.set_content(
                    json{
                        {
                            "error",
                            "Invalid recipe pagination parameters."
                        }
                    }.dump(),
                    "application/json"
                );

                return;
            }

            try
            {                
                json results =
                    SpoonacularClient::searchRecipesJson(
                        query,
                        number,
                        offset
                    );

                res.set_content(
                    results.dump(),
                    "application/json"
                );

                std::cout
                    << "Recipe search completed: "
                    << query
                    << " | Number: "
                    << number
                    << " | Offset: "
                    << offset
                    << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr
                    << "Recipe search error: "
                    << e.what()
                    << std::endl;

                res.status = 502;

                res.set_content(
                    json{
                        { "error", "Recipe search failed. Check the backend console." }
                    }.dump(),
                    "application/json"
                );
            }
        }
    );

    server.Get(R"(/api/recipes/(\d+))",
        [](const httplib::Request& req,
            httplib::Response& res)
        {
            try
            {                
                int recipeId =
                    std::stoi(req.matches[1].str());

                nlohmann::json recipe =
                    SpoonacularClient::getRecipeDetailsJson(recipeId);

                res.set_content(
                    recipe.dump(),
                    "application/json"
                );

                std::cout
                    << "Recipe details retrieved: "
                    << recipeId
                    << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr
                    << "Recipe details error: "
                    << e.what()
                    << std::endl;

                res.status = 502;

                res.set_content(
                    nlohmann::json{
                        { "error", "Unable to retrieve recipe details." }
                    }.dump(),
                    "application/json"
                );
            }
        }
    );
   
    server.Post("/api/meal-plan",
        [](const httplib::Request& req,
            httplib::Response& res)
        {
            auto badRequest = [&](const std::string& error)
                {
                    res.status = 400;

                    res.set_content(
                        json{
                            { "error", error }
                        }.dump(),
                        "application/json"
                    );
                };

            json input = json::parse(
                req.body,
                nullptr,
                false
            );

            if (!input.is_object() ||
                !input.contains("goal") ||
                !input["goal"].is_string() ||
                !input.contains("calories") ||
                !input["calories"].is_number_integer() ||
                !input.contains("days") ||
                !input["days"].is_number_integer())
            {
                badRequest("Invalid meal plan input.");
                return;
            }

            if ((input.contains("restrictions") &&
                !input["restrictions"].is_string()) ||

                (input.contains("cookingTime") &&
                    !input["cookingTime"].is_number_integer()) ||

                (input.contains("requirements") &&
                    !input["requirements"].is_string()))
            {
                badRequest("Invalid personalization preferences.");
                return;
            }

            if (input["calories"] < 1200 ||
                input["calories"] > 6000 ||
                (input["days"] != 3 &&
                    input["days"] != 5 &&
                    input["days"] != 7))
            {
                badRequest("Invalid calorie target or number of days.");
                return;
            }

            if (input.contains("cookingTime") &&
                (input["cookingTime"] < 0 ||
                    input["cookingTime"] > 180))
            {
                badRequest("Invalid cooking time.");
                return;
            }

            std::string goal =
                input["goal"].get<std::string>();

            int calories =
                input["calories"].get<int>();

            int days =
                input["days"].get<int>();

            std::string restrictions =
                input.value("restrictions", std::string(""));

            int cookingTime =
                input.value("cookingTime", 0);

            std::string requirements =
                input.value("requirements", std::string(""));

            if (goal.find_first_not_of(" \t\r\n")
                == std::string::npos ||
                goal.size() > 200 ||
                restrictions.size() > 2000 ||
                requirements.size() > 1000)
            {
                badRequest("Invalid meal planning preferences.");
                return;
            }

            try
            {
                AIClient ai;

                std::cout
                    << "Generating personalized Gemini meal plan..."
                    << std::endl;

                std::string plan =
                    ai.generateMealPlan(
                        goal,
                        calories,
                        days,
                        restrictions,
                        cookingTime,
                        requirements
                    );

                res.set_content(
                    json{
                        { "plan", plan },
                        { "days", days },
                        { "calories", calories }
                    }.dump(),
                    "application/json"
                );

                std::cout
                    << "AI meal plan generated successfully."
                    << std::endl;
            }

            catch (const std::exception& e)
            {
                std::cerr
                    << "Meal planner error: "
                    << e.what()
                    << std::endl;

                res.status = 502;

                res.set_content(
                    json{
                        {
                            "error",
                            "Meal plan generation failed. "
                            "Please try again."
                        }
                    }.dump(),
                    "application/json"
                );
            }
        }
    );

    server.Post("/api/nutrition",
        [](const httplib::Request& req,
            httplib::Response& res)
        {
            try
            {
                json input = json::parse(req.body);

                if (!input.is_object() ||
                    !input.contains("question") ||
                    !input["question"].is_string())
                {
                    res.status = 400;

                    res.set_content(
                        json{
                            { "error", "A nutrition question is required." }
                        }.dump(),
                        "application/json"
                    );

                    return;
                }

                std::string question =
                    input["question"].get<std::string>();

                if (question.empty() || question.size() > 2000)
                {
                    res.status = 400;

                    res.set_content(
                        json{
                            { "error", "Question must be 1-2000 characters." }
                        }.dump(),
                        "application/json"
                    );

                    return;
                }

                AIClient ai;

                std::cout
                    << "Processing Gemini nutrition question..."
                    << std::endl;

                std::string answer =
                    ai.askQuestion(question);

                res.set_content(
                    json{
                        { "answer", answer }
                    }.dump(),
                    "application/json"
                );
            }
            catch (const json::exception&)
            {
                res.status = 400;

                res.set_content(
                    json{
                        { "error", "Invalid JSON request." }
                    }.dump(),
                    "application/json"
                );
            }
            catch (const std::exception& e)
            {
                std::cerr
                    << "Nutrition assistant error: "
                    << e.what()
                    << std::endl;

                res.status = 502;

                res.set_content(
                    json{
                        { "error", "Nutrition assistant failed." }
                    }.dump(),
                    "application/json"
                );
            }
        }
    );

    server.Post("/api/adapt-recipe",
        [](const httplib::Request& req,
            httplib::Response& res)
        {
            json input = json::parse(
                req.body,
                nullptr,
                false
            );

            if (!input.is_object() ||
                !input.contains("recipeId") ||
                !input["recipeId"].is_number_integer() ||
                !input.contains("restrictions") ||
                !input["restrictions"].is_string())
            {
                res.status = 400;

                res.set_content(
                    json{
                        { "error", "Invalid recipe customization input." }
                    }.dump(),
                    "application/json"
                );

                return;
            }

            if (input["recipeId"] <= 0 ||
                input["recipeId"] > 2147483647)
            {
                res.status = 400;

                res.set_content(
                    json{
                        { "error", "Invalid recipe ID." }
                    }.dump(),
                    "application/json"
                );

                return;
            }

            int recipeId =
                input["recipeId"].get<int>();

            std::string restrictions =
                input["restrictions"].get<std::string>();

            if (restrictions.find_first_not_of(" \t\r\n")
                == std::string::npos ||
                restrictions.size() > 2000)
            {
                res.status = 400;

                res.set_content(
                    json{
                        { "error", "Please provide valid dietary requirements." }
                    }.dump(),
                    "application/json"
                );

                return;
            }

            try
            {
                std::cout
                    << "Retrieving original recipe: "
                    << recipeId
                    << std::endl;

                json recipe =
                    SpoonacularClient::getRecipeDetailsJson(
                        recipeId
                    );

                if (!recipe.is_object())
                {
                    throw std::runtime_error(
                        "Original recipe is unavailable."
                    );
                }

                bool hasIngredients =
                    (recipe.contains("extendedIngredients") &&
                        recipe["extendedIngredients"].is_array() &&
                        !recipe["extendedIngredients"].empty())
                    ||
                    (recipe.contains("ingredients") &&
                        recipe["ingredients"].is_array() &&
                        !recipe["ingredients"].empty());

                if (!hasIngredients)
                {
                    throw std::runtime_error(
                        "Original recipe ingredients are unavailable."
                    );
                }

                std::string recipeDetails =
                    recipe.dump();

                AIClient ai;

                std::cout
                    << "Customizing recipe with Gemini..."
                    << std::endl;

                std::string adaptedRecipe =
                    ai.adaptRecipe(
                        recipeDetails,
                        restrictions
                    );

                res.set_content(
                    json{
                        { "recipeId", recipeId },
                        { "adaptedRecipe", adaptedRecipe }
                    }.dump(),
                    "application/json"
                );

                std::cout
                    << "Recipe customization completed."
                    << std::endl;
            }
            catch (const std::exception& e)
            {
                std::cerr
                    << "Recipe customization error: "
                    << e.what()
                    << std::endl;

                res.status = 502;

                res.set_content(
                    json{
                        { "error", "Unable to customize this recipe. Please try again." }
                    }.dump(),
                    "application/json"
                );
            }
        }
    );
      
    const char* portEnv = std::getenv("PORT");

    int port = portEnv
        ? std::stoi(portEnv)
        : 8080;

    std::string host = portEnv
        ? "0.0.0.0"
        : "127.0.0.1";

    std::cout
        << "MealMate backend starting..."
        << std::endl;

    std::cout
        << "Server address: http://"
        << host
        << ":"
        << port
        << std::endl;

    std::cout
        << "Press Ctrl+C to stop the server."
        << std::endl;

    if (!server.listen(host, port))
    {
        std::cerr
            << "Unable to start server on port "
            << port
            << std::endl;

        return 1;
    }

    return 0;
}