/**
 * Smart Lamp Controller - Dashboard & Voice Wake-Up Engine
 * 100% English Voice Recognition (Native Web Speech & Web Audio API)
 */

const STORAGE_KEY = 'lamp_dashboard_api_key';
const FALLBACK_STORAGE_KEY = 'poc5_dashboard_api_key';
const apiKey = localStorage.getItem(STORAGE_KEY) || localStorage.getItem(FALLBACK_STORAGE_KEY);

// Auth Guard
if (!apiKey) {
  window.location.href = '/login';
}

const deviceGrid = document.getElementById('deviceGrid');
const refreshBtn = document.getElementById('refreshBtn');
const logoutBtn = document.getElementById('logoutBtn');
const voiceToggleBtn = document.getElementById('voiceToggleBtn');
const voicePowerBtn = document.getElementById('voicePowerBtn');
const voiceLiveBadge = document.getElementById('voiceLiveBadge');
const voiceLiveDot = document.getElementById('voiceLiveDot');
const voiceLiveText = document.getElementById('voiceLiveText');
const voiceWidget = document.getElementById('voiceWidget');
const voiceWidgetHeader = document.getElementById('voiceWidgetHeader');
const voiceEmojiIcon = document.getElementById('voiceEmojiIcon');
const voiceStateTitle = document.getElementById('voiceStateTitle');
const voiceCountdownBadge = document.getElementById('voiceCountdownBadge');
const voiceHintText = document.getElementById('voiceHintText');
const voiceWaves = document.getElementById('voiceWaves');
const voiceStreamLine = document.getElementById('voiceStreamLine');
const voiceToast = document.getElementById('voiceToast');

let isOperating = false;
let currentDevices = [];

// Logout
if (logoutBtn) {
  logoutBtn.addEventListener('click', () => {
    localStorage.removeItem(STORAGE_KEY);
    localStorage.removeItem(FALLBACK_STORAGE_KEY);
    window.location.href = '/login';
  });
}

if (refreshBtn) {
  refreshBtn.addEventListener('click', () => {
    fetchDevices();
  });
}

function headers() {
  return {
    'Content-Type': 'application/json',
    'X-API-Key': apiKey,
  };
}

function showToast(msg) {
  if (!voiceToast) return;
  voiceToast.textContent = msg;
  voiceToast.classList.add('show');
  setTimeout(() => {
    voiceToast.classList.remove('show');
  }, 3000);
}

// ===== Web Audio API Synthesizer =====
let audioCtx = null;

function getAudioContext() {
  if (!audioCtx) {
    const AudioContextClass = window.AudioContext || window.webkitAudioContext;
    if (AudioContextClass) {
      audioCtx = new AudioContextClass();
    }
  }
  if (audioCtx && audioCtx.state === 'suspended') {
    audioCtx.resume();
  }
  return audioCtx;
}

function playTone(freq, type, duration, startTime, gainLevel = 0.15) {
  const ctx = getAudioContext();
  if (!ctx) return;
  try {
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.type = type;
    osc.frequency.setValueAtTime(freq, startTime);
    gain.gain.setValueAtTime(gainLevel, startTime);
    gain.gain.exponentialRampToValueAtTime(0.001, startTime + duration);
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.start(startTime);
    osc.stop(startTime + duration);
  } catch (e) {
    console.warn('Audio tone error:', e);
  }
}

function playWakeChime() {
  const ctx = getAudioContext();
  if (!ctx) return;
  const now = ctx.currentTime;
  playTone(523.25, 'sine', 0.15, now, 0.18);        // C5
  playTone(659.25, 'sine', 0.15, now + 0.1, 0.18);  // E5
  playTone(783.99, 'sine', 0.25, now + 0.2, 0.2);   // G5
}

function playSuccessChime() {
  const ctx = getAudioContext();
  if (!ctx) return;
  const now = ctx.currentTime;
  playTone(783.99, 'triangle', 0.12, now, 0.18);     // G5
  playTone(1046.50, 'triangle', 0.25, now + 0.1, 0.2); // C6
}

function playTimeoutChime() {
  const ctx = getAudioContext();
  if (!ctx) return;
  const now = ctx.currentTime;
  playTone(440.00, 'sine', 0.18, now, 0.12);        // A4
  playTone(349.23, 'sine', 0.25, now + 0.12, 0.12); // F4
}

// ===== REST Device API =====
async function fetchDevices() {
  try {
    const res = await fetch('/api/devices', { headers: headers() });
    if (res.status === 401) {
      localStorage.removeItem(STORAGE_KEY);
      localStorage.removeItem(FALLBACK_STORAGE_KEY);
      window.location.href = '/login';
      return;
    }
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const devices = await res.json();
    currentDevices = devices;
    renderDevices(devices);
  } catch (err) {
    console.error('Fetch devices error:', err);
  }
}

function renderDevices(devices) {
  if (isOperating) return;

  if (!devices || devices.length === 0) {
    deviceGrid.innerHTML = `
      <div class="empty-card">
        No ESP32 devices found on server. Ensure firmware is configured and online.
      </div>
    `;
    return;
  }

  deviceGrid.innerHTML = devices.map(device => {
    const isOnline = device.online === true;
    const isLampOn = device.on === true;

    return `
      <div class="device-card" id="card-${device.device_id}">
        <div class="device-card-header">
          <div class="device-meta">
            <div class="chip-avatar">
              <svg viewBox="0 0 24 24">
                <rect x="4" y="4" width="16" height="16" rx="2"></rect>
                <rect x="9" y="9" width="6" height="6"></rect>
                <line x1="9" y1="1" x2="9" y2="4"></line>
                <line x1="15" y1="1" x2="15" y2="4"></line>
                <line x1="9" y1="20" x2="9" y2="23"></line>
                <line x1="15" y1="20" x2="15" y2="23"></line>
                <line x1="20" y1="9" x2="23" y2="9"></line>
                <line x1="20" y1="14" x2="23" y2="14"></line>
                <line x1="1" y1="9" x2="4" y2="9"></line>
                <line x1="1" y1="14" x2="4" y2="14"></line>
              </svg>
            </div>
            <div>
              <div class="device-name">${device.device_id}</div>
              <div class="device-type">ESP32 DevKit V1 &bull; 30-Pin</div>
            </div>
          </div>

          <div class="status-badge ${isOnline ? 'online' : 'offline'}">
            <span class="badge-dot"></span>
            <span>${isOnline ? 'ONLINE' : 'OFFLINE'}</span>
          </div>
        </div>

        <!-- Smart Lamp Control Box -->
        <div class="lamp-control-box ${isLampOn ? 'active' : ''}">
          <div class="lamp-visual-group">
            <div class="lamp-bulb-icon ${isLampOn ? 'lit' : ''}">
              <svg viewBox="0 0 24 24">
                <path d="M9 18h6M10 22h4M12 2a7 7 0 0 0-7 7c0 2.38 1.19 4.47 3 5.74V17a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1v-2.26c1.81-1.27 3-3.36 3-5.74a7 7 0 0 0-7-7z"/>
              </svg>
            </div>
            <div class="lamp-details">
              <div class="lamp-title">
                Smart Lamp
                <span class="lamp-pin-tag">GPIO 26 (Relay)</span>
              </div>
              <div class="lamp-status-text ${isLampOn ? 'on' : 'off'}">
                State: ${isLampOn ? '● LIGHT ON' : '○ LIGHT OFF'}
              </div>
            </div>
          </div>

          <label class="switch">
            <input type="checkbox" ${isLampOn ? 'checked' : ''} onchange="toggleDevice('${device.device_id}', ${isLampOn})" />
            <span class="slider"></span>
          </label>
        </div>

        <!-- Voice Hints -->
        <div class="voice-hint-box">
          <div class="voice-hint-title">
            <span>🗣️ Voice Commands</span>
          </div>
          <div>Wake: <span class="cmd-tag">"Wake Up"</span> &bull; Toggle: <span class="cmd-tag">"Change status"</span> &bull; Direct: <span class="cmd-tag">"Turn on"</span> / <span class="cmd-tag">"Turn off"</span></div>
        </div>
      </div>
    `;
  }).join('');
}

window.toggleDevice = async function(deviceId, currentOn) {
  isOperating = true;
  try {
    const res = await fetch(`/api/devices/${encodeURIComponent(deviceId)}/state`, {
      method: 'PUT',
      headers: headers(),
      body: JSON.stringify({ on: !currentOn }),
    });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    await fetchDevices();
  } catch (err) {
    console.error('Toggle error:', err);
    showToast('Failed to switch lamp state');
  } finally {
    isOperating = false;
  }
};

async function executeVoiceCommand(desiredOn) {
  if (currentDevices.length === 0) {
    showToast('No devices available to control');
    return false;
  }
  const targetDevice = currentDevices[0];
  isOperating = true;
  try {
    const res = await fetch(`/api/devices/${encodeURIComponent(targetDevice.device_id)}/state`, {
      method: 'PUT',
      headers: headers(),
      body: JSON.stringify({ on: desiredOn }),
    });
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    showToast(`Command executed: Lamp turned ${desiredOn ? 'ON' : 'OFF'}`);
    await fetchDevices();
    return true;
  } catch (err) {
    console.error('Voice execution error:', err);
    showToast('Error executing voice command');
    return false;
  } finally {
    isOperating = false;
  }
}

async function toggleVoiceCommand() {
  if (currentDevices.length === 0) {
    showToast('No devices available');
    return false;
  }
  const targetDevice = currentDevices[0];
  const desiredOn = !targetDevice.on;
  return await executeVoiceCommand(desiredOn);
}

// ===== Web Speech API Voice Engine (English Only) =====
const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;

const VoiceState = {
  INACTIVE: 'INACTIVE',
  SLEEPING: 'SLEEPING',
  AWAKE: 'AWAKE',
  EXECUTING: 'EXECUTING',
};

let currentVoiceState = VoiceState.INACTIVE;
let recognition = null;
let countdownTimer = null;
let countdownRemaining = 8;

function updateVoiceUI(state) {
  currentVoiceState = state;
  if (!voiceWidget) return;

  voiceWidget.className = `voice-widget ${state.toLowerCase()}`;

  if (state === VoiceState.INACTIVE) {
    voiceEmojiIcon.textContent = '🔇';
    voiceStateTitle.textContent = 'Voice Assistant';
    voiceHintText.textContent = 'Click to start Always-Listening';
    voiceCountdownBadge.style.display = 'none';
    voiceWaves.style.display = 'none';
    voiceStreamLine.textContent = '● Microphone inactive';
    voiceLiveBadge.className = 'voice-live-badge inactive';
    voiceLiveText.textContent = 'Microphone: Disabled';
  } else if (state === VoiceState.SLEEPING) {
    voiceEmojiIcon.textContent = '💤';
    voiceStateTitle.textContent = 'Always-Listening';
    voiceHintText.textContent = 'Say "Wake Up" to activate';
    voiceCountdownBadge.style.display = 'none';
    voiceWaves.style.display = 'none';
    voiceStreamLine.textContent = '● Listening for "Wake Up"...';
    voiceLiveBadge.className = 'voice-live-badge sleeping';
    voiceLiveText.textContent = 'Microphone: Standby ("Wake Up")';
  } else if (state === VoiceState.AWAKE) {
    voiceEmojiIcon.textContent = '⚡';
    voiceStateTitle.textContent = 'Listening for command...';
    voiceHintText.textContent = 'Say "Change status", "Turn on", "Turn off"';
    voiceCountdownBadge.style.display = 'inline-block';
    voiceCountdownBadge.textContent = `${countdownRemaining}s`;
    voiceWaves.style.display = 'flex';
    voiceStreamLine.textContent = '● Awaiting command...';
    voiceLiveBadge.className = 'voice-live-badge awake';
    voiceLiveText.textContent = `Awake (${countdownRemaining}s left)`;
  } else if (state === VoiceState.EXECUTING) {
    voiceEmojiIcon.textContent = '⚙️';
    voiceStateTitle.textContent = 'Executing command...';
    voiceHintText.textContent = 'Applying state to ESP32...';
    voiceCountdownBadge.style.display = 'none';
    voiceWaves.style.display = 'none';
    voiceStreamLine.textContent = '● Sending command to device...';
  }
}

function startCountdown() {
  stopCountdown();
  countdownRemaining = 8;
  if (voiceCountdownBadge) {
    voiceCountdownBadge.textContent = '8s';
  }
  countdownTimer = setInterval(() => {
    countdownRemaining--;
    if (voiceCountdownBadge) {
      voiceCountdownBadge.textContent = `${countdownRemaining}s`;
    }
    if (voiceLiveBadge && currentVoiceState === VoiceState.AWAKE) {
      voiceLiveText.textContent = `Awake (${countdownRemaining}s left)`;
    }
    if (countdownRemaining <= 0) {
      stopCountdown();
      playTimeoutChime();
      showToast('Command timeout (8s)');
      updateVoiceUI(VoiceState.SLEEPING);
    }
  }, 1000);
}

function stopCountdown() {
  if (countdownTimer) {
    clearInterval(countdownTimer);
    countdownTimer = null;
  }
}

function initSpeechRecognition() {
  if (!SpeechRecognition) {
    console.warn('Web Speech API not supported in this browser.');
    if (voiceStreamLine) {
      voiceStreamLine.textContent = 'Web Speech API not supported in this browser';
    }
    return null;
  }

  const rec = new SpeechRecognition();
  rec.continuous = true;
  rec.interimResults = true;
  rec.lang = 'en-US';

  rec.onstart = () => {
    updateVoiceUI(VoiceState.SLEEPING);
  };

  rec.onerror = (event) => {
    console.warn('Speech recognition error:', event.error);
    if (event.error === 'not-allowed') {
      showToast('Microphone access denied');
      stopVoice();
    }
  };

  rec.onend = () => {
    // If still in active mode, restart continuous listening
    if (currentVoiceState !== VoiceState.INACTIVE) {
      try {
        rec.start();
      } catch (e) {
        // already started or transitioning
      }
    }
  };

  rec.onresult = async (event) => {
    let transcript = '';
    for (let i = event.resultIndex; i < event.results.length; i++) {
      transcript += event.results[i][0].transcript;
    }
    transcript = transcript.trim().toLowerCase();
    if (!transcript) return;

    if (voiceStreamLine) {
      voiceStreamLine.textContent = `● Heard: "${transcript}"`;
    }

    if (currentVoiceState === VoiceState.SLEEPING) {
      // Look for Wake Word: "wake up"
      if (transcript.includes('wake up')) {
        playWakeChime();
        showToast('Assistant Awake! Listening for command...');
        updateVoiceUI(VoiceState.AWAKE);
        startCountdown();
      }
    } else if (currentVoiceState === VoiceState.AWAKE) {
      // Look for Commands
      if (transcript.includes('change status') || transcript.includes('toggle')) {
        stopCountdown();
        updateVoiceUI(VoiceState.EXECUTING);
        const ok = await toggleVoiceCommand();
        if (ok) playSuccessChime();
        updateVoiceUI(VoiceState.SLEEPING);
      } else if (transcript.includes('turn on') || transcript.includes('light on') || transcript.includes('lamp on')) {
        stopCountdown();
        updateVoiceUI(VoiceState.EXECUTING);
        const ok = await executeVoiceCommand(true);
        if (ok) playSuccessChime();
        updateVoiceUI(VoiceState.SLEEPING);
      } else if (transcript.includes('turn off') || transcript.includes('light off') || transcript.includes('lamp off')) {
        stopCountdown();
        updateVoiceUI(VoiceState.EXECUTING);
        const ok = await executeVoiceCommand(false);
        if (ok) playSuccessChime();
        updateVoiceUI(VoiceState.SLEEPING);
      }
    }
  };

  return rec;
}

function startVoice() {
  getAudioContext();
  if (!recognition) {
    recognition = initSpeechRecognition();
  }
  if (recognition) {
    try {
      recognition.start();
    } catch (e) {
      console.warn('Recognition start exception:', e);
    }
  }
}

function stopVoice() {
  stopCountdown();
  if (recognition) {
    try {
      recognition.stop();
    } catch (e) {}
  }
  updateVoiceUI(VoiceState.INACTIVE);
}

function toggleVoice() {
  if (currentVoiceState === VoiceState.INACTIVE) {
    startVoice();
  } else {
    stopVoice();
  }
}

if (voiceToggleBtn) {
  voiceToggleBtn.addEventListener('click', toggleVoice);
}
if (voicePowerBtn) {
  voicePowerBtn.addEventListener('click', toggleVoice);
}
if (voiceLiveBadge) {
  voiceLiveBadge.addEventListener('click', toggleVoice);
}

// Initial fetch & background polling
fetchDevices();
setInterval(fetchDevices, 3000);
