const API_BASE = '/api';
const API_STATE = `${API_BASE}/state`;
const API_POWER = `${API_BASE}/power`;
const API_SETPOINT = `${API_BASE}/setpoint`;

const powerSwitch = document.getElementById('power-switch');
const powerLabel = document.getElementById('power-label');
const relayStatusText = document.getElementById('relay-status-text');
const relayDot = document.getElementById('relay-dot');
const tempSlider = document.getElementById('temp-slider');
const setpointValue = document.getElementById('setpoint-value');
const currentTempValue = document.getElementById('current-temp-value');
const statusText = document.getElementById('status-text');
const statusDot = document.getElementById('status-dot');

async function updateState() {
    try {
        const response = await fetch(API_STATE);
        if (!response.ok) {
            throw new Error('Failed to fetch state');
        }
        const state = await response.json();
        
        powerSwitch.checked = state.is_on;
        powerLabel.textContent = state.is_on ? 'Включен' : 'Выключен';
        
        if (state.relay_state !== null && state.relay_state !== undefined) {
            const relayOn = state.relay_state === true;
            relayStatusText.textContent = relayOn ? 'Включено' : 'Выключено';
            relayDot.className = 'relay-dot ' + (relayOn ? 'on' : 'off');
        } else {
            relayStatusText.textContent = '--';
            relayDot.className = 'relay-dot';
        }
        
        tempSlider.value = state.setpoint_temp;
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
        powerSwitch.checked = !isOn;
    }
}

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

function updateStatus(isOnline) {
    if (isOnline) {
        statusText.textContent = 'Подключено';
        statusDot.className = 'status-dot online';
    } else {
        statusText.textContent = 'Офлайн';
        statusDot.className = 'status-dot offline';
    }
}

powerSwitch.addEventListener('change', (e) => {
    setPower(e.target.checked);
});

let setpointTimeout = null;
tempSlider.addEventListener('input', (e) => {
    const value = parseFloat(e.target.value);
    setpointValue.textContent = value.toFixed(1);
    
    if (setpointTimeout) {
        clearTimeout(setpointTimeout);
    }
    
    setpointTimeout = setTimeout(() => {
        setSetpoint(value);
    }, 300);
});

updateState();
setInterval(updateState, 2000);

