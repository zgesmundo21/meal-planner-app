
cmake_minimum_required(VERSION 3.20)

project(MealPlanner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(MealPlanner
    MealPlanner.cpp
    Meal.cpp
    SpoonacularClient.cpp
    "Back-end/AIClient.cpp"
    Favorites.cpp
    GroceryList.cpp
)

target_link_libraries(MealPlanner PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
    fmt::fmt
)