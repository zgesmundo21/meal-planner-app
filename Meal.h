#pragma once

#include <string>

class Meal
{
private:
    int id;
    std::string name;

public:
    Meal();

    Meal(
        int id,
        std::string name);

    int getId() const;

    std::string getName() const;
};
