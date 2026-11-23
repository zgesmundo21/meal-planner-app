
#include <iostream>
#include <exception>
#include <string>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "SpoonacularClient.h"
#include "AIClient.h"

using json = nlohmann::json;

int main()
{
    httplib::Server server;

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

            try
            {                
                json results =
                    SpoonacularClient::searchRecipesJson(query);

                res.set_content(
                    results.dump(),
                    "application/json"
                );

                std::cout
                    << "Recipe search completed: "
                    << query
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

    // Example request: GET /api/recipes/123456

    server.Get(R"(/api/recipes/(\d+))",
        [](const httplib::Request& req,
            httplib::Response& res)
        {
            try
            {
                // Extract the recipe ID from the URL.
                int recipeId =
                    std::stoi(req.matches[1].str());

                // Retrieve recipe information from Spoonacular.
                nlohmann::json recipe =
                    SpoonacularClient::getRecipeDetailsJson(recipeId);

                // Send the recipe data to the frontend.
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
            try
            {
                json input = json::parse(req.body);

                if (!input.is_object() ||
                    !input.contains("goal") ||
                    !input["goal"].is_string() ||
                    !input.contains("calories") ||
                    !input["calories"].is_number_integer() ||
                    !input.contains("days") ||
                    !input["days"].is_number_integer())
                {
                    res.status = 400;
                    res.set_content(
                        json{ { "error", "Invalid meal plan input." } }.dump(),
                        "application/json"
                    );
                    return;
                }

                std::string goal =
                    input["goal"].get<std::string>();

                int calories = input["calories"].get<int>();
                int days = input["days"].get<int>();

                if (goal.empty() || goal.size() > 200 ||
                    calories < 500 || calories > 6000 ||
                    (days != 3 && days != 5 && days != 7))
                {
                    res.status = 400;
                    res.set_content(
                        json{ { "error", "Invalid meal planning values." } }.dump(),
                        "application/json"
                    );
                    return;
                }

                AIClient ai;

                std::cout
                    << "Generating Gemini meal plan..."
                    << std::endl;

                std::string plan =
                    ai.generateMealPlan(goal, calories, days);

                res.set_content(
                    json{ { "plan", plan } }.dump(),
                    "application/json"
                );
            }
            catch (const json::exception&)
            {
                res.status = 400;
                res.set_content(
                    json{ { "error", "Invalid JSON request." } }.dump(),
                    "application/json"
                );
            }
            catch (const std::exception& e)
            {
                std::cerr
                    << "Meal planner error: "
                    << e.what()
                    << std::endl;

                res.status = 502;
                res.set_content(
                    json{ { "error", "Meal plan generation failed." } }.dump(),
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
                // Read the JSON sent from the frontend.
                json input = json::parse(req.body);

                // Make sure the request contains a question.
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

                // Prevent empty or excessively long questions.
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

                // Create an instance of your existing Gemini client.
                AIClient ai;

                std::cout
                    << "Processing Gemini nutrition question..."
                    << std::endl;

                // Call your existing AIClient.cpp function.
                std::string answer =
                    ai.askQuestion(question);

                // Send the response back to JavaScript.
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
      
    std::cout
        << "MealMate backend starting..."
        << std::endl;

    std::cout
        << "API address: http://localhost:8080"
        << std::endl;

    std::cout
        << "Press Ctrl+C to stop the server."
        << std::endl;

    if (!server.listen("127.0.0.1", 8080))
    {
        std::cerr
            << "Unable to start server on port 8080."
            << std::endl;

        return 1;
    }

    return 0;
}