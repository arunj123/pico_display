const SERVICE_UUID = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
const WIFI_CHAR_UUID = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
const LOC_CHAR_UUID = "0000ff02-0000-1000-8000-00805f9b34fb";

let bluetoothDevice;
let wifiCharacteristic;
let locationCharacteristic;

let map;
let marker;

const ui = {
    connectBtn: document.getElementById('connectBtn'),
    saveBtn: document.getElementById('saveBtn'),
    exitBtn: document.getElementById('exitBtn'),
    status: document.getElementById('status-indicator'),
    form: document.getElementById('form-ui'),

    // Sections
    wifiSection: document.getElementById('wifi-section'),
    wifiToggle: document.getElementById('wifi-toggle'),

    // Inputs
    ssid: document.getElementById('ssid'),
    pass: document.getElementById('password'),
    locName: document.getElementById('loc_name'),
    locLat: document.getElementById('loc_lat'),
    locLon: document.getElementById('loc_lon'),

    geoBtn: document.getElementById('geoBtn'),
    log: document.getElementById('log-console')
};

// --- MAP LOGIC ---
function initMap(lat = 49.45, lon = 11.08) {
    if (map) {
        // Just fly to new default if provided
        map.flyTo([lat, lon], 13);
        marker.setLatLng([lat, lon]);
        return;
    }

    if (typeof L === 'undefined') {
        log("Leaflet JS not loaded. Map disabled.", true);
        return;
    }

    map = L.map('map').setView([lat, lon], 13);

    L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
        attribution: '&copy; <a href="https://openstreetmap.org">OpenStreetMap</a>'
    }).addTo(map);

    marker = L.marker([lat, lon], { draggable: true }).addTo(map);

    marker.on('dragend', function (e) { updateInputs(marker.getLatLng()); });
    map.on('click', function (e) {
        marker.setLatLng(e.latlng);
        updateInputs(e.latlng);
    });

    setTimeout(() => { map.invalidateSize(); }, 200);
}

function updateInputs(latlng) {
    ui.locLat.value = latlng.lat.toFixed(6);
    ui.locLon.value = latlng.lng.toFixed(6);
}

ui.geoBtn.addEventListener('click', () => {
    if (!navigator.geolocation) return log("Geolocation disabled", true);
    ui.geoBtn.disabled = true;
    ui.geoBtn.textContent = "Locating...";

    navigator.geolocation.getCurrentPosition(
        (pos) => {
            const { latitude: lat, longitude: lng } = pos.coords;
            log(`Browser Location: ${lat}, ${lng}`);
            if (map) {
                const newLL = new L.LatLng(lat, lng);
                map.flyTo(newLL, 13);
                marker.setLatLng(newLL);
                updateInputs(newLL);
            } else {
                ui.locLat.value = lat.toFixed(6);
                ui.locLon.value = lng.toFixed(6);
            }
            ui.geoBtn.disabled = false;
            ui.geoBtn.innerHTML = '<span style="font-size:16px">📍</span> Use My Current Location';
        },
        (err) => {
            log(`Geo Error: ${err.message}`, true);
            ui.geoBtn.disabled = false;
            ui.geoBtn.textContent = "Retry Location";
        }
    );
});

// --- UI TOGGLES ---
ui.wifiToggle.addEventListener('click', () => {
    const isHidden = ui.wifiSection.classList.contains('hidden');
    if (isHidden) {
        ui.wifiSection.classList.remove('hidden');
        ui.wifiToggle.textContent = 'Hide Wi-Fi';
    } else {
        ui.wifiSection.classList.add('hidden');
        ui.wifiToggle.textContent = 'Configure Wi-Fi';
    }
});

// --- BLUETOOTH LOGIC ---

async function sendCommand(char, str) {
    try {
        const encoder = new TextEncoder();
        const data = encoder.encode(str);
        const CHUNK_SIZE = 20; // Safe LE MTU

        if (data.length <= CHUNK_SIZE) {
            await char.writeValueWithResponse(data);
        } else {
            console.log(`[BLE] Chunking ${data.length} bytes...`);
            for (let i = 0; i < data.length; i += CHUNK_SIZE) {
                const chunk = data.slice(i, i + CHUNK_SIZE);
                await char.writeValueWithResponse(chunk);
                await new Promise(r => setTimeout(r, 50)); // Delay for stability
            }
        }
        return true;
    } catch (e) {
        setStatus('Write Failed: ' + e.message, 'error');
        return false;
    }
}

async function readValue(char) {
    try {
        const value = await char.readValue();
        const dec = new TextDecoder("utf-8");
        return dec.decode(value);
    } catch (e) {
        log(`Read Error: ${e.message}`, true);
        return null; // Return null on read error (e.g. auth failed)
    }
}

ui.connectBtn.addEventListener('click', async () => {
    try {
        setStatus('Scanning...');
        bluetoothDevice = await navigator.bluetooth.requestDevice({
            filters: [{ namePrefix: "Pico" }, { namePrefix: "Gateway" }, { namePrefix: "PiZero" }],
            optionalServices: [SERVICE_UUID]
        });

        bluetoothDevice.addEventListener('gattserverdisconnected', onDisconnected);
        setStatus('Connecting...');
        const server = await bluetoothDevice.gatt.connect();

        setStatus('Discovering Services...');
        const service = await server.getPrimaryService(SERVICE_UUID);

        wifiCharacteristic = await service.getCharacteristic(WIFI_CHAR_UUID);

        try {
            locationCharacteristic = await service.getCharacteristic(LOC_CHAR_UUID);
            // READ CURRENT VALUES
            log("Reading stored configuration...");
            const jsonStr = await readValue(locationCharacteristic);
            if (jsonStr) {
                log(`Read Config: ${jsonStr}`);
                try {
                    const data = JSON.parse(jsonStr);
                    if (data.name) ui.locName.value = data.name;
                    if (data.lat && data.lon) {
                        ui.locLat.value = data.lat;
                        ui.locLon.value = data.lon;
                        // Init map with saved coords
                        initMap(parseFloat(data.lat), parseFloat(data.lon));
                    } else {
                        initMap(); // Default
                    }
                } catch (e) {
                    log("Invalid JSON read, using defaults");
                    initMap();
                }
            } else {
                initMap();
            }
        } catch (e) {
            log(`Loc/Config Char not found: ${e.message}`);
            initMap();
        }

        setStatus('Connected', 'success');
        ui.connectBtn.parentElement.classList.add('hidden');
        ui.form.classList.remove('hidden');

    } catch (error) {
        console.error(error);
        setStatus(error.name === 'NotFoundError' ? 'Cancelled' : 'Connection Failed', 'error');
    }
});

ui.saveBtn.addEventListener('click', async () => {
    const ssid = ui.ssid.value.trim();
    const password = ui.pass.value.trim();
    const locName = ui.locName.value.trim();
    const locLat = ui.locLat.value.trim();
    const locLon = ui.locLon.value.trim();

    const hasWifi = (!ui.wifiSection.classList.contains('hidden') && ssid.length > 0);
    const hasLocation = (locLat.length > 0 && locLon.length > 0);

    if (!hasWifi && !hasLocation) {
        return setStatus('Nothing to save. Enter Wi-Fi or Select Location.', 'error');
    }

    if (locName.length > 0 && !hasLocation) {
        return setStatus('Location valid coordinates required.', 'error');
    }

    ui.saveBtn.disabled = true;
    ui.saveBtn.innerHTML = '<div class="spinner"></div> Saving...';

    try {
        if (hasWifi) {
            log(`Saving Wi-Fi: ${ssid}`);
            if (!await sendCommand(wifiCharacteristic, "S:" + ssid)) throw new Error("SSID Write Failed");
            await new Promise(r => setTimeout(r, 50));
            if (password) {
                if (!await sendCommand(wifiCharacteristic, "P:" + password)) throw new Error("Pass Write Failed");
                await new Promise(r => setTimeout(r, 50));
            }
        }

        if (hasLocation) {
            log(`Saving Location: ${locLat}, ${locLon}`);
            const locData = JSON.stringify({
                name: locName || "Device",
                lat: parseFloat(locLat),
                lon: parseFloat(locLon)
            });
            if (!await sendCommand(locationCharacteristic, locData)) throw new Error("Location Write Failed");
            await new Promise(r => setTimeout(r, 50));
        }

        if (!await sendCommand(wifiCharacteristic, "SAVE")) throw new Error("Save Command Failed");

        setStatus('Saved! Device Rebooting...', 'success');
        setTimeout(() => disconnect(), 2000);

    } catch (error) {
        log(`SAVE ERROR: ${error.message}`, true);
        ui.saveBtn.disabled = false;
        ui.saveBtn.textContent = 'Save & Reboot';
        setStatus(error.message, 'error');
    }
});

ui.exitBtn.addEventListener('click', async () => {
    if (await sendCommand(wifiCharacteristic, "EXIT")) {
        setStatus('Exiting...', 'success');
        setTimeout(() => disconnect(), 1000);
    }
});

function disconnect() {
    if (bluetoothDevice && bluetoothDevice.gatt.connected) {
        bluetoothDevice.gatt.disconnect();
    }
}

function onDisconnected() {
    setStatus('Disconnected');
    ui.form.classList.add('hidden');
    ui.connectBtn.parentElement.classList.remove('hidden');
    ui.saveBtn.disabled = false;
    ui.saveBtn.textContent = 'Save & Reboot';
    // Clear inputs? Maybe keep them for retry.
}

function setStatus(msg, type = 'normal') {
    log(`Status: ${msg}`);
    ui.status.textContent = msg;
    ui.status.className = 'status-badge ' + type;
}

function log(msg, isError = false) {
    const div = document.createElement('div');
    div.className = 'log-entry';
    if (isError) div.style.color = 'red';
    div.innerHTML = `<span class="log-time">[${new Date().toLocaleTimeString()}]</span> ${msg}`;
    ui.log.appendChild(div);
    ui.log.scrollTop = ui.log.scrollHeight;
    console.log(`[BLE] ${msg}`);
}
