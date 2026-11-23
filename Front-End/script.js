
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

// Fetch and display full recipe details.
async function showRecipe(id) {
    const dialog = $("#recipe-dialog");
    const content = $("#recipe-dialog-content");

    content.textContent = "Loading recipe details...";
    dialog.showModal();

    try {
        // Expected GET /api/recipes/:id
        const r = await api(
            `/api/recipes/${encodeURIComponent(id)}`
        );

        content.replaceChildren();

        const h = document.createElement("h2");
        h.textContent = r.title || "Recipe details";
        content.append(h);

        const meta = document.createElement("p");
        meta.textContent = [
            r.servings ? `${r.servings} servings` : null,
            r.readyInMinutes ? `${r.readyInMinutes} minutes` : null
        ].filter(Boolean).join(" · ");
        content.append(meta);

        if (r.nutrition) {
            const n = document.createElement("p");
            n.textContent =
                `Per serving: ${r.nutrition.calories ?? "—"} kcal · ` +
                `${r.nutrition.protein ?? "—"} g protein · ` +
                `${r.nutrition.carbs ?? "—"} g carbs · ` +
                `${r.nutrition.fat ?? "—"} g fat`;
            content.append(n);
        }

        if (Array.isArray(r.ingredients)) {
            const h3 = document.createElement("h3");
            h3.textContent = "Ingredients";
            content.append(h3);

            const ul = document.createElement("ul");

            r.ingredients.forEach(item => {
                const li = document.createElement("li");
                li.textContent = typeof item === "string"
                    ? item
                    : String(item.name || "");
                ul.append(li);
            });

            content.append(ul);

            const add = document.createElement("button");
            add.className = "button button-outline";
            add.textContent = "+ Add ingredients to grocery list";

            add.addEventListener("click", () => {
                r.ingredients.forEach(item => {
                    const name = typeof item === "string"
                        ? item
                        : String(item.name || "");

                    if (
                        name.trim() &&
                        !grocery.some(
                            g => g.name.toLowerCase() === name.toLowerCase()
                        )
                    ) {
                        grocery.push({
                            id: crypto.randomUUID(),
                            name,
                            done: false
                        });
                    }
                });

                save();
                renderGrocery();
                add.textContent = "✓ Added to grocery list";
            });

            content.append(add);
        }

        if (r.instructions) {
            const h3 = document.createElement("h3");
            h3.textContent = "Instructions";
            content.append(h3);

            const instructions = document.createElement("p");
            instructions.textContent = String(r.instructions);
            content.append(instructions);
        }
    } catch (e) {
        content.textContent =
            `Couldn't load recipe details: ${e.message}`;
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


// ==========================================
// TOP BAR RECIPE SEARCH
// ==========================================

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