/**
 * ESP32 Cloud Controller - Dashboard Client Logic & Voice Wake-Up Engine
 */

const STORAGE_KEY = 'poc5_dashboard_api_key';
const apiKey = localStorage.getItem(STORAGE_KEY);

// Auth Guard: Redirect if not logged in
if (!apiKey) {
  window.location.href = '/login';
}

const deviceGrid = document.getElementById('deviceGrid');
const refreshBtn = document.getElementById('refreshBtn');
const logoutBtn = document.getElementById('logoutBtn');

let isOperating = false;

// Logout Action
if (logoutBtn) {
  logoutBtn.addEventListener('click', () => {
    localStorage.removeItem(STORAGE_KEY);
    window.location.href = '/login';
  });
}

function headers() {
  return {
    'Content-Type': 'application/json',
    'X-API-Key': apiKey,
  };
}

function formatTime(isoString) {
  if (!isoString) return 'Chưa có';
  try {
    const date = new Date(isoString);
    return date.toLocaleTimeString('vi-VN', { hour12: false }) + ' ' + date.toLocaleDateString('vi-VN');
  } catch {
    return isoString;
  }
}

async function fetchDevices() {
  try {
    const res = await fetch('/api/devices', { headers: headers() });
    if (res.status === 401) {
      localStorage.removeItem(STORAGE_KEY);
      window.location.href = '/login';
      return;
    }
    if (!res.ok) throw new Error(`HTTP ${res.status}`);
    const devices = await res.json();
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
        Chưa có thiết bị ESP32 nào được đăng ký trên hệ thống.
      </div>
    `;
    return;
  }

  deviceGrid.innerHTML = devices.map(device => {
    const isOnline = device.online === true;
    const isRelayOn = device.on === true;
    const hasPending = device.pending_on !== null && device.pending_on !== undefined;

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
              <div class="device-type">ESP32 DevKit V1</div>
            </div>
          </div>

          <div class="status-badge ${isOnline ? 'online' : 'offline'}">
            <span class="badge-dot"></span>
            <span>${isOnline ? 'ONLINE' : 'OFFLINE'}</span>
          </div>
        </div>

        <!-- Relay Controller -->
        <div class="relay-control-box">
          <div class="relay-info">
            <div class="relay-title">
              Real_Device
              <span class="relay-pin">GPIO 23</span>
            </div>
            <div class="relay-state-label">
              Trạng thái: 
              <span class="state-indicator-text ${isRelayOn ? 'on' : 'off'}">
                ${isRelayOn ? 'BẬT (ON)' : 'TẮT (OFF)'}
              </span>
            </div>
          </div>

          <label class="switch">
            <input 
              type="checkbox" 
              id="toggle-${device.device_id}" 
              ${isRelayOn ? 'checked' : ''} 
              onchange="handleToggle('${device.device_id}', this.checked)"
            >
            <span class="slider"></span>
          </label>
        </div>

        <!-- Queued Alert if offline command stored -->
        <div class="queued-alert ${hasPending ? 'show' : ''}">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" style="flex-shrink:0">
            <circle cx="12" cy="12" r="10"></circle>
            <line x1="12" y1="8" x2="12" y2="12"></line>
            <line x1="12" y1="16" x2="12.01" y2="16"></line>
          </svg>
          <span>Lệnh ${device.pending_on ? 'BẬT' : 'TẮT'} đang lưu trong hàng đợi (sẽ tự chạy khi online).</span>
        </div>

        <!-- Telemetry & Info -->
        <div class="telemetry-list">
          <div class="telemetry-item">
            <div class="telemetry-label">Hoạt động gần nhất</div>
            <div class="telemetry-value">${formatTime(device.last_seen)}</div>
          </div>
          <div class="telemetry-item">
            <div class="telemetry-label">Đồng bộ Cloud</div>
            <div class="telemetry-value" style="color: ${isOnline ? '#059669' : '#64748b'}">
              ${isOnline ? 'WSS Live' : 'Chờ kết nối'}
            </div>
          </div>
        </div>
      </div>
    `;
  }).join('');
}

async function handleToggle(deviceId, targetState) {
  isOperating = true;
  const toggleInput = document.getElementById(`toggle-${deviceId}`);
  if (toggleInput) toggleInput.disabled = true;

  try {
    const response = await fetch(`/api/devices/${encodeURIComponent(deviceId)}/state`, {
      method: 'PUT',
      headers: headers(),
      body: JSON.stringify({ on: targetState })
    });

    if (response.status === 401) {
      localStorage.removeItem(STORAGE_KEY);
      window.location.href = '/login';
      return;
    }

    const data = await response.json();
    if (!response.ok) {
      throw new Error(data.detail || `HTTP ${response.status}`);
    }
    await fetchDevices();
  } catch (err) {
    console.error('Failed to change state:', err);
    await fetchDevices();
    throw err;
  } finally {
    isOperating = false;
    if (toggleInput) toggleInput.disabled = false;
  }
}

if (refreshBtn) {
  refreshBtn.addEventListener('click', fetchDevices);
}

// Initial load
fetchDevices();

// Auto sync interval (3 seconds)
setInterval(fetchDevices, 3000);

// ==========================================
// VOICE WAKE-UP & COMMAND LISTENING ENGINE
// ==========================================
const SpeechRecognition = window.SpeechRecognition || window.webkitSpeechRecognition;
const AudioContextClass = window.AudioContext || window.webkitAudioContext;
const audioCtx = AudioContextClass ? new AudioContextClass() : null;

// Elements
const voiceToggleBtn = document.getElementById('voiceToggleBtn');
const voiceBtnLabel = document.getElementById('voiceBtnLabel');
const voiceWidget = document.getElementById('voiceWidget');
const voiceStateTitle = document.getElementById('voiceStateTitle');
const voiceHintText = document.getElementById('voiceHintText');
const voiceCountdownBadge = document.getElementById('voiceCountdownBadge');
const voiceWaves = document.getElementById('voiceWaves');
const voiceToast = document.getElementById('voiceToast');
const voicePowerBtn = document.getElementById('voicePowerBtn');
const voiceTranscriptPreview = document.getElementById('voiceTranscriptPreview');
const transcriptText = document.getElementById('transcriptText');
const voiceLiveBadge = document.getElementById('voiceLiveBadge');
const voiceLiveText = document.getElementById('voiceLiveText');
const voiceStreamLine = document.getElementById('voiceStreamLine');
const voiceEmojiIcon = document.getElementById('voiceEmojiIcon');

// Rich Console Logger with Styling
const VoiceLogger = {
  active: (msg) => {
    console.log(`%c[Voice Engine] 🟢 ${msg}`, 'color: #ffffff; background: #059669; font-weight: bold; font-size: 12px; padding: 2px 7px; border-radius: 4px;');
  },
  info: (msg) => {
    console.log(`%c[Voice Engine] 🎙️ ${msg}`, 'color: #2563eb; font-weight: bold;');
  },
  sound: (msg) => {
    console.log(`%c[Voice Engine] 🔊 ${msg}`, 'color: #0891b2;');
  },
  speech: (msg) => {
    console.log(`%c[Voice Engine] 🗣️ ${msg}`, 'color: #7c3aed; font-weight: bold;');
  },
  wake: (msg) => {
    console.log(`%c[Voice Engine] ⚡ ${msg}`, 'color: #ffffff; background: #4f46e5; font-weight: bold; font-size: 13px; padding: 3px 8px; border-radius: 4px;');
  },
  command: (msg) => {
    console.log(`%c[Voice Engine] 🚀 ${msg}`, 'color: #ffffff; background: #16a34a; font-weight: bold; font-size: 13px; padding: 3px 8px; border-radius: 4px;');
  },
  magic: (msg) => {
    console.log(`%c[Voice Engine] ✨ ${msg}`, 'color: #ffffff; background: #9333ea; font-weight: bold; font-size: 13px; padding: 3px 8px; border-radius: 4px;');
  },
  loop: (msg) => {
    console.log(`%c[Voice Engine] 🔄 ${msg}`, 'color: #64748b; font-style: italic;');
  },
  warn: (msg) => {
    console.warn(`[Voice Engine] ⚠️ ${msg}`);
  },
  error: (msg) => {
    console.error(`[Voice Engine] ❌ ${msg}`);
  }
};

// ==========================================
// STRICT KEYWORD DICTIONARY (USER SPECIFIED)
// ==========================================
// Wake words: 'Siri', 'Luna', 'Jarvis'
const WAKE_WORDS = ['siri', 'luna', 'jarvis'];

// Bật đèn: 'light on', 'turn the light on', 'expecto patronum', 'lumos'
const LIGHT_ON_COMMANDS = ['turn the light on', 'light on', 'expecto patronum', 'lumos'];

// Tắt đèn: 'light off', 'turn the light off'
const LIGHT_OFF_COMMANDS = ['turn the light off', 'light off'];

// Đổi trạng thái: 'change status', 'change the status', 'toggle'
const TOGGLE_COMMANDS = ['change the status', 'change status', 'toggle'];

/**
 * Clean spoken transcript:
 * - Lowercase
 * - Remove punctuation marks
 * - Normalize multiple whitespaces
 */
function cleanTranscript(text) {
  if (!text) return '';
  return text
    .toLowerCase()
    .replace(/[.,!?;:()"'`~@#$%^&*\-_+=/\\]/g, ' ')
    .replace(/\s+/g, ' ')
    .trim();
}

/**
 * Exact keyword match with word boundaries.
 * Ensures whole-token/phrase matching (e.g. 'siri' doesn't match 'desirous').
 */
function containsExactKeyword(text, keyword) {
  if (!text || !keyword) return false;
  const escaped = keyword.replace(/[.*+?^${}()|[\]\\]/g, '\\$&').replace(/\s+/g, '\\s+');
  const pattern = new RegExp(`(^|\\b)${escaped}(\\b|$)`, 'i');
  return pattern.test(text);
}

/**
 * Find matched keyword from candidate list
 */
function matchKeyword(text, list) {
  for (const item of list) {
    if (containsExactKeyword(text, item)) {
      return item;
    }
  }
  return null;
}

// Voice State Machine: 'inactive' | 'sleeping' | 'awake' | 'executing' | 'error'
let voiceState = 'inactive';
let recognition = null;
let isRecognizing = false;
let awakeTimeoutId = null;
let countdownIntervalId = null;
let countdownSeconds = 8;
let toastTimeoutId = null;

const BASE_TITLE = 'Dashboard | ESP32 Cloud Controller';

function updateDocumentTitle() {
  if (voiceState === 'inactive') {
    document.title = BASE_TITLE;
  } else if (voiceState === 'sleeping') {
    document.title = `💤 [Sleep - Lắng nghe...] ${BASE_TITLE}`;
  } else if (voiceState === 'awake') {
    document.title = `🎤 [Awake ${countdownSeconds}s] ${BASE_TITLE}`;
  } else if (voiceState === 'executing') {
    document.title = `⚡ [Executing...] ${BASE_TITLE}`;
  } else if (voiceState === 'error') {
    document.title = `⚠️ [Mic Lỗi] ${BASE_TITLE}`;
  }
}

function setConsoleStream(text) {
  if (voiceStreamLine) {
    voiceStreamLine.textContent = text;
  }
}

// Audio Feedback (Synthesized pleasant chimes via Web Audio API)
function playAudioTone(type) {
  if (!audioCtx) return;
  try {
    if (audioCtx.state === 'suspended') {
      audioCtx.resume();
    }
    const now = audioCtx.currentTime;
    const osc = audioCtx.createOscillator();
    const gain = audioCtx.createGain();
    osc.connect(gain);
    gain.connect(audioCtx.destination);

    if (type === 'wake') {
      // Upward chime (440Hz -> 880Hz)
      osc.type = 'sine';
      osc.frequency.setValueAtTime(440, now);
      osc.frequency.exponentialRampToValueAtTime(880, now + 0.18);
      gain.gain.setValueAtTime(0.12, now);
      gain.gain.exponentialRampToValueAtTime(0.001, now + 0.25);
      osc.start(now);
      osc.stop(now + 0.25);
    } else if (type === 'command') {
      // Double success chime (700Hz -> 1050Hz)
      osc.type = 'sine';
      osc.frequency.setValueAtTime(700, now);
      osc.frequency.setValueAtTime(1050, now + 0.1);
      gain.gain.setValueAtTime(0.15, now);
      gain.gain.exponentialRampToValueAtTime(0.001, now + 0.28);
      osc.start(now);
      osc.stop(now + 0.28);
    } else if (type === 'sleep') {
      // Soft down tone (440Hz -> 260Hz)
      osc.type = 'sine';
      osc.frequency.setValueAtTime(440, now);
      osc.frequency.exponentialRampToValueAtTime(260, now + 0.22);
      gain.gain.setValueAtTime(0.08, now);
      gain.gain.exponentialRampToValueAtTime(0.001, now + 0.26);
      osc.start(now);
      osc.stop(now + 0.26);
    }
  } catch (e) {
    console.debug('Tone play suppressed:', e);
  }
}

function showVoiceToast(msg, duration = 3500) {
  if (!voiceToast) return;
  voiceToast.textContent = msg;
  voiceToast.classList.add('show');
  clearTimeout(toastTimeoutId);
  toastTimeoutId = setTimeout(() => {
    voiceToast.classList.remove('show');
  }, duration);
}

function setVoiceState(newState, customHint = null) {
  voiceState = newState;
  updateDocumentTitle();

  // Clear pending awake timers when leaving awake
  if (newState !== 'awake') {
    if (awakeTimeoutId) {
      clearTimeout(awakeTimeoutId);
      awakeTimeoutId = null;
    }
    if (countdownIntervalId) {
      clearInterval(countdownIntervalId);
      countdownIntervalId = null;
    }
    if (voiceCountdownBadge) voiceCountdownBadge.style.display = 'none';
  }

  if (voiceWidget) {
    voiceWidget.className = `voice-widget ${newState}`;
  }

  if (newState === 'inactive') {
    if (voiceEmojiIcon) voiceEmojiIcon.textContent = '🔇';
    if (voiceToggleBtn) {
      voiceToggleBtn.classList.remove('active');
      if (voiceBtnLabel) voiceBtnLabel.textContent = 'Voice Control';
    }
    if (voiceLiveBadge) {
      voiceLiveBadge.className = 'voice-live-badge inactive';
      if (voiceLiveText) voiceLiveText.textContent = 'Micro: Tắt (Bấm để bật)';
    }
    if (voiceStateTitle) voiceStateTitle.textContent = 'Voice Control';
    if (voiceHintText) voiceHintText.textContent = customHint || 'Bấm để bật mic nhận diện';
    if (voiceWaves) voiceWaves.style.display = 'none';
    if (voiceTranscriptPreview) voiceTranscriptPreview.style.display = 'none';
    setConsoleStream('● Micro chưa kích hoạt (Click để bật)');
  } else if (newState === 'sleeping') {
    if (voiceEmojiIcon) voiceEmojiIcon.textContent = '💤';
    if (voiceToggleBtn) {
      voiceToggleBtn.classList.add('active');
      if (voiceBtnLabel) voiceBtnLabel.textContent = '💤 Mic Live: Sleep';
    }
    if (voiceLiveBadge) {
      voiceLiveBadge.className = 'voice-live-badge sleeping';
      if (voiceLiveText) voiceLiveText.textContent = '💤 Mic Live: Chế độ Sleep (Đang lắng nghe...)';
    }
    if (voiceStateTitle) voiceStateTitle.textContent = 'Chế độ Sleep (Lắng nghe)';
    if (voiceHintText) voiceHintText.textContent = customHint || 'Nói "Siri", "Luna" hoặc "Jarvis"';
    if (voiceWaves) voiceWaves.style.display = 'flex';
    if (voiceTranscriptPreview) voiceTranscriptPreview.style.display = 'flex';
    setConsoleStream('🟢 [SLEEP] Đang thu âm... Chờ "Siri", "Luna", "Jarvis"');
  } else if (newState === 'awake') {
    if (voiceEmojiIcon) voiceEmojiIcon.textContent = '🎤';
    if (voiceToggleBtn) {
      voiceToggleBtn.classList.add('active');
      if (voiceBtnLabel) voiceBtnLabel.textContent = '🎤 Mic Live: Awake!';
    }
    if (voiceLiveBadge) {
      voiceLiveBadge.className = 'voice-live-badge awake';
      if (voiceLiveText) voiceLiveText.textContent = '🎤 Mic Live: Đang thức dậy (Chờ lệnh 8s)';
    }
    if (voiceStateTitle) voiceStateTitle.textContent = 'Đã thức dậy (Awake)';
    if (voiceHintText) voiceHintText.textContent = customHint || 'Nói "Light on", "Light off", "Toggle"';
    if (voiceWaves) voiceWaves.style.display = 'flex';
    if (voiceTranscriptPreview) voiceTranscriptPreview.style.display = 'flex';
    setConsoleStream('🎤 [AWAKE] Đang chờ chỉ thị... (8s)');
    
    // Start countdown and 8s timeout
    countdownSeconds = 8;
    if (voiceCountdownBadge) {
      voiceCountdownBadge.style.display = 'inline-block';
      voiceCountdownBadge.textContent = `${countdownSeconds}s`;
    }
    updateDocumentTitle();

    if (countdownIntervalId) clearInterval(countdownIntervalId);
    countdownIntervalId = setInterval(() => {
      countdownSeconds -= 1;
      if (countdownSeconds > 0) {
        if (voiceCountdownBadge) voiceCountdownBadge.textContent = `${countdownSeconds}s`;
        if (voiceLiveText && voiceState === 'awake') {
          voiceLiveText.textContent = `🎤 Mic Live: Chờ lệnh (${countdownSeconds}s)`;
        }
        updateDocumentTitle();
      } else {
        clearInterval(countdownIntervalId);
        countdownIntervalId = null;
      }
    }, 1000);

    // 8-second listening window timeout
    if (awakeTimeoutId) clearTimeout(awakeTimeoutId);
    awakeTimeoutId = setTimeout(() => {
      if (voiceState === 'awake') {
        playAudioTone('sleep');
        VoiceLogger.info('⏱️ Hết 8s chờ lệnh mà không nhận được lệnh hợp lệ. Tự động trở về chế độ Sleep.');
        showVoiceToast('⏱️ Hết 8s chờ lệnh, quay về chế độ Sleep.');
        setVoiceState('sleeping');
      }
    }, 8000);
  } else if (newState === 'executing') {
    if (voiceEmojiIcon) voiceEmojiIcon.textContent = '⚡';
    if (voiceLiveBadge) {
      voiceLiveBadge.className = 'voice-live-badge executing';
      if (voiceLiveText) voiceLiveText.textContent = '⚡ Đang thực thi lệnh...';
    }
    if (voiceStateTitle) voiceStateTitle.textContent = 'Đang thực thi lệnh!';
    if (voiceHintText) voiceHintText.textContent = customHint || 'Đang gọi API gửi lệnh đến ESP32...';
    if (voiceWaves) voiceWaves.style.display = 'none';
    if (voiceCountdownBadge) voiceCountdownBadge.style.display = 'none';
    setConsoleStream('⚡ [EXECUTING] Đang gọi API gửi lệnh đến ESP32...');
  } else if (newState === 'error') {
    if (voiceEmojiIcon) voiceEmojiIcon.textContent = '⚠️';
    if (voiceToggleBtn) voiceToggleBtn.classList.remove('active');
    if (voiceLiveBadge) {
      voiceLiveBadge.className = 'voice-live-badge error';
      if (voiceLiveText) voiceLiveText.textContent = '❌ Lỗi Microphone';
    }
    if (voiceStateTitle) voiceStateTitle.textContent = 'Không khả dụng';
    if (voiceHintText) voiceHintText.textContent = customHint || 'Lỗi nhận diện giọng nói';
    if (voiceWaves) voiceWaves.style.display = 'none';
    setConsoleStream('❌ [ERROR] Microphone lỗi hoặc bị từ chối');
  }
}

async function executeVoiceCommand(actionType, matchedKeyword = '') {
  // Find first available device toggle
  const toggles = document.querySelectorAll('input[type="checkbox"][id^="toggle-"]');
  if (!toggles || toggles.length === 0) {
    showVoiceToast('⚠️ Chưa có thiết bị nào trên dashboard!');
    setVoiceState('sleeping');
    return;
  }

  // Target first device
  const targetToggle = toggles[0];
  const deviceId = targetToggle.id.replace('toggle-', '');

  let targetState;
  let actionLabel = '';

  if (actionType === 'on') {
    targetState = true;
    actionLabel = (matchedKeyword === 'lumos' || matchedKeyword === 'expecto patronum')
      ? `✨ Phép thuật [${matchedKeyword}] -> BẬT đèn`
      : `BẬT ĐÈN [${matchedKeyword}]`;
  } else if (actionType === 'off') {
    targetState = false;
    actionLabel = `TẮT ĐÈN [${matchedKeyword}]`;
  } else {
    // 'toggle'
    targetState = !targetToggle.checked;
    actionLabel = `ĐỔI TRẠNG THÁI [${matchedKeyword}] -> ${targetState ? 'BẬT (ON)' : 'TẮT (OFF)'}`;
  }

  VoiceLogger.command(`Gửi lệnh: ${actionLabel} cho [${deviceId}]`);
  showVoiceToast(`⚡ ${actionLabel}...`);

  try {
    await handleToggle(deviceId, targetState);
    VoiceLogger.active(`Đã thiết lập đèn [${deviceId}] sang ${targetState ? 'BẬT (ON)' : 'TẮT (OFF)'} thành công!`);
    showVoiceToast(`✅ ${actionLabel} thành công!`);
  } catch (err) {
    VoiceLogger.error(`Gửi lệnh thất bại: ${err.message}`);
    showVoiceToast(`❌ Thất bại: ${err.message}`);
  } finally {
    setTimeout(() => {
      if (voiceState !== 'inactive') {
        setVoiceState('sleeping');
      }
    }, 1200);
  }
}

function initVoiceRecognition() {
  if (!SpeechRecognition) {
    VoiceLogger.error('Trình duyệt hiện tại KHÔNG hỗ trợ Web Speech API. Vui lòng mở bằng Chrome hoặc Edge.');
    setVoiceState('error', 'Trình duyệt không hỗ trợ Web Speech API. Hãy dùng Chrome/Edge.');
    showVoiceToast('Trình duyệt chưa hỗ trợ Web Speech API (khuyến nghị dùng Chrome hoặc Edge)');
    return false;
  }

  if (recognition) return true;

  try {
    recognition = new SpeechRecognition();
    recognition.continuous = true;
    recognition.interimResults = true;
    recognition.lang = 'en-US';
    recognition.maxAlternatives = 1;

    recognition.onstart = () => {
      isRecognizing = true;
      VoiceLogger.active('========================================================');
      VoiceLogger.active('MICROPHONE ĐÃ ACTIVE VÀ ĐANG HOẠT ĐỘNG!');
      VoiceLogger.info('Từ khóa: Wake [Siri, Luna, Jarvis] | Bật [Light on, Turn the light on, Lumos, Expecto patronum] | Tắt [Light off, Turn the light off] | Toggle [Change status, Change the status, Toggle]');
      VoiceLogger.active('========================================================');
      if (voiceState === 'inactive') {
        setVoiceState('sleeping');
      }
    };

    recognition.onaudiostart = () => {
      VoiceLogger.sound('Audio stream captured: Luồng âm thanh từ microphone đang truyền vào trình duyệt.');
      setConsoleStream('🟢 [MIC ACTIVE] Đang thu âm... Chờ "Siri", "Luna", "Jarvis"');
    };

    recognition.onsoundstart = () => {
      VoiceLogger.sound('Phát hiện tín hiệu âm thanh thu được từ microphone.');
    };

    recognition.onspeechstart = () => {
      VoiceLogger.speech('Phát hiện tiếng nói con người! Đang phân tích nhận diện...');
      setConsoleStream('🗣️ [PHÁT HIỆN GIỌNG NÓI] Đang phân tích...');
    };

    recognition.onspeechend = () => {
      VoiceLogger.loop('Kết thúc đoạn giọng nói. Đang chờ kết quả phiên âm...');
    };

    recognition.onresult = (event) => {
      let latestTranscript = '';
      for (let i = event.resultIndex; i < event.results.length; ++i) {
        latestTranscript += event.results[i][0].transcript;
      }

      const cleanText = cleanTranscript(latestTranscript);
      if (!cleanText) return;

      VoiceLogger.speech(`Nghe được: "${cleanText}"`);
      setConsoleStream(`🗣️ [ĐANG NGHE] "${cleanText}"`);

      if (transcriptText) {
        transcriptText.textContent = `"${latestTranscript.trim()}"`;
      }

      // 1. SLEEPING STATE: Check for Wake Words strictly ('siri', 'luna', 'jarvis')
      // Commands & spells ('lumos', 'expecto patronum') can ONLY be triggered after wake word activation!
      if (voiceState === 'sleeping') {
        const matchedWake = matchKeyword(cleanText, WAKE_WORDS);

        if (matchedWake) {
          playAudioTone('wake');
          VoiceLogger.wake(`PHÁT HIỆN TỪ KHÓA WAKE-UP ("${matchedWake}")! Thức dậy, chờ lệnh trong 8s...`);
          showVoiceToast(`🎤 [${matchedWake.toUpperCase()}] Đã thức dậy! Đang chờ lệnh trong 8s...`, 3000);
          setVoiceState('awake');

          // Chained check: If user said wake word + command in the same breath (e.g. "Siri lumos")
          const matchedOn = matchKeyword(cleanText, LIGHT_ON_COMMANDS);
          const matchedOff = matchKeyword(cleanText, LIGHT_OFF_COMMANDS);
          const matchedToggle = matchKeyword(cleanText, TOGGLE_COMMANDS);

          if (matchedOn) {
            setTimeout(() => {
              if (voiceState === 'awake') {
                playAudioTone('command');
                if (matchedOn === 'lumos' || matchedOn === 'expecto patronum') {
                  VoiceLogger.magic(`✨ THI TRIỂN PHÉP THUẬT: "${matchedOn}" -> BẬT ĐÈN!`);
                  setVoiceState('executing', `✨ Phép thuật [${matchedOn}] đang bật đèn...`);
                } else {
                  VoiceLogger.command(`PHÁT HIỆN LỆNH BẬT ĐÈN ("${matchedOn}")!`);
                  setVoiceState('executing', `Đang gửi lệnh BẬT đèn [${matchedOn}]...`);
                }
                executeVoiceCommand('on', matchedOn);
              }
            }, 300);
            return;
          }
          if (matchedOff) {
            setTimeout(() => {
              if (voiceState === 'awake') {
                playAudioTone('command');
                VoiceLogger.command(`PHÁT HIỆN LỆNH TẮT ĐÈN ("${matchedOff}")!`);
                setVoiceState('executing', `Đang gửi lệnh TẮT đèn [${matchedOff}]...`);
                executeVoiceCommand('off', matchedOff);
              }
            }, 300);
            return;
          }
          if (matchedToggle) {
            setTimeout(() => {
              if (voiceState === 'awake') {
                playAudioTone('command');
                VoiceLogger.command(`PHÁT HIỆN LỆNH ĐỔI TRẠNG THÁI ("${matchedToggle}")!`);
                setVoiceState('executing', `Đang gửi lệnh ĐỔI TRẠNG THÁI [${matchedToggle}]...`);
                executeVoiceCommand('toggle', matchedToggle);
              }
            }, 300);
            return;
          }

          return;
        }
      } 
      // 2. AWAKE STATE: Check for strict action commands
      else if (voiceState === 'awake') {
        // Check BẬT ĐÈN ('light on', 'turn the light on', 'expecto patronum', 'lumos')
        const matchedOn = matchKeyword(cleanText, LIGHT_ON_COMMANDS);
        if (matchedOn) {
          playAudioTone('command');
          if (matchedOn === 'lumos' || matchedOn === 'expecto patronum') {
            VoiceLogger.magic(`✨ THI TRIỂN PHÉP THUẬT: "${matchedOn}" -> BẬT ĐÈN!`);
            setVoiceState('executing', `✨ Phép thuật [${matchedOn}] đang bật đèn...`);
          } else {
            VoiceLogger.command(`PHÁT HIỆN LỆNH BẬT ĐÈN ("${matchedOn}")!`);
            setVoiceState('executing', `Đang gửi lệnh BẬT đèn [${matchedOn}]...`);
          }
          executeVoiceCommand('on', matchedOn);
          return;
        }

        // Check TẮT ĐÈN ('light off', 'turn the light off')
        const matchedOff = matchKeyword(cleanText, LIGHT_OFF_COMMANDS);
        if (matchedOff) {
          playAudioTone('command');
          VoiceLogger.command(`PHÁT HIỆN LỆNH TẮT ĐÈN ("${matchedOff}")!`);
          setVoiceState('executing', `Đang gửi lệnh TẮT đèn [${matchedOff}]...`);
          executeVoiceCommand('off', matchedOff);
          return;
        }

        // Check ĐỔI TRẠNG THÁI ('change status', 'change the status', 'toggle')
        const matchedToggle = matchKeyword(cleanText, TOGGLE_COMMANDS);
        if (matchedToggle) {
          playAudioTone('command');
          VoiceLogger.command(`PHÁT HIỆN LỆNH ĐỔI TRẠNG THÁI ("${matchedToggle}")!`);
          setVoiceState('executing', `Đang gửi lệnh ĐỔI TRẠNG THÁI [${matchedToggle}]...`);
          executeVoiceCommand('toggle', matchedToggle);
          return;
        }
      }
    };

    recognition.onerror = (event) => {
      VoiceLogger.warn(`SpeechRecognition event error: ${event.error}`);
      if (event.error === 'not-allowed' || event.error === 'service-not-allowed') {
        isRecognizing = false;
        VoiceLogger.error('Quyền truy cập Microphone bị từ chối.');
        setVoiceState('error', 'Chưa cấp quyền microphone.');
        showVoiceToast('⚠️ Bạn đã từ chối quyền microphone. Vui lòng cho phép quyền micro.');
      } else if (event.error === 'network') {
        VoiceLogger.warn('Không kết nối được dịch vụ nhận dạng tiếng nói (Google Speech network).');
        showVoiceToast('⚠️ Lỗi kết nối Web Speech API (Google Cloud).');
      }
    };

    recognition.onend = () => {
      isRecognizing = false;
      VoiceLogger.loop('Phiên nhận diện vừa hoàn tất một chu kỳ. Tự động kết nối lại (Keep-alive restart)...');
      // Auto-restart for continuous listening
      if (voiceState !== 'inactive' && voiceState !== 'error') {
        setTimeout(() => {
          if (voiceState !== 'inactive' && voiceState !== 'error' && !isRecognizing) {
            try {
              recognition.start();
              VoiceLogger.loop('Đã tự động khởi động lại chu kỳ nhận diện.');
            } catch (e) {
              console.debug('[Voice Engine] Restart notice:', e.message);
            }
          }
        }, 250);
      }
    };

    return true;
  } catch (err) {
    VoiceLogger.error(`Init recognition failed: ${err.message}`);
    setVoiceState('error', err.message);
    return false;
  }
}

function toggleVoiceControl() {
  if (audioCtx && audioCtx.state === 'suspended') {
    audioCtx.resume();
  }

  if (voiceState === 'inactive' || voiceState === 'error') {
    VoiceLogger.info('Bắt đầu khởi động Voice Control...');
    const ok = initVoiceRecognition();
    if (!ok) return;
    try {
      recognition.start();
      setVoiceState('sleeping');
      VoiceLogger.info('Yêu cầu bật Microphone đã gửi đi. Chờ cấp quyền từ trình duyệt...');
      showVoiceToast('🎙️ Voice Wake-Up đã BẬT. Nói "Siri", "Luna" hoặc "Jarvis" để gọi!');
    } catch (e) {
      VoiceLogger.warn(`Start call notice: ${e.message}`);
    }
  } else {
    setVoiceState('inactive');
    VoiceLogger.info('Người dùng đã bấm TẮT Voice Control.');
    showVoiceToast('🔇 Voice Control đã TẮT.');
    if (recognition) {
      try {
        recognition.stop();
      } catch (e) {}
    }
  }
}

// Attach click triggers
if (voiceToggleBtn) {
  voiceToggleBtn.addEventListener('click', toggleVoiceControl);
}
if (voiceLiveBadge) {
  voiceLiveBadge.addEventListener('click', toggleVoiceControl);
}
if (voicePowerBtn) {
  voicePowerBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    toggleVoiceControl();
  });
}
if (voiceWidget) {
  voiceWidget.addEventListener('click', () => {
    if (voiceState === 'inactive') {
      toggleVoiceControl();
    }
  });
}

// Initial console banner to guide user
console.log(
  '%c[Voice Engine] 🚀 Sẵn sàng! Wake words: "Siri", "Luna", "Jarvis" | Bật: "light on", "turn the light on", "lumos", "expecto patronum" | Tắt: "light off", "turn the light off" | Toggle: "change status", "change the status", "toggle"',
  'color: #3b82f6; font-size: 11px; font-weight: bold;'
);
