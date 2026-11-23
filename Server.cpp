
#include <iostream>
#include <exception>
#include <string>

#include <httplib.h>
#include <nlohmann/json.hpp>

#include "SpoonacularClient.h"

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
                "GET, OPTIONS"
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