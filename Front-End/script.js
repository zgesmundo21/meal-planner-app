const navButtons = document.querySelectorAll(".nav-btn");

const pages = document.querySelectorAll(".page");

function showPage(pageId) {

    pages.forEach(page => {
        page.classList.remove("active");
    });

    navButtons.forEach(button => {
        button.classList.remove("active");
    });

    const selectedPage = document.getElementById(pageId);

    if (!selectedPage) {
        return;
    }

    selectedPage.classList.add("active");

    const activeButton = document.querySelector(
        `.nav-btn[data-page="${pageId}"]`
    );

    if (activeButton) {
        activeButton.classList.add("active");
    }
}

navButtons.forEach(button => {
    button.addEventListener("click", () => {
        showPage(button.dataset.page);
    });
});

document.getElementById("start-planning")
    .addEventListener("click", () => {
        showPage("planner");
    });

document.getElementById("recipe-form")
    .addEventListener("submit", event => {
        event.preventDefault();

        document.getElementById("recipe-results")
            .textContent =
            "Recipe search will be connected to Spoonacular soon.";
    });

document.getElementById("planner-form")
    .addEventListener("submit", event => {
        event.preventDefault();

        document.getElementById("meal-plan-result")
            .textContent =
            "AI meal planning will be connected to Gemini soon.";
    });