const label = document.getElementById('label');

function handleLabel() {
    if (window.innerWidth <= 500) {
        label.textContent = "GHMS"
    }
    else {
        label.textContent = "GreenHouse Monitoring System"
    };
}

window.addEventListener('load', handleLabel);
window.addEventListener('resize', handleLabel);