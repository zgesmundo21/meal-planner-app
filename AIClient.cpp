#include "AIClient.h"

#include <iostream>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <string>
#include <stdexcept>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace
{
    std::string readGeminiApiKey()
    {
        char* value = nullptr;

        if (_dupenv_s(&value, nullptr, "GEMINI_API_KEY") != 0 ||
            value == nullptr)
        {
            return {};
        }

        std::string key(value);
        std::free(value);
        return key;
    }
}

std::string AIClient::generateMealPlan(
    const std::string& goal,
    int calories,
    int days)
{
    std::string apiKey = readGeminiApiKey();

    if (apiKey.empty())
    {
        throw std::runtime_error(
            "Gemini API key not found. Set GEMINI_API_KEY and restart Visual Studio."
        );
    }

    std::string prompt =
        "You are MealMate, a helpful meal planning assistant.\n\n";

    prompt +=
        "Create a complete "
        + std::to_string(days)
        + "-day meal plan.\n\n";

    prompt += "Dietary goal: " + goal + "\n";

    prompt +=
        "Daily calorie target: "
        + std::to_string(calories)
        + " kcal.\n\n";

    prompt +=
        "Requirements:\n"
        "- Include breakfast, lunch, and dinner for every day.\n"
        "- Include suggested portion sizes.\n"
        "- Include estimated calories for each meal.\n"
        "- Include the estimated total calories for each day.\n"
        "- Aim to meet the daily calorie target.\n"
        "- Provide a variety of meals.\n"
        "- Keep each meal description concise.\n"
        "- Do not skip any days or meals.\n\n";

    prompt +=
        "Follow this exact format for every day:\n\n";

    for (int day = 1; day <= days; day++)
    {
        prompt +=
            "Day " + std::to_string(day) + "\n"
            "Breakfast: Meal name, portion, estimated calories\n"
            "Lunch: Meal name, portion, estimated calories\n"
            "Dinner: Meal name, portion, estimated calories\n"
            "Daily total: Estimated calories\n\n";
    }

    prompt +=
        "Replace all placeholders with actual meal suggestions.\n"
        "Complete every listed day before ending your response.\n"
        "Do not include Markdown formatting, introductions, "
        "or concluding remarks.\n"
        "Return only the completed meal plan.";

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
        },
        {
            "generationConfig",
            {
                { "maxOutputTokens", 8192 },
                { "temperature", 0.5 }
            }
        }
    };

    auto response =
        cpr::Post(
            cpr::Timeout{ 120000 },
            cpr::Url{
                "https://generativelanguage.googleapis.com/v1beta/models/gemini-3.5-flash-lite:generateContent"
            },
            cpr::Header{
                { "Content-Type", "application/json" },
                { "x-goog-api-key", apiKey }
            },
            cpr::Body{
                requestBody.dump()
            });

    if (response.status_code == 503)
    {
        throw std::runtime_error(
            "Gemini is currently busy. Please try again later."
        );
    }

    if (response.status_code != 200)
    {
        std::cerr
            << "\nGEMINI STATUS CODE: "
            << response.status_code
            << std::endl;

        std::cerr
            << "\nGEMINI RESPONSE:\n"
            << response.text
            << std::endl;

        throw std::runtime_error(
            "Failed to generate meal plan."
        );
    }
     
    try
    {        
        json data = json::parse(response.text);

        if (!data.contains("candidates") ||
            data["candidates"].empty())
        {
            throw std::runtime_error(
                "Gemini returned no meal plan."
            );
        }

        const auto& candidate = data["candidates"][0];

        std::string finishReason =
            candidate.value("finishReason", "UNKNOWN");

        std::cout
            << "\nGEMINI FINISH REASON: "
            << finishReason
            << std::endl;

        if (finishReason != "STOP")
        {
            throw std::runtime_error(
                "Gemini did not complete the meal plan: "
                + finishReason
            );
        }

        std::string completePlan;

        for (const auto& part :
            candidate.at("content").at("parts"))
        {
            if (part.contains("text") &&
                !part.value("thought", false))
            {
                completePlan +=
                    part["text"].get<std::string>();
            }
        }

        if (completePlan.empty())
        {
            throw std::runtime_error(
                "Gemini returned an empty meal plan."
            );
        }

        std::istringstream planStream(completePlan);

        std::string line;
        int expectedDay = 1;
        bool breakfast = false;
        bool lunch = false;
        bool dinner = false;

        while (std::getline(planStream, line))
        {            
            const auto start =
                line.find_first_not_of(" \t\r");

            if (start == std::string::npos)
            {
                continue;
            }

            line = line.substr(
                start,
                line.find_last_not_of(" \t\r") - start + 1
            );

            if (line.rfind("Day ", 0) == 0)
            {
                if (expectedDay > 1 &&
                    (!breakfast || !lunch || !dinner))
                {
                    throw std::runtime_error(
                        "A day is missing one or more meals."
                    );
                }

                if (expectedDay > days ||
                    line != "Day " + std::to_string(expectedDay))
                {
                    throw std::runtime_error(
                        "Unexpected meal plan day."
                    );
                }

                expectedDay++;

                breakfast = false;
                lunch = false;
                dinner = false;
            }
            else if (expectedDay > 1)
            {
                if (line.rfind("Breakfast:", 0) == 0 &&
                    line.find_first_not_of(" \t", 10) != std::string::npos)
                {
                    breakfast = true;
                }

                if (line.rfind("Lunch:", 0) == 0 &&
                    line.find_first_not_of(" \t", 6) != std::string::npos)
                {
                    lunch = true;
                }

                if (line.rfind("Dinner:", 0) == 0 &&
                    line.find_first_not_of(" \t", 7) != std::string::npos)
                {
                    dinner = true;
                }
            }
        }

        if (expectedDay != days + 1 ||
            !breakfast || !lunch || !dinner)
        {
            throw std::runtime_error(
                "Gemini returned an incomplete meal plan."
            );
        }

        std::cout
            << "\nCOMPLETE MEAL PLAN:\n"
            << completePlan
            << std::endl;

        return completePlan;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "\nMEAL PLAN ERROR: "
            << e.what()
            << std::endl;

        throw;
    }
}

std::string AIClient::generateMealPlanFromRecipes(
    const std::string& goal,
    int calories,
    const std::vector<std::string>& recipes)
{
    std::string apiKey = readGeminiApiKey();

    if (apiKey.empty())
    {
        return "Gemini API key not found. Set GEMINI_API_KEY and restart Visual Studio.";
    }

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
        "- Do not repeat the same meal on consecutive days.\n"

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
            cpr::Header{
                { "Content-Type", "application/json" },
                { "x-goog-api-key", apiKey }
            },
            cpr::Body{
                requestBody.dump()
            });

    std::cout << "\nGEMINI REQUEST FINISHED\n";
    
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
    std::string apiKey = readGeminiApiKey();

    if (apiKey.empty())
    {
        return "Gemini API key not found. Set GEMINI_API_KEY and restart Visual Studio.";
    }

    std::string prompt =
        "Answer this nutrition question in one short paragraph:\n\n";

    prompt += question;
	
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
            cpr::Timeout{ 240000 },
            cpr::Url{
                "https://generativelanguage.googleapis.com/v1beta/models/gemini-3.5-flash-lite:generateContent"
            },
            cpr::Header{
                { "Content-Type", "application/json" },
                { "x-goog-api-key", apiKey }
            },
            cpr::Body{
                requestBody.dump()
            });

    if (response.status_code != 200)
    {
        std::cerr << "Gemini request failed (HTTP status: "
                  << response.status_code << ").\n";
        return "The nutrition assistant is unavailable right now. Please try again.";
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