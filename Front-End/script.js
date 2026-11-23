
"use strict";

// MealMate frontend.
// API requests go through the C++ backend.
// Never put Gemini or Spoonacular API keys here.

// Change this URL if your C++ server uses a different port.
const API_BASE = "http://localhost:8080";

// Browser storage keys.
const STORE = {
    favorites: "mealmate-favorites-v1",
    grocery: "mealmate-grocery-v1"
};

const $ = (selector) => document.querySelector(selector);

// Read saved data safely from localStorage.
const load = (key) => {
    try {
        const data = JSON.parse(localStorage.getItem(key));
        return Array.isArray(data) ? data : [];
    } catch {
        return [];
    }
};

let favorites = load(STORE.favorites);
let grocery = load(STORE.grocery);
let searchResults = [];

// Save favorites and grocery items.
const save = () => {
    localStorage.setItem(STORE.favorites, JSON.stringify(favorites));
    localStorage.setItem(STORE.grocery, JSON.stringify(grocery));
    updateCounts();
};

// Escape text if it needs to be inserted into HTML.
const escapeHTML = (value) =>
    String(value ?? "").replace(/[&<>"']/g, c => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        "\"": "&quot;",
        "'": "&#39;"
    }[c]));


// ==========================================
// RECIPE DETAILS HELPERS
// ==========================================

// Convert Spoonacular HTML instructions into
// readable, individual cooking steps.
function getRecipeSteps(recipe) {
    const groups = Array.isArray(recipe.analyzedInstructions)
        ? recipe.analyzedInstructions
        : [];

    const structuredSteps = groups.flatMap(group =>
        Array.isArray(group.steps) ? group.steps : []
    );

    if (structuredSteps.length > 0) {
        return structuredSteps
            .map(item => String(item.step || "").trim())
            .filter(Boolean);
    }

    // Fallback: Spoonacular sometimes returns HTML
    // such as <ol><li>Step 1</li></ol>.
    const doc = new DOMParser().parseFromString(
        String(recipe.instructions || ""),
        "text/html"
    );

    const listItems = [...doc.querySelectorAll("li")];

    if (listItems.length > 0) {
        return listItems
            .map(item => item.textContent.trim())
            .filter(Boolean);
    }

    const text = doc.body.textContent.trim();

    return text ? [text] : [];
}

// Extract equipment from Spoonacular's
// analyzed instructions when available.
function getRecipeEquipment(recipe) {
    const equipment = new Set();

    for (const group of recipe.analyzedInstructions ?? []) {
        for (const step of group.steps ?? []) {
            for (const item of step.equipment ?? []) {
                if (item.name) {
                    equipment.add(item.name);
                }
            }
        }
    }

    return [...equipment];
}
function getRecipeIngredients(recipe) {
    const source = Array.isArray(recipe.extendedIngredients)
        ? recipe.extendedIngredients
        : Array.isArray(recipe.ingredients)
            ? recipe.ingredients
            : [];

    return source
        .map(item => {
            if (typeof item === "string") {
                return {
                    display: item,
                    name: item
                };
            }

            return {
                display: String(
                    item.original || item.originalString ||
                    item.name || ""
                ),
                name: String(item.name || item.original || "")
            };
        })
        .filter(item => item.display.trim());
}

// Display informational and error messages.
const message = (selector, text, isError = false) => {
    const node = $(selector);
    node.textContent = text;
    node.classList.toggle("error", isError);
};

// Send HTTP requests to the C++ backend.
async function api(path, options = {}) {
    const response = await fetch(`${API_BASE}${path}`, {
        ...options,
        headers: {
            "Content-Type": "application/json",
            ...(options.headers || {})
        }
    });

    const text = await response.text();

    let data;
    try {
        data = text ? JSON.parse(text) : {};
    } catch {
        throw new Error(
            `Server returned non-JSON data (HTTP ${response.status}).`
        );
    }

    if (!response.ok) {
        throw new Error(
            data.error ||
            data.message ||
            `Request failed (HTTP ${response.status}).`
        );
    }

    return data;
}

// Navigate between the five website sections.
function navigate() {
    const page = location.hash.replace("#", "") || "home";
    const selected = document.getElementById(page) ? page : "home";

    document.querySelectorAll(".page").forEach(el => {
        el.classList.toggle("active", el.id === selected);
    });

    document.querySelectorAll("[data-page]").forEach(el => {
        const active = el.dataset.page === selected;
        el.classList.toggle("active", active);

        if (active) {
            el.setAttribute("aria-current", "page");
        } else {
            el.removeAttribute("aria-current");
        }
    });

    window.scrollTo({ top: 0, behavior: "instant" });

    if (selected === "favorites") renderFavorites();
    if (selected === "grocery") renderGrocery();
}

// Update the counts shown on the homepage.
function updateCounts() {
    $("#home-fav-count").textContent = favorites.length;
    $("#home-grocery-count").textContent =
        grocery.filter(i => !i.done).length;
}

// Convert backend recipe data into a consistent format.
function normalizeRecipe(r) {
    return {
        id: Number(r.id),
        title: String(r.title || "Untitled recipe"),
        image: typeof r.image === "string" ? r.image : ""
    };
}

// Create a recipe card.
function recipeCard(r, saved = false) {
    const card = document.createElement("article");
    card.className = "recipe-card";

    const visual = document.createElement("div");
    visual.className = "recipe-visual";

    if (r.image && /^https:\/\//.test(r.image)) {
        const img = document.createElement("img");
        img.src = r.image;
        img.alt = "";
        img.loading = "lazy";
        img.style.cssText =
            "width:100%;height:100%;object-fit:cover";
        visual.append(img);
    } else {
        visual.textContent = "🥗";
    }

    const body = document.createElement("div");
    body.className = "recipe-card-body";

    const title = document.createElement("h3");
    title.textContent = r.title;
    body.append(title);

    const actions = document.createElement("div");
    actions.className = "recipe-actions";

    const details = document.createElement("button");
    details.className = "small-button";
    details.textContent = "View details";
    details.addEventListener("click", () => showRecipe(r.id));
    actions.append(details);

    const fav = document.createElement("button");
    fav.className = "small-button secondary";
    fav.textContent = saved
        ? "♥ Remove favorite"
        : "♡ Save recipe";

    fav.addEventListener("click", () => {
        if (favorites.some(x => x.id === r.id)) {
            favorites = favorites.filter(x => x.id !== r.id);
        } else {
            favorites.push(r);
        }

        save();
        renderFavorites();
        renderRecipes();
    });

    actions.append(fav);
    body.append(actions);
    card.append(visual, body);

    return card;
}

// Display recipe search results.
function renderRecipes() {
    const root = $("#recipe-results");
    root.replaceChildren();

    searchResults.forEach(r => {
        root.append(
            recipeCard(r, favorites.some(f => f.id === r.id))
        );
    });
}

// Display saved favorite recipes.
function renderFavorites() {
    const root = $("#favorites-list");
    root.replaceChildren();

    if (!favorites.length) {
        root.innerHTML =
            '<div class="empty-state">♡' +
            '<strong>No favorites yet</strong>' +
            'Search for recipes and save the ones you love.</div>';
        return;
    }

    favorites.forEach(r => root.append(recipeCard(r, true)));
}

async function showRecipe(id) {
    const dialog = $("#recipe-dialog");
    const content = $("#recipe-dialog-content");

    content.textContent = "Loading recipe details...";
    dialog.showModal();

    try {
        // Retrieve the full recipe from our C++ backend.
        const recipe = await api(
            `/api/recipes/${encodeURIComponent(id)}`
        );

        const ingredients = getRecipeIngredients(recipe);
        const equipment = getRecipeEquipment(recipe);
        const steps = getRecipeSteps(recipe);

        // Only allow HTTPS images from Spoonacular.
        const image = typeof recipe.image === "string" &&
            /^https:\/\/(?:[\w-]+\.)*spoonacular\.com\//i
                .test(recipe.image)
            ? recipe.image
            : "";

        // Build the recipe details layout.
        content.innerHTML = `
            <article class="recipe-detail">

                ${image ? `
                    <img class="recipe-detail-image"
                         src="${escapeHTML(image)}"
                         alt="${escapeHTML(recipe.title || "Recipe")}">
                ` : ""}

                <div class="recipe-detail-content">

                    <span class="eyebrow muted">
                        MEALMATE RECIPE
                    </span>

                    <h2>
                        ${escapeHTML(recipe.title || "Recipe details")}
                    </h2>

                    <div class="recipe-detail-meta">
                        <span>
                            ◷ ${Number(recipe.readyInMinutes) || "—"} minutes
                        </span>

                        <span>
                            ♧ ${Number(recipe.servings) || "—"} servings
                        </span>
                    </div>

                    <!-- INGREDIENTS -->
                    <section class="recipe-detail-section">

                        <h3>Ingredients</h3>

                        ${ingredients.length ? `
                            <ul class="recipe-ingredient-list">
                                ${ingredients.map(item => `
                                    <li>
                                        <label>
                                            <input type="checkbox">
                                            <span>
                                                ${escapeHTML(item.display)}
                                            </span>
                                        </label>
                                    </li>
                                `).join("")}
                            </ul>

                            <button id="add-recipe-ingredients"
                                    class="button button-outline"
                                    type="button">
                                + Add ingredients to grocery list
                            </button>
                        ` : `
                            <p>Ingredients are not available.</p>
                        `}

                    </section>

                    <!-- EQUIPMENT -->
                    <section class="recipe-detail-section">

                        <h3>Equipment</h3>

                        ${equipment.length ? `
                            <div class="recipe-equipment-list">
                                ${equipment.map(item => `
                                    <span>${escapeHTML(item)}</span>
                                `).join("")}
                            </div>
                        ` : `
                            <p>
                                Equipment information is not available
                                for this recipe.
                            </p>
                        `}

                    </section>

                    <!-- INSTRUCTIONS -->
                    <section class="recipe-detail-section">

                        <h3>Step-by-step instructions</h3>

                        ${steps.length ? `
                            <ol class="recipe-steps">
                                ${steps.map(step => `
                                    <li>${escapeHTML(step)}</li>
                                `).join("")}
                            </ol>
                        ` : `
                            <p>Instructions are not available.</p>
                        `}

                    </section>

                </div>
            </article>
        `;

        // Preserve your existing grocery-list functionality.
        const addButton = content.querySelector(
            "#add-recipe-ingredients"
        );

        addButton?.addEventListener("click", () => {
            for (const item of ingredients) {
                const name = item.name.trim();

                if (!name) continue;

                const alreadyAdded = grocery.some(
                    g => g.name.toLowerCase() === name.toLowerCase()
                );

                if (!alreadyAdded) {
                    grocery.push({
                        id: crypto.randomUUID(),
                        name,
                        done: false
                    });
                }
            }

            save();
            renderGrocery();

            addButton.textContent = "✓ Added to grocery list";
            addButton.disabled = true;
        });

    } catch (error) {
        content.textContent =
            `Couldn't load recipe details: ${error.message}`;
    }
}

// Display and manage grocery-list items.
function renderGrocery() {
    const root = $("#grocery-list");
    root.replaceChildren();

    const done = grocery.filter(i => i.done).length;
    $("#grocery-progress").textContent =
        `${done} of ${grocery.length} completed`;

    if (!grocery.length) {
        root.innerHTML =
            '<li class="empty-state">🛒' +
            '<strong>Your list is looking fresh</strong>' +
            'Add your first grocery item above.</li>';
        return;
    }

    grocery.forEach(item => {
        const li = document.createElement("li");
        li.className = `grocery-item${item.done ? " done" : ""}`;

        const label = document.createElement("label");

        const checkbox = document.createElement("input");
        checkbox.type = "checkbox";
        checkbox.checked = item.done;

        checkbox.addEventListener("change", () => {
            item.done = checkbox.checked;
            save();
            renderGrocery();
        });

        const text = document.createElement("span");
        text.textContent = item.name;

        label.append(checkbox, text);

        const del = document.createElement("button");
        del.className = "small-button secondary";
        del.textContent = "Remove";
        del.setAttribute("aria-label", `Remove ${item.name}`);

        del.addEventListener("click", () => {
            grocery = grocery.filter(i => i.id !== item.id);
            save();
            renderGrocery();
        });

        li.append(label, del);
        root.append(li);
    });
}

// Check whether the C++ HTTP server is running.
async function checkAPI() {
    try {
        await api("/api/health");
        $("#api-status").textContent = "● API connected";
        $("#api-status").classList.remove("offline");
    } catch {
        $("#api-status").textContent = "● Preview mode";
        $("#api-status").classList.add("offline");
    }
}

// Listen for navigation changes.
window.addEventListener("hashchange", navigate);

// Recipe search form.
$("#search-form").addEventListener("submit", async (e) => {
    e.preventDefault();

    const query = $("#recipe-query").value.trim();
    if (!query) return;

    message("#recipe-message", "Searching recipes...");
    $("#recipe-results").replaceChildren();

    try {
        const data = await api(
            `/api/recipes?query=${encodeURIComponent(query)}`
        );

        if (!Array.isArray(data.recipes)) {
            throw new Error("Expected a recipes array from the backend.");
        }

        searchResults = data.recipes
            .map(normalizeRecipe)
            .filter(r => Number.isFinite(r.id));

        renderRecipes();

        message(
            "#recipe-message",
            searchResults.length
                ? `${searchResults.length} recipes found.`
                : "No recipes found. Try a different search."
        );
    } catch (err) {
        message(
            "#recipe-message",
            `Recipe search unavailable: ${err.message}`,
            true
        );
    }
});

// AI meal planner form.
$("#plan-form").addEventListener("submit", async (e) => {
    e.preventDefault();

    message("#plan-message", "Generating your meal plan...");
    $("#plan-result").hidden = true;

    try {
        const data = await api("/api/meal-plan", {
            method: "POST",
            body: JSON.stringify({
                goal: $("#goal").value,
                calories: Number($("#calories").value),
                days: Number($("#days").value)
            })
        });

        if (typeof data.plan !== "string") {
            throw new Error("Expected a plan string from the backend.");
        }

        $("#plan-result").textContent = data.plan;
        $("#plan-result").hidden = false;
        message("#plan-message", "Here's your personalized plan!");
    } catch (err) {
        message(
            "#plan-message",
            `Meal planner unavailable: ${err.message}`,
            true
        );
    }
});

// AI nutrition assistant form.
$("#nutrition-form").addEventListener("submit", async (e) => {
    e.preventDefault();

    message("#nutrition-message", "Thinking about your question...");
    $("#nutrition-result").hidden = true;

    try {
        const data = await api("/api/nutrition", {
            method: "POST",
            body: JSON.stringify({
                question: $("#nutrition-question").value.trim()
            })
        });

        if (typeof data.answer !== "string") {
            throw new Error("Expected an answer string from the backend.");
        }

        $("#nutrition-result").textContent = data.answer;
        $("#nutrition-result").hidden = false;
        message("#nutrition-message", "Here's what I found:");
    } catch (err) {
        message(
            "#nutrition-message",
            `Nutrition assistant unavailable: ${err.message}`,
            true
        );
    }
});

// Add grocery items.
$("#grocery-form").addEventListener("submit", e => {
    e.preventDefault();

    const name = $("#grocery-input").value.trim();
    if (!name) return;

    grocery.push({
        id: crypto.randomUUID(),
        name,
        done: false
    });

    $("#grocery-input").value = "";
    save();
    renderGrocery();
});

// Clear completed grocery items.
$("#clear-completed").addEventListener("click", () => {
    grocery = grocery.filter(i => !i.done);
    save();
    renderGrocery();
});

// Close the recipe details dialog.
$("#close-dialog").addEventListener("click", () => {
    $("#recipe-dialog").close();
});

$("#recipe-dialog").addEventListener("click", e => {
    if (e.target === $("#recipe-dialog")) {
        $("#recipe-dialog").close();
    }
});

// Initialize MealMate.
updateCounts();
renderFavorites();
renderGrocery();
navigate();
checkAPI();

const topbarSearchForm =
    document.getElementById("topbar-search-form");

const topbarSearchInput =
    document.getElementById("topbar-search-input");

topbarSearchForm?.addEventListener("submit", function (event) {
     
    event.preventDefault();

    // Get the user's search text
    const searchTerm = topbarSearchInput.value.trim();

    if (!searchTerm) {
        return;
    }

    // Find the existing recipe search elements
    const recipeInput =
        document.getElementById("recipe-query");

    const recipeForm =
        document.getElementById("search-form");

    if (!recipeInput || !recipeForm) {
        console.error("Recipe search form not found.");
        return;
    }

    // Copy the search into Discover Recipes
    recipeInput.value = searchTerm;

    // Navigate to the Recipes section
    window.location.hash = "recipes";

    // Run the existing recipe search handler
    recipeForm.requestSubmit();

});