#include <iostream>
#include <vector>
#include <string>

#include "Meal.h"
#include "SpoonacularClient.h"
#include "AIClient.h"
#include "Favorites.h"
#include "GroceryList.h"

using namespace std;

int main()
{
    SpoonacularClient spoonacular;
    AIClient ai;

    int choice;

    do
    {
        cout << "\n=================================";
        cout << "\nMEALMATE";
        cout << "\n=================================\n";

        cout << "1. Search Recipes" << endl;
        cout << "2. AI Meal Planner" << endl;
        cout << "3. View Favorites" << endl;
        cout << "4. View Grocery List" << endl;
        cout << "5. Remove Grocery Item" << endl;
        cout << "6. AI Nutrition Assistant" << endl;
        cout << "7. Exit" << endl;

        cout << "\nChoice: ";
        cin >> choice;
        cin.ignore();

        switch (choice)
        {
        case 1:
        {
            string search;

            cout << "\nSearch recipe: ";
            getline(cin, search);

            vector<Meal> meals =
                spoonacular.searchMeals(search);

            if (meals.empty())
            {
                cout
                    << "\nNo recipes found."
                    << endl;

                break;
            }

            cout
                << "\nRESULTS\n\n";

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

            int recipeChoice;

            cout
                << "\nSelect recipe: ";

            cin
                >> recipeChoice;

            cin.ignore();

            if (recipeChoice >= 1 &&
                recipeChoice <= meals.size())
            {
                Meal selectedMeal =
                    meals[recipeChoice - 1];

                cout
                    << "\nLoading recipe details..."
                    << endl;

                spoonacular.displayRecipeDetails(
                    selectedMeal.getId());

                vector<string> equipment =
                    spoonacular.getEquipment(
                        selectedMeal.getId());

                cout
                    << "\n================================="
                    << "\nEQUIPMENT NEEDED"
                    << "\n=================================\n\n";

                for (const auto& item : equipment)
                {
                    cout
                        << "- "
                        << item
                        << endl;
                }

                char groceryChoice;

                cout
                    << "\nGenerate grocery list? (y/n): ";

                cin
                    >> groceryChoice;

                cin.ignore();

                if (groceryChoice == 'y' ||
                    groceryChoice == 'Y')
                {
                    vector<string> ingredients =
                        spoonacular.getIngredients(
                            selectedMeal.getId());

                    GroceryList::saveList(
                        ingredients);

                    cout
                        << "Grocery list updated."
                        << endl;
                }

                char favoriteChoice;

                cout
                    << "\nSave to favorites? (y/n): ";

                cin
                    >> favoriteChoice;

                cin.ignore();

                if (favoriteChoice == 'y' ||
                    favoriteChoice == 'Y')
                {
                    Favorites::saveRecipe(
                        selectedMeal.getId(),
                        selectedMeal.getName());

                    cout
                        << "Recipe saved."
                        << endl;
                }
            }
            else
            {
                cout
                    << "Invalid selection."
                    << endl;
            }

            break;
        }

        case 2:
        {
            string goal;
            int calories;

            cout
                << "\nGoal: ";

            getline(cin, goal);

            cout
                << "Daily Calories: ";

            cin >> calories;

            cin.ignore();

            vector<Meal> meals =
                spoonacular.getHealthyRecipes(goal);

            if (meals.empty())
            {
                cout
                    << "\nFailed to retrieve recipes from Spoonacular."
                    << endl;

                break;
            }

            vector<string> recipeNames;

            for (const auto& meal : meals)
            {
                recipeNames.push_back(
                    meal.getName());
            }

            string mealPlan =
                ai.generateMealPlanFromRecipes(
                    goal,
                    calories,
                    recipeNames);

            cout
                << "\n================================="
                << "\nAI MEAL PLAN"
                << "\n=================================\n\n";

            cout
                << mealPlan
                << endl;

            break;
        }

        case 3:
        {
            Favorites::displayFavorites();

            break;
        }

        case 4:
        {
            GroceryList::viewList();

            break;
        }

        case 5:
        {
            string item;

            GroceryList::viewList();

            cout
                << "\nEnter ingredient to remove: ";

            getline(cin, item);

            GroceryList::removeItem(item);

            cout
                << "\nIngredient removed."
                << endl;

            break;
        }

        case 6:
        {
            string question;

            cout
                << "\nAsk a nutrition question: ";

            getline(cin, question);

            cout
                << "\n================================="
                << "\nAI NUTRITION ASSISTANT"
                << "\n=================================\n\n";

            cout
                << ai.askQuestion(question)
                << endl;

            break;
        }

        case 7:
        {
            cout
                << "\nGoodbye!"
                << endl;

            break;
        }

        default:
        {
            cout
                << "\nInvalid option."
                << endl;

            break;
        }
        }

    } while (choice != 7);

    return 0;
}
