#include "Meal.h"

Meal::Meal()
{
    id = 0;
    name = "";
}

Meal::Meal(
    int id,
    std::string name)
{
    this->id = id;
    this->name = name;
}

int Meal::getId() const
{
    return id;
}

std::string Meal::getName() const
{
    return name;
}