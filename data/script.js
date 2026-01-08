// API endpoints
const API_BASE = '/api';
const API_STATE = `${API_BASE}/state`;
const API_POWER = `${API_BASE}/power`;
const API_SETPOINT = `${API_BASE}/setpoint`;

// UI элементы
const powerSwitch = document.getElementById('power-switch');
const powerLabel = document.getElementById('power-label');
const tempSlider = document.getElementById('temp-slider');
const tempInput = document.getElementById('temp-input');
const setpointValue = document.getElementById('setpoint-value');
const setTempBtn = document.getElementById('set-temp-btn');
const currentTempValue = document.getElementById('current-temp-value');
const statusText = document.getElementById('status-text');
const statusDot = document.getElementById('status-dot');

// Обновление состояния с сервера
async function updateState() {
    try {
        const response = await fetch(API_STATE);
        if (!response.ok) {
            throw new Error('Failed to fetch state');
        }
        const state = await response.json();
        
        // Обновление UI
        powerSwitch.checked = state.is_on;
        powerLabel.textContent = state.is_on ? 'Включен' : 'Выключен';
        
        tempSlider.value = state.setpoint_temp;
        tempInput.value = state.setpoint_temp.toFixed(1);
        setpointValue.textContent = state.setpoint_temp.toFixed(1);
        
        if (state.current_temp !== null && state.current_temp !== undefined) {
            currentTempValue.textContent = state.current_temp.toFixed(1);
        } else {
            currentTempValue.textContent = '--';
        }
        
        updateStatus(true);
    } catch (error) {
        console.error('Error updating state:', error);
        updateStatus(false);
    }
}

// Установка питания
async function setPower(isOn) {
    try {
        const response = await fetch(API_POWER, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ is_on: isOn })
        });
        
        if (!response.ok) {
            throw new Error('Failed to set power');
        }
        
        await updateState();
    } catch (error) {
        console.error('Error setting power:', error);
        updateStatus(false);
        // Откат состояния переключателя
        powerSwitch.checked = !isOn;
    }
}

// Установка температуры поддержания
async function setSetpoint(temp) {
    try {
        const response = await fetch(API_SETPOINT, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({ temperature: temp })
        });
        
        if (!response.ok) {
            throw new Error('Failed to set setpoint');
        }
        
        await updateState();
    } catch (error) {
        console.error('Error setting setpoint:', error);
        updateStatus(false);
    }
}

// Обновление статуса подключения
function updateStatus(isOnline) {
    if (isOnline) {
        statusText.textContent = 'Подключено';
        statusDot.className = 'status-dot online';
    } else {
        statusText.textContent = 'Офлайн';
        statusDot.className = 'status-dot offline';
    }
}

// Обработчики событий
powerSwitch.addEventListener('change', (e) => {
    setPower(e.target.checked);
});

tempSlider.addEventListener('input', (e) => {
    const value = parseFloat(e.target.value);
    setpointValue.textContent = value.toFixed(1);
    tempInput.value = value.toFixed(1);
});

tempSlider.addEventListener('change', (e) => {
    setSetpoint(parseFloat(e.target.value));
});

tempInput.addEventListener('input', (e) => {
    const value = parseFloat(e.target.value);
    if (!isNaN(value) && value >= 30 && value <= 100) {
        tempSlider.value = value;
        setpointValue.textContent = value.toFixed(1);
    }
});

setTempBtn.addEventListener('click', () => {
    const value = parseFloat(tempInput.value);
    if (!isNaN(value) && value >= 30 && value <= 100) {
        setSetpoint(value);
    } else {
        alert('Температура должна быть в диапазоне 30-100°C');
    }
});

// Периодическое обновление состояния
setInterval(updateState, 2000); // Обновление каждые 2 секунды

// Начальная загрузка
updateState();

