#include "AIClient.h"

#include <iostream>
#include <vector>
#include <sstream>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::string AIClient::generateMealPlan(
    const std::string& goal,
    int calories,
    int days)
{
    std::string apiKey =
        "AQ.Ab8RN6JZD8ANLKPViNehDho-K5fwoziPZyY8NG-jSg14x03YmQ";

    std::string prompt =
        "Create a "
        + std::to_string(days)
        + "-day meal plan.\n\n"
        + "Goal: "
        + goal
        + "\nDaily Calories: "
        + std::to_string(calories)
        + "\n\n"
        + "For each day provide:\n"
        + "Breakfast\n"
        + "Lunch\n"
        + "Dinner\n\n"
        + "Return only meal names.";

    json requestBody =
    {
        {
            "contents",
            { {
                {
                    "parts",
                    { {
                        { "text", prompt }
                    } }
                }
            } }
        }
    };

    auto response =
        cpr::Post(
            cpr::Timeout{ 120000 },
            cpr::Url{
                "https://generativelanguage.googleapis.com/v1beta/models/gemini-3.5-flash-lite:generateContent"
            },
            cpr::Parameters{
                { "key", apiKey }
            },
            cpr::Header{
                {
                    "Content-Type",
                    "application/json"
                }
            },
            cpr::Body{
                requestBody.dump()
            });

    if (response.status_code == 503)
    {
        return
            "Gemini is currently busy. Please try again later.";
    }

    if (response.status_code != 200)
    {
        std::cout
            << "\nSTATUS CODE: "
            << response.status_code
            << std::endl;

        std::cout
            << "\nRESPONSE:\n"
            << response.text
            << std::endl;

        return
            "Failed to generate meal plan.";
    }

    try
    {
        json data =
            json::parse(response.text);

        return
            data["candidates"][0]
            ["content"]["parts"][0]
            ["text"];
    }
    catch (...)
    {
        return
            "Failed to parse AI response.";
    }
}

std::string AIClient::generateMealPlanFromRecipes(
    const std::string& goal,
    int calories,
    const std::vector<std::string>& recipes)
{
    std::string apiKey =
        "AQ.Ab8RN6JZD8ANLKPViNehDho-K5fwoziPZyY8NG-jSg14x03YmQ";

    std::string prompt;

    prompt += "Create a 3-day meal plan.\n\n";
    prompt += "Goal: " + goal + "\n";
    prompt += "Daily Calories: " + std::to_string(calories) + "\n\n";

    prompt +=
        "First categorize each recipe as Breakfast, Lunch, Dinner, Dessert, or Snack.\n"
        "Then create a meal plan using the most appropriate recipes for each meal.\n"
        "Avoid using recipes categorized as Dessert unless no other option exists.\n\n"
        "Recipes:\n";

    int count = 0;

    for (const auto& recipe : recipes)
    {
        prompt += "- " + recipe + "\n";

        count++;

        if (count >= 10)
        {
            break;
        }
    }

    prompt +=
        "\nRules:\n"
        "- Categorize recipes by meal type.\n"
        "- Use breakfast recipes for breakfast.\n"
        "- Use lunch recipes for lunch.\n"
        "- Use dinner recipes for dinner.\n"
        "- Avoid desserts unless no suitable alternative exists.\n"
        "- Maximize variety.\n"
        "- Do not repeat the same meal on consecutive days.\n";

        "Return ONLY this format:\n\n"

        "Day 1\n"
        "Breakfast: Recipe Name\n"
        "Lunch: Recipe Name\n"
        "Dinner: Recipe Name\n\n"

        "Day 2\n"
        "Breakfast: Recipe Name\n"
        "Lunch: Recipe Name\n"
        "Dinner: Recipe Name\n\n"

        "Day 3\n"
        "Breakfast: Recipe Name\n"
        "Lunch: Recipe Name\n"
        "Dinner: Recipe Name\n";

    std::cout << "\nENTERED generateMealPlanFromRecipes()\n";

    std::cout << "\nPROMPT:\n";
    std::cout << prompt << std::endl;

    json requestBody =
    {
        {
            "contents",
            {
                {
                    {
                        "parts",
                        {
                            {
                                {
                                    "text",
                                    prompt
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    std::cout << "\nSENDING GEMINI REQUEST...\n";

    auto response =
        cpr::Post(
            cpr::Timeout{ 30000 },
            cpr::Url{
                "https://generativelanguage.googleapis.com/v1beta/models/gemini-3.5-flash-lite:generateContent"
            },
            cpr::Parameters{
                {
                    "key",
                    apiKey
                }
            },
            cpr::Header{
                {
                    "Content-Type",
                    "application/json"
                }
            },
            cpr::Body{
                requestBody.dump()
            });

    std::cout << "\nGEMINI REQUEST FINISHED\n";

    std::cout << "\nSTATUS CODE: "
        << response.status_code
        << std::endl;

    std::cout << "\nERROR CODE: "
        << static_cast<int>(response.error.code)
        << std::endl;

    std::cout << "\nERROR MESSAGE: "
        << response.error.message
        << std::endl;

    std::cout << "\nRAW RESPONSE:\n"
        << response.text
        << std::endl;

    if (response.status_code != 200)
    {
        return "Failed to generate meal plan.";
    }

    try
    {
        json data = json::parse(response.text);

        if (!data.contains("candidates"))
        {
            return "Gemini returned no candidates.";
        }

        if (data["candidates"].empty())
        {
            return "Gemini returned an empty response.";
        }

        return data["candidates"][0]
            ["content"]["parts"][0]
            ["text"]
            .get<std::string>();
    }
    catch (const std::exception& e)
    {
        std::cout
            << "\nPARSE ERROR: "
            << e.what()
            << std::endl;

        return "Failed to parse Gemini response.";
    }
}

std::string AIClient::askQuestion(
    const std::string& question)
{
    std::string apiKey =
        "AQ.Ab8RN6JZD8ANLKPViNehDho-K5fwoziPZyY8NG-jSg14x03YmQ";

    std::string prompt =
        "Answer this nutrition question in one short paragraph:\n\n";

    prompt += question;

    std::cout
        << "\nQUESTION:\n"
        << prompt
        << std::endl;

    json requestBody =
    {
        {
            "contents",
            { {
                {
                    "parts",
                    { {
                        {
                            "text",
                            prompt
                        }
                    } }
                }
            } }
        }
    };

    std::cout
        << "\nREQUEST BODY:\n"
        << requestBody.dump(4)
        << std::endl;

    auto response =
        cpr::Post(
            cpr::Timeout{ 240000 },
            cpr::Url{
                "https://generativelanguage.googleapis.com/v1beta/models/gemini-3.5-flash-lite:generateContent"
            },
            cpr::Parameters{
                { "key", apiKey }
            },
            cpr::Header{
                {
                    "Content-Type",
                    "application/json"
                }
            },
            cpr::Body{
                requestBody.dump()
            });

    std::cout
        << "\nSTATUS CODE: "
        << response.status_code
        << std::endl;

    std::cout
        << "\nERROR CODE: "
        << static_cast<int>(response.error.code)
        << std::endl;

    std::cout
        << "\nERROR MESSAGE: "
        << response.error.message
        << std::endl;

    if (response.status_code != 200)
    {
        std::cout
            << "\nRESPONSE:\n"
            << response.text
            << std::endl;

        return
            "AI assistant unavailable.";
    }

    try
    {
        json data =
            json::parse(response.text);

        return
            data["candidates"][0]
            ["content"]["parts"][0]
            ["text"];
    }
    catch (...)
    {
        return
            "Failed to process AI response.";
    }
}