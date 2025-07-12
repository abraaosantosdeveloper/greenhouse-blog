function updateProgressBarThrottled() {
    let isScrolling = false;

    function updateProgress() {
        const progressBar = document.getElementById('progressBar');
        const totalHeight = document.documentElement.scrollHeight - window.innerHeight;
        const scrollPosition = window.pageYOffset || document.documentElement.scrollTop;
        const scrollPercentage = Math.min((scrollPosition / totalHeight) * 100, 100);
        const backToTop = document.getElementById("backToTop");

        progressBar.style.width = scrollPercentage + '%';
        isScrolling = false;

        if (window.pageYOffset != 0) {
            backToTop.classList.add('visible');
            
        } else {
            backToTop.classList.remove('visible');
        }
    };

    if (!isScrolling) {
        requestAnimationFrame(updateProgress);
        isScrolling = true;
    };

};




function scrollToTop() {
    window.scrollTo({
        top: 0,
        behavior: 'smooth'
    });
}

// Versão otimizada para usar
function initOptimizedProgressTracker() {
    window.addEventListener('scroll', updateProgressBarThrottled);
    updateProgressBarThrottled();
}

// Descomente a linha abaixo se quiser usar a versão otimizada
document.addEventListener('DOMContentLoaded', initOptimizedProgressTracker);
