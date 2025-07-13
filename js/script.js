const label = document.getElementById('label');

function handleLabel() {
    if (window.innerWidth <= 540) {
        label.textContent = "GHMS"
    }
    else {
        label.textContent = "GreenHouse Monitoring System"
    };
}

window.addEventListener('load', handleLabel);
window.addEventListener('resize', handleLabel);


const video = document.querySelector('video');

// Controles programáticos
video.play();
video.pause();

// Event listeners
video.addEventListener('loadedmetadata', () => {
    console.log('Duração:', video.duration);
});

video.addEventListener('timeupdate', () => {
    console.log('Tempo atual:', video.currentTime);
});