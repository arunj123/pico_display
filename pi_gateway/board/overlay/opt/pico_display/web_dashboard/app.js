// Dashboard App - Fetches and displays data
const API_BASE = '';
const SENSOR_NAMES = ['Kindr', 'Wohn', 'Flur', 'Bad', 'Kuche', 'Schlf'];

// Weather icon mapping using icon names from API
const WEATHER_ICONS = {
    'sun': '☀️', 'moon': '🌙',
    'sun_cloud': '⛅', 'moon_cloud': '🌤️',
    'cloud': '☁️', 'fog': '🌫️',
    'drizzle': '🌧️', 'light_rain': '🌦️', 'rain': '🌧️', 'heavy_rain': '🌧️',
    'freezing_rain': '🌨️',
    'light_snow': '🌨️', 'snow': '❄️', 'heavy_snow': '🌨️',
    'storm': '⛈️', 'storm_hail': '⛈️'
};

// Humidity face emoji based on level
function getHumidityFace(hum) {
    if (hum < 30) return '😟';  // Too dry
    if (hum <= 60) return '😊'; // Good
    return '😐'; // Too humid
}

function updateTime() {
    const now = new Date();
    document.getElementById('current-time').textContent =
        now.toLocaleTimeString('de-DE', { hour: '2-digit', minute: '2-digit' });
    document.getElementById('current-date').textContent =
        now.toLocaleDateString('en-US', { weekday: 'short', month: 'short', day: 'numeric', year: 'numeric' });
}

async function fetchTheme() {
    try {
        const res = await fetch(`${API_BASE}/api/theme`);
        const theme = await res.json();
        document.documentElement.style.setProperty('--gradient-start', theme.gradient_start);
        document.documentElement.style.setProperty('--gradient-end', theme.gradient_end);
        document.documentElement.style.setProperty('--text-primary', theme.text_primary);
        document.documentElement.style.setProperty('--text-secondary', theme.text_secondary);
    } catch (e) {
        console.error('Theme fetch error:', e);
    }
}

async function fetchWeather() {
    try {
        const res = await fetch(`${API_BASE}/api/weather`);
        const data = await res.json();
        if (data.error) return;

        document.getElementById('temperature').textContent = `${data.temperature}°C`;
        document.getElementById('weather-icon').textContent = WEATHER_ICONS[data.icon] || '🌡️';
        document.getElementById('weather-desc').textContent = data.description || 'Unknown';
        document.getElementById('wind').textContent = `${data.windspeed} km/h`;
        document.getElementById('humidity').textContent = `${data.humidity}%`;
        document.getElementById('sunrise').textContent = data.sunrise;
        document.getElementById('sunset').textContent = data.sunset;
    } catch (e) {
        console.error('Weather fetch error:', e);
    }
}

async function fetchSensors() {
    try {
        const res = await fetch(`${API_BASE}/api/sensors`);
        const data = await res.json();
        renderSensors(data);
    } catch (e) {
        console.error('Sensors fetch error:', e);
    }
}

function renderSensors(sensorData) {
    const grid = document.getElementById('sensors-grid');
    grid.innerHTML = '';
    const now = Date.now() / 1000;

    for (const name of SENSOR_NAMES) {
        const sensor = sensorData[name];
        const card = document.createElement('div');
        card.className = 'sensor-card glass-card';

        let tempStr = '--.-°C';
        let humStr = '--%';
        let ageStr = '--';
        let humFace = '';
        let isStale = true;

        if (sensor) {
            const age = now - sensor.timestamp;
            isStale = age > 300;
            tempStr = `${sensor.temp.toFixed(1)}°C`;
            humStr = `${sensor.hum}%`;
            ageStr = age < 60 ? `${Math.floor(age)}s` : `${Math.floor(age / 60)}m`;
            if (!isStale) humFace = getHumidityFace(sensor.hum);
        }

        if (isStale) card.classList.add('stale');

        card.innerHTML = `
            <div class="sensor-header">
                <span class="sensor-name">${name}</span>
                <span class="sensor-face">${humFace}</span>
                <span class="sensor-age">${ageStr}</span>
            </div>
            <div class="sensor-values">
                <span class="sensor-temp">${tempStr}</span>
                <span class="sensor-hum">${humStr}</span>
            </div>
        `;
        grid.appendChild(card);
    }
}

// Initial load
updateTime();
fetchTheme();
fetchWeather();
fetchSensors();

// Live updates
setInterval(updateTime, 1000);
setInterval(fetchTheme, 60000); // Theme every minute
setInterval(fetchWeather, 300000); // Weather every 5 min
setInterval(fetchSensors, 5000); // Sensors every 5 sec
