
"use strict";

// MealMate frontend.
// API requests go through the C++ backend.

// Change this URL if C++ server uses a different port.
const API_BASE =
    window.location.port === "5500"
        ? "http://localhost:8080"
        : window.location.origin;

const STORE = {
    favorites: "mealmate-favorites-v1",
    grocery: "mealmate-grocery-v1"
};

const $ = (selector) => document.querySelector(selector);

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

const RECIPES_PER_PAGE = 12;

let currentRecipeQuery = "";

let recipeOffset = 0;

let totalRecipeResults = 0;

let hasMoreRecipes = false;

let isLoadingRecipes = false;

const save = () => {
    localStorage.setItem(STORE.favorites, JSON.stringify(favorites));
    localStorage.setItem(STORE.grocery, JSON.stringify(grocery));
    updateCounts();
};

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

const message = (selector, text, isError = false) => {
    const node = $(selector);
    node.textContent = text;
    node.classList.toggle("error", isError);
};

async function api(path, options = {}) {
    let response;

    try {
        response = await fetch(`${API_BASE}${path}`, {
            ...options,
            headers: {
                "Content-Type": "application/json",
                ...(options.headers || {})
            }
        });
    } catch (error) {
        console.error("Backend connection failed:", {
            path,
            error
        });

        throw new Error(
            `Could not connect to the MealMate backend at ${API_BASE}. ` +
            "Check that MealMateServer.exe is running and that " +
            "the requested endpoint and CORS settings are correct."
        );
    }

    const text = await response.text();

    let data;

    try {
        data = text ? JSON.parse(text) : {};
    } catch {
        throw new Error(
            `Backend returned invalid JSON (HTTP ${response.status}).`
        );
    }

    if (!response.ok) {
        throw new Error(
            data.error ||
            data.message ||
            `Backend request failed (HTTP ${response.status}).`
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
        const recipe = await api(
            `/api/recipes/${encodeURIComponent(id)}`
        );

        const ingredients = getRecipeIngredients(recipe);
        const equipment = getRecipeEquipment(recipe);
        const steps = getRecipeSteps(recipe);

        const image = typeof recipe.image === "string" &&
            /^https:\/\/(?:[\w-]+\.)*spoonacular\.com\//i
                .test(recipe.image)
            ? recipe.image
            : "";

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

                    <!-- AI RECIPE CUSTOMIZATION -->
<section class="recipe-customization">

    <!-- Mate Header -->
    <div class="customization-header">

        <span class="customization-icon"
              aria-hidden="true">
            ✧
        </span>

        <div>
            <h3>Customize with Mate</h3>

            <p>
                Make this recipe work for you.
            </p>
        </div>

    </div>

    <!-- Customization Form -->
    <form id="recipe-customization-form"
          class="customization-body">

        <fieldset class="customization-fieldset">

            <legend class="customization-label">
                Allergies and dietary restrictions
            </legend>

            <p class="customization-description">
                Select all that apply.
            </p>

            <div class="restriction-grid">

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Milk allergy">
                    <span>Milk allergy</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Wheat allergy">
                    <span>Wheat allergy</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Egg allergy">
                    <span>Egg allergy</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Peanut allergy">
                    <span>Peanut allergy</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Tree nut allergy">
                    <span>Tree nut allergy</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Soy allergy">
                    <span>Soy allergy</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Fish allergy">
                    <span>Fish allergy</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Shellfish allergy">
                    <span>Shellfish allergy</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Sesame allergy">
                    <span>Sesame allergy</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Gluten-free">
                    <span>Gluten-free</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Vegetarian">
                    <span>Vegetarian</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Vegan">
                    <span>Vegan</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Lactose-free">
                    <span>Lactose-free</span>
                </label>

                <label class="restriction-option">
                    <input type="checkbox"
                           name="restriction"
                           value="Low sodium">
                    <span>Low sodium</span>
                </label>

            </div>

        </fieldset>

        <!-- Additional Requirements -->
        <div>

            <label for="customization-requirements"
                   class="customization-label">
                Additional requirements
            </label>

            <textarea
                id="customization-requirements"
                class="customization-input"
                placeholder="e.g. No coconut, no mushrooms, or other dietary requirements..."
                maxlength="1000"
                rows="3"></textarea>

        </div>

        <!-- Generate Customized Recipe -->
        <button type="submit"
                class="customize-recipe-button">
            ✧ Customize my recipe
        </button>

        <!-- Status Messages -->
        <p id="customization-message"
           class="customization-message"
           role="status"
           aria-live="polite"></p>

        <!-- Allergy Safety Disclaimer -->
        <div class="allergy-warning">

            <strong>Important allergy information</strong>

            AI-generated recipes are suggestions and
            are not verified for allergen safety.
            Always check ingredient labels, manufacturer
            information and potential cross-contact risks.

        </div>

    </form>

    <!-- AI Customized Recipe Results -->
    <div id="adapted-recipe-result"
         class="adapted-recipe"
         aria-live="polite"
         hidden>
    </div>

</section>

<!-- Original Recipe Label -->
<p class="original-recipe-heading">
    ORIGINAL SPOONACULAR RECIPE
</p>

                    <!-- INGREDIENTS -->
                    <section class="recipe-detail-section">

                        <h3>Original ingredients</h3>

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
                                + Add original ingredients to grocery list
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

        // Connect the AI recipe customization form.
        setupRecipeCustomization(
            content,
            id
        );

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

function setupRecipeCustomization(content, recipeId) {

    const form = content.querySelector(
        "#recipe-customization-form"
    );

    if (!form) return;

    const submitButton = form.querySelector(
        "button[type='submit']"
    );

    const status = form.querySelector(
        "#customization-message"
    );

    const additionalInput = form.querySelector(
        "#customization-requirements"
    );

    const result = content.querySelector(
        "#adapted-recipe-result"
    );

    function updateStatus(text, isError = false) {

        status.textContent = text;

        status.classList.toggle(
            "error",
            isError
        );
    }

    form.addEventListener("input", () => {

        result.hidden = true;
        result.replaceChildren();

        updateStatus("");
    });

    form.addEventListener("submit", async (event) => {

        event.preventDefault();

        if (submitButton.disabled) {
            return;
        }

        const selectedRestrictions = Array.from(
            form.querySelectorAll(
                'input[name="restriction"]:checked'
            )
        ).map(input => input.value);

        const additionalRequirements =
            additionalInput.value.trim();

        const requirements = [
            ...selectedRestrictions
        ];

        if (additionalRequirements) {

            requirements.push(
                additionalRequirements
            );
        }

        if (!requirements.length) {

            updateStatus(
                "Please select or enter at least one dietary requirement.",
                true
            );

            return;
        }

        const restrictions =
            requirements.join("; ");

        const originalButtonText =
            submitButton.textContent;

        result.hidden = true;
        result.replaceChildren();

        const controls = Array.from(
            form.querySelectorAll(
                "input, textarea, button"
            )
        );

        controls.forEach(control => {
            control.disabled = true;
        });

        submitButton.textContent =
            "✧ Mate is customizing your recipe...";

        updateStatus(
            "Reviewing ingredients and finding suitable substitutions..."
        );

        result.setAttribute(
            "aria-busy",
            "true"
        );

        try {

            const data = await api(
                "/api/adapt-recipe",
                {
                    method: "POST",

                    body: JSON.stringify({
                        recipeId: Number(recipeId),
                        restrictions: restrictions
                    })
                }
            );

            if (
                typeof data.adaptedRecipe !== "string" ||
                !data.adaptedRecipe.trim()
            ) {

                throw new Error(
                    "The backend did not return a valid customized recipe."
                );
            }

            if (
                !form.isConnected ||
                !$("#recipe-dialog").open
            ) {
                return;
            }

            renderAdaptedRecipe(
                result,
                data.adaptedRecipe,
                restrictions
            );

            result.hidden = false;

            updateStatus(
                "Your suggested recipe adaptation is ready!"
            );

            result.scrollIntoView({
                behavior: "smooth",
                block: "start"
            });

        } catch (error) {

            console.error(
                "Recipe customization error:",
                error
            );

            updateStatus(
                `Unable to customize recipe: ${error.message}`,
                true
            );

        } finally {

            controls.forEach(control => {
                control.disabled = false;
            });

            submitButton.textContent =
                originalButtonText;

            result.removeAttribute(
                "aria-busy"
            );
        }
    });
}

function renderAdaptedRecipe(
    container,
    recipeText,
    restrictions
) {

    container.replaceChildren();

    const badge = document.createElement("span");

    badge.className = "adapted-recipe-badge";

    badge.textContent =
        "✧ CUSTOMIZED BY MATE";

    container.append(badge);

    const heading = document.createElement("h3");

    heading.textContent =
        "Your customized recipe";

    container.append(heading);

    const requirements = document.createElement("p");

    requirements.className =
        "customization-description";

    requirements.textContent =
        `Requested adaptations: ${restrictions}`;

    container.append(requirements);

    const sectionHeadings = {

        "ADAPTED RECIPE":
            "Adapted recipe",

        "INGREDIENT SUBSTITUTIONS":
            "Ingredient substitutions",

        "UPDATED INGREDIENTS":
            "Updated ingredients",

        "EQUIPMENT":
            "Equipment",

        "UPDATED INSTRUCTIONS":
            "Updated instructions",

        "ALLERGY AND DIETARY NOTES":
            "Allergy and dietary notes"
    };

    const sections = [];

    let currentSection = {
        title: "",
        lines: []
    };

    const lines = recipeText
        .replace(/\r\n?/g, "\n")
        .split("\n");

    for (const line of lines) {

        const normalized = line
            .trim()
            .replace(/^#+\s*/, "")
            .replace(/\*\*/g, "")
            .replace(/:\s*$/, "")
            .trim()
            .toUpperCase();

        if (
            Object.prototype.hasOwnProperty.call(
                sectionHeadings,
                normalized
            )
        ) {

            if (
                currentSection.lines.length ||
                currentSection.title
            ) {

                sections.push(
                    currentSection
                );
            }

            currentSection = {
                title: sectionHeadings[normalized],
                lines: []
            };

        } else {

            currentSection.lines.push(line);
        }
    }

    if (
        currentSection.lines.length ||
        currentSection.title
    ) {

        sections.push(currentSection);
    }

    for (const section of sections) {

        const sectionText =
            section.lines.join("\n").trim();

        if (!sectionText) {
            continue;
        }

        const block =
            document.createElement("section");

        block.className =
            "adapted-recipe-section";

        if (
            section.title ===
            "Allergy and dietary notes"
        ) {

            block.classList.add(
                "safety-section"
            );
        }

        if (section.title) {

            const title =
                document.createElement("h4");

            title.textContent =
                section.title;

            block.append(title);
        }

        const body =
            document.createElement("div");

        body.className =
            "adapted-recipe-text";

        body.textContent =
            sectionText;

        block.append(body);

        container.append(block);
    }

    const warning =
        document.createElement("p");

    warning.className =
        "allergy-warning";

    warning.textContent =
        "This adaptation has not been verified for allergen safety. " +
        "Check all ingredients, labels, preparation surfaces and " +
        "equipment before preparing or consuming the recipe.";

    container.append(warning);
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

window.addEventListener("hashchange", navigate);

function updateRecipePagination() {

    const pagination = $("#recipe-pagination");
    const loadMoreButton = $("#load-more-recipes");
    const count = $("#recipe-count");
    const progress = $("#recipe-progress-fill");

    if (!pagination || !loadMoreButton ||
        !count || !progress) {

        console.error(
            "Missing recipe pagination HTML elements."
        );

        return;
    }

    console.log("Recipe pagination:", {
        displayed: searchResults.length,
        total: totalRecipeResults,
        offset: recipeOffset,
        hasMore: hasMoreRecipes,
        loading: isLoadingRecipes
    });

    pagination.hidden = searchResults.length === 0;

    const displayed = searchResults.length;

    count.textContent =
        `Showing ${displayed} of ${totalRecipeResults} recipes`;

    const percentage =
        totalRecipeResults > 0
            ? (displayed / totalRecipeResults) * 100
            : 0;

    progress.style.width =
        `${Math.min(100, percentage)}%`;

    loadMoreButton.hidden = !hasMoreRecipes;

    loadMoreButton.disabled = isLoadingRecipes;

    loadMoreButton.textContent =
        isLoadingRecipes
            ? "Loading more recipes..."
            : "Load more recipes ↓";
}

async function fetchRecipes(reset = false) {

    if (isLoadingRecipes) {
        return;
    }

    if (!currentRecipeQuery) {
        return;
    }

    if (!reset && !hasMoreRecipes) {
        return;
    }

    const searchButton =
        $("#search-form button[type='submit']")
        || $("#search-form button");

    if (reset) {

        recipeOffset = 0;

        totalRecipeResults = 0;

        hasMoreRecipes = false;

        searchResults = [];

        $("#recipe-results").replaceChildren();

        $("#recipe-pagination").hidden = true;
    }

    isLoadingRecipes = true;

    searchButton.disabled = true;

    updateRecipePagination();

    message(
        "#recipe-message",
        reset
            ? "Searching recipes..."
            : "Loading more recipes..."
    );

    try {

        const params = new URLSearchParams({

            query: currentRecipeQuery,

            number: String(RECIPES_PER_PAGE),

            offset: String(recipeOffset)

        });

        const data = await api(
            `/api/recipes?${params.toString()}`
        );

        if (!Array.isArray(data.recipes) ||
            !Number.isInteger(data.totalResults) ||
            !Number.isInteger(data.nextOffset) ||
            typeof data.hasMore !== "boolean" ||
            data.totalResults < 0 ||
            data.nextOffset < recipeOffset ||
            (data.hasMore &&
             data.nextOffset <= recipeOffset)) {

            throw new Error(
                "Invalid recipe response from the backend."
            );
        }

        const newRecipes = data.recipes
            .map(normalizeRecipe)
            .filter(recipe =>
                Number.isFinite(recipe.id)
            );

        const existingIds = new Set(
            searchResults.map(recipe => recipe.id)
        );

        for (const recipe of newRecipes) {

            if (!existingIds.has(recipe.id)) {

                searchResults.push(recipe);

                existingIds.add(recipe.id);
            }
        }

        totalRecipeResults = data.totalResults;

        recipeOffset = data.nextOffset;

        hasMoreRecipes = data.hasMore;

        renderRecipes();

        message(
            "#recipe-message",
            
            searchResults.length
        ? `${totalRecipeResults.toLocaleString()} matching recipes found.`
        : "No recipes found. Try a different search."
);

    } catch (error) {

        console.error(
            "Recipe pagination error:",
            error
        );

        message(
            "#recipe-message",

            reset
                ? `Recipe search unavailable: ${error.message}`
                : `Unable to load more recipes: ${error.message}`,

            true
        );

    } finally {

        isLoadingRecipes = false;

        searchButton.disabled = false;

        updateRecipePagination();
    }
}

$("#search-form").addEventListener("submit", async (e) => {

    e.preventDefault();

    if (isLoadingRecipes) {
        return;
    }

    const query = $("#recipe-query").value.trim();

    if (!query) {
        return;
    }

    currentRecipeQuery = query;

    await fetchRecipes(true);
});

$("#load-more-recipes").addEventListener(
    "click",
    async () => {

        if (isLoadingRecipes ||
            !hasMoreRecipes) {

            return;
        }

        await fetchRecipes(false);
    }
);

function renderMealPlan(plan) {

    const container = $("#plan-result");

    container.replaceChildren();

    const lines = plan
        .split(/\r?\n/)
        .map(line => line.trim())
        .filter(Boolean);

    let currentDay = null;

    for (const line of lines) {

        const dayMatch = line.match(/^Day\s+(\d+)$/i);

        if (dayMatch) {

            currentDay = document.createElement("article");
            currentDay.className = "meal-day-card";

            const heading = document.createElement("h3");
            heading.className = "meal-day-heading";
            heading.textContent = `Day ${dayMatch[1]}`;

            currentDay.append(heading);
            container.append(currentDay);

            continue;
        }

        if (!currentDay) {
            continue;
        }

        const mealMatch = line.match(
            /^(Breakfast|Lunch|Dinner):\s*(.+)$/i
        );

        if (mealMatch) {

            const mealType = mealMatch[1].toLowerCase();
            const mealDescription = mealMatch[2];

            const icons = {
                breakfast: "☀",
                lunch: "☼",
                dinner: "☾"
            };

            const meal = document.createElement("div");
            meal.className = "planned-meal";

            const icon = document.createElement("span");
            icon.className = "planned-meal-icon";
            icon.textContent = icons[mealType];
            icon.setAttribute("aria-hidden", "true");

            const details = document.createElement("div");
            details.className = "planned-meal-details";

            const label = document.createElement("strong");
            label.className = "planned-meal-label";
            label.textContent = mealMatch[1];

            const description = document.createElement("p");
            description.textContent = mealDescription;

            details.append(label, description);
            meal.append(icon, details);
            currentDay.append(meal);

            continue;
        }

        const totalMatch = line.match(
            /^Daily total:\s*(.+)$/i
        );

        if (totalMatch) {

            const total = document.createElement("div");
            total.className = "meal-day-total";

            total.textContent =
                `Estimated daily total: ${totalMatch[1]}`;

            currentDay.append(total);
        }
    }

    if (!container.children.length) {
        throw new Error(
            "The AI returned an unrecognized meal plan format."
        );
    }
}

$("#plan-form").addEventListener("submit", async (e) => {

    e.preventDefault();

    const submitButton =
        $("#plan-form button[type='submit']");

    const goal = $("#goal").value;

    const calories =
        Number($("#calories").value);

    const days =
        Number($("#days").value);

    const selectedRestrictions = [
        ...document.querySelectorAll(
            "#plan-form input[name='plan-restriction']:checked"
        )
    ].map(input => input.value);

    const restrictions =
        selectedRestrictions.join(", ");

    const cookingTime =
        Number($("#plan-cooking-time").value);

    const requirements =
        $("#plan-requirements").value.trim();

    if (!goal ||
        !Number.isInteger(calories) ||
        calories < 1200 ||
        calories > 6000 ||
        ![3, 5, 7].includes(days) ||
        !Number.isInteger(cookingTime) ||
        cookingTime < 0 ||
        cookingTime > 180 ||
        requirements.length > 1000) {

        message(
            "#plan-message",
            "Please enter valid meal planning preferences.",
            true
        );

        return;
    }

    message(
        "#plan-message",
        "Mate is creating your personalized meal plan. "
        + "This may take a moment..."
    );

    $("#plan-result").hidden = true;
    $("#plan-result").replaceChildren();

    submitButton.disabled = true;

    submitButton.textContent =
        "✧ Creating your meal plan...";

    try {

        const data = await api("/api/meal-plan", {

            method: "POST",

            body: JSON.stringify({

                goal: goal,
                calories: calories,
                days: days,
                restrictions: restrictions,
                cookingTime: cookingTime,
                requirements: requirements

            })
        });

        if (typeof data.plan !== "string" ||
            !data.plan.trim()) {

            throw new Error(
                "The backend did not return a valid meal plan."
            );
        }

        renderMealPlan(data.plan);

        $("#plan-result").hidden = false;

        message(
            "#plan-message",
            "Your personalized "
            + days
            + "-day meal plan is ready!"
        );

    } catch (error) {

        console.error(
            "Gemini meal planner error:",
            error
        );

        $("#plan-result").hidden = true;

        message(
            "#plan-message",
            `Meal planner unavailable: ${error.message}`,
            true
        );

    } finally {

        submitButton.disabled = false;

        submitButton.textContent =
            "✧ Generate my meal plan";
    }
});

function scrollAssistantMessages() {

    const messages = $("#assistant-messages");

    messages.scrollTop = messages.scrollHeight;

}

function addAssistantMessage(text, sender) {

    const messages = $("#assistant-messages");

    const bubble = document.createElement("div");

    bubble.classList.add("chat-message");

    if (sender === "user") {

        bubble.classList.add("user-message");

    } else {

        bubble.classList.add("assistant-message");

    }

    bubble.textContent = text;

    messages.appendChild(bubble);

    scrollAssistantMessages();

    return bubble;

}

$("#nutrition-form").addEventListener("submit", async (e) => {

    e.preventDefault();

    const questionInput = $("#nutrition-question");

    const submitButton = $(
        "#nutrition-form button[type='submit']"
    );

    const suggestionButtons =
        document.querySelectorAll(".assistant-prompt");

    const question = questionInput.value.trim();

    if (!question) {

        message(
            "#nutrition-message",
            "Please enter a question.",
            true
        );

        return;

    }

    if (submitButton.disabled) {
        return;
    }

    message("#nutrition-message", "");

    addAssistantMessage(question, "user");

    questionInput.value = "";

    submitButton.disabled = true;

    questionInput.readOnly = true;

    suggestionButtons.forEach(button => {

        button.disabled = true;

    });

    const loadingMessage = addAssistantMessage(
        "Mate is thinking...",
        "assistant"
    );

    loadingMessage.classList.add("thinking");


    try {

        const data = await api("/api/nutrition", {

            method: "POST",

            body: JSON.stringify({
                question: question
            })

        });

        if (typeof data.answer !== "string" ||
            !data.answer.trim()) {

            throw new Error(
                "The backend did not return a valid answer."
            );

        }

        loadingMessage.textContent = data.answer;

        loadingMessage.classList.remove("thinking");

        scrollAssistantMessages();


    } catch (error) {

        console.error("Ask Mate error:", error);

        loadingMessage.textContent =
            "Sorry, I couldn't answer your question right now. "
            + "Please try again.";

        loadingMessage.classList.remove("thinking");

        loadingMessage.classList.add("chat-error");

        scrollAssistantMessages();


    } finally {

        submitButton.disabled = false;

        questionInput.readOnly = false;

        suggestionButtons.forEach(button => {

            button.disabled = false;

        });

        if ($("#ai-assistant-dialog").open) {

            questionInput.focus();

        }

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

// ==========================================
// HOMEPAGE DATE AND DYNAMIC GREETING
// ==========================================

function updateHomeGreeting() {

    const now = new Date();

    // Locate the greeting elements in Index.html.
    const dateElement = $("#welcome-date");
    const greetingElement = $("#welcome-greeting");

    // Display the current date.
    if (dateElement) {
        dateElement.textContent = now.toLocaleDateString(
            "en-CA",
            {
                weekday: "long",
                month: "long",
                day: "numeric"
            }
        ).toUpperCase();
    }

    // Determine the appropriate greeting.
    const hour = now.getHours();

    let greeting;

    if (hour < 12) {
        greeting = "Good morning!";
    } else if (hour < 17) {
        greeting = "Good afternoon!";
    } else {
        greeting = "Good evening!";
    }

    // Update the homepage greeting.
    if (greetingElement) {
        greetingElement.textContent = greeting;
    }
}

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

    recipeInput.value = searchTerm;

    window.location.hash = "recipes";

    recipeForm.requestSubmit();

});

function initializeAssistant() {

    // Locate the assistant elements.
    const askMateButton = $("#open-ai-assistant");
    const assistantDialog = $("#ai-assistant-dialog");
    const closeAssistantButton = $("#close-ai-assistant");
    const assistantQuestionInput = $("#nutrition-question");

    // Check that the necessary HTML elements exist.
    if (!askMateButton || !assistantDialog ||
        !closeAssistantButton || !assistantQuestionInput) {

        console.warn("Ask Mate assistant elements not found.");
        return;
    }

    // Open the AI assistant popup.
        askMateButton.addEventListener("click", () => {

        if (!assistantDialog.open) {

            assistantDialog.showModal();

            assistantQuestionInput.focus();

            scrollAssistantMessages();

        }

    });

    // Close the assistant using the X button.
    closeAssistantButton.addEventListener("click", () => {
        assistantDialog.close();
    });

    // Close the assistant when clicking outside the popup.
    assistantDialog.addEventListener("click", (event) => {

        const bounds = assistantDialog.getBoundingClientRect();

        const clickedOutside =
            event.clientX < bounds.left ||
            event.clientX > bounds.right ||
            event.clientY < bounds.top ||
            event.clientY > bounds.bottom;

        if (event.target === assistantDialog && clickedOutside) {
            assistantDialog.close();
        }

    });

    // Return keyboard focus to the homepage button.
    assistantDialog.addEventListener("close", () => {
        askMateButton.focus();
    });

    // Handle suggested AI questions.
    assistantDialog.querySelectorAll(".assistant-prompt")
        .forEach((button) => {

            button.addEventListener("click", () => {

                const question = button.dataset.question;

                const submitButton = $(
                    "#nutrition-form button[type='submit']"
                );

                if (!question || submitButton.disabled) {
                    return;
                }

                assistantQuestionInput.value = question;

                $("#nutrition-form").requestSubmit();

            });

        });

}

updateHomeGreeting();

initializeAssistant();

setInterval(updateHomeGreeting, 60000);

updateCounts();
renderFavorites();
renderGrocery();

navigate();

checkAPI();