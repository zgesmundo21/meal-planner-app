#include <iostream>
#include <vector>

#include "Meal.h"
#include "SpoonacularClient.h"

using namespace std;

int main()
{
    SpoonacularClient spoonacular;

    string search;

    cout << "Search recipe: ";
    getline(cin, search);

    vector<Meal> meals =
        spoonacular.searchMeals(search);

    if (meals.empty())
    {
        cout << "\nNo recipes found."
            << endl;

        return 0;
    }

    cout << "\nRESULTS\n\n";

    for (int i = 0;
        i < meals.size();
        i++)
    {
        cout
            << i + 1
            << ". "
            << meals[i].getName()
            << endl;
    }

    int choice;

    cout << "\nSelect recipe: ";
    cin >> choice;

    if (choice >= 1 &&
        choice <= meals.size())
    {
        spoonacular.displayRecipeDetails(
            meals[choice - 1].getId());
    }
    else
    {
        cout << "Invalid selection."
            << endl;
    }

    return 0;
}
