"use client";

import React from "react";
import type { AIVoiceState, VoiceLanguage, TTSVoiceKey } from "@/hooks/useAIVoice";
import type { ModelKey } from "@/lib/ai/ai-config";
import type { UIMessage } from "ai";

const MODEL_OPTIONS: { key: ModelKey; label: string; description: string }[] = [
  {
    key: "sglang-qwen38-27b",
    label: "Qwen 3.8 27B (SGLang)",
    description: "Powerful, best for complex commands",
  },
  {
    key: "ollama-qwen3-4b",
    label: "Qwen 3 4B (Ollama)",
    description: "Balanced speed and quality",
  },
  {
    key: "ollama-qwen3-1.7b",
    label: "Qwen 3 1.7B (Ollama)",
    description: "Fastest, for simple commands",
  },
];

const TTS_VOICE_OPTIONS: { key: TTSVoiceKey; label: string }[] = [
  { key: "auto", label: "✨ Auto (Hoài My / Aria)" },
  { key: "vi-VN-HoaiMyNeural", label: "🇻🇳 Hoài My (Nữ - Edge)" },
  { key: "vi-VN-NamMinhNeural", label: "🇻🇳 Nam Minh (Nam - Edge)" },
  { key: "en-US-AriaNeural", label: "🇺🇸 Aria (Nữ - Edge)" },
  { key: "en-US-GuyNeural", label: "🇺🇸 Guy (Nam - Edge)" },
  { key: "browser-native", label: "🌐 Browser Native" },
];

interface AIVoiceWidgetProps {
  voiceState: AIVoiceState;
  liveTranscript: string;
  countdown: number | null;
  isListening: boolean;
  supported: boolean;
  messages: UIMessage[];
  lastAssistantText: string;
  status: string;
  modelKey: ModelKey;
  ttsEnabled: boolean;
  ttsVoice: TTSVoiceKey;
  language: VoiceLanguage;
  onSetLanguage: (lang: VoiceLanguage) => void;
  onSetModelKey: (key: ModelKey) => void;
  onSetTtsEnabled: (enabled: boolean) => void;
  onSetTtsVoice: (voice: TTSVoiceKey) => void;
  onToggleListening: () => void;
  onTriggerWakeUp: () => void;
}

export function AIVoiceWidget({
  voiceState,
  liveTranscript,
  countdown,
  isListening,
  supported,
  messages,
  lastAssistantText,
  status,
  modelKey,
  ttsEnabled,
  ttsVoice,
  language,
  onSetLanguage,
  onSetModelKey,
  onSetTtsEnabled,
  onSetTtsVoice,
  onToggleListening,
  onTriggerWakeUp,
}: AIVoiceWidgetProps) {
  const getBadgeStyle = () => {
    switch (voiceState) {
      case "SLEEPING":
        return "bg-indigo-500/10 text-indigo-400 ring-1 ring-indigo-500/30";
      case "AWAKE":
        return "bg-amber-500/10 text-amber-400 ring-1 ring-amber-500/30 animate-pulse";
      case "PROCESSING":
        return "bg-violet-500/10 text-violet-400 ring-1 ring-violet-500/30 animate-pulse";
      case "SPEAKING":
        return "bg-emerald-500/10 text-emerald-400 ring-1 ring-emerald-500/30";
      case "INACTIVE":
      default:
        return "bg-slate-800 text-slate-400 ring-1 ring-slate-700";
    }
  };

  const getBadgeText = () => {
    switch (voiceState) {
      case "SLEEPING":
        return "SLEEPING";
      case "AWAKE":
        return "LISTENING...";
      case "PROCESSING":
        return "AI THINKING...";
      case "SPEAKING":
        return ttsVoice === "browser-native" ? "SPEAKING (Native)..." : "SPEAKING (Edge TTS)...";
      case "INACTIVE":
      default:
        return "OFF";
    }
  };

  const getPromptHint = () => {
    if (!supported) {
      return "Web Speech API not supported. Please use Chrome or Edge.";
    }
    const isVi = language === "vi-VN";
    switch (voiceState) {
      case "SLEEPING":
        return isVi
          ? 'Nói "Trợ lý ơi", "Đèn ơi" hoặc "Wake Up" để đánh thức AI'
          : 'Say "Wake Up" or "Hey Lamp" to activate AI assistant';
      case "AWAKE":
        return isVi
          ? 'Đang nghe... Mời bạn nói lệnh tiếp theo (hoặc nói "ngủ đi")'
          : 'Listening... Say follow-up command (or say "sleep" / "bye")';
      case "PROCESSING":
        return isVi
          ? "AI đang xử lý mệnh lệnh và điều khiển thiết bị..."
          : "AI is understanding your command and controlling devices...";
      case "SPEAKING":
        return isVi ? "AI đang trả lời..." : "AI is responding...";
      case "INACTIVE":
      default:
        return isVi
          ? "Bật micro để điều khiển thiết bị bằng giọng nói AI"
          : "Turn on microphone to enable AI-powered voice control";
    }
  };

  // Get the last user message for display
  const lastUserText = (() => {
    for (let i = messages.length - 1; i >= 0; i--) {
      const msg = messages[i];
      if (msg && msg.role === "user") {
        for (const part of msg.parts) {
          if (part.type === "text") return part.text;
        }
      }
    }
    return "";
  })();

  return (
    <div className="rounded-2xl border border-white/10 bg-slate-900/80 p-6 shadow-xl backdrop-blur-md transition-all">
      {/* Top Header Row */}
      <div className="flex flex-col gap-4 sm:flex-row sm:items-center sm:justify-between">
        <div className="flex items-center gap-3">
          <div
            className={`flex h-11 w-11 items-center justify-center rounded-xl transition-all duration-300 ${
              isListening
                ? "bg-violet-500/20 text-violet-300 ring-1 ring-violet-500/40 shadow-[0_0_16px_rgba(139,92,246,0.3)]"
                : "bg-slate-800 text-slate-500"
            }`}
          >
            <svg
              className="h-5 w-5"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
            >
              <path d="M12 1a3 3 0 0 0-3 3v8a3 3 0 0 0 6 0V4a3 3 0 0 0-3-3z" />
              <path d="M19 10v2a7 7 0 0 1-14 0v-2" />
              <line x1="12" y1="19" x2="12" y2="23" />
              <line x1="8" y1="23" x2="16" y2="23" />
            </svg>
          </div>
          <div>
            <h2 className="text-base font-semibold text-white">
              AI Voice Control
            </h2>
            <p className="text-xs text-slate-400">
              Powered by AI SDK + Self-hosted LLM
            </p>
          </div>
        </div>

        {/* Right Controls */}
        <div className="flex items-center gap-2.5 sm:gap-3">
          {countdown !== null && (
            <span className="flex items-center gap-1.5 rounded-full bg-amber-500/10 px-2.5 py-1 text-xs font-semibold text-amber-400 ring-1 ring-amber-500/30">
              <span className="h-2 w-2 animate-ping rounded-full bg-amber-400" />
              <span>{countdown}s</span>
            </span>
          )}

          <span
            className={`rounded-full px-2.5 py-1 text-xs font-semibold uppercase tracking-wider ${getBadgeStyle()}`}
          >
            {getBadgeText()}
          </span>

          {/* Wake button */}
          {onTriggerWakeUp &&
            voiceState !== "AWAKE" &&
            voiceState !== "PROCESSING" &&
            voiceState !== "SPEAKING" && (
              <button
                type="button"
                onClick={onTriggerWakeUp}
                title="Activate voice command window"
                className="flex items-center gap-1 rounded-xl border border-white/10 bg-slate-800/80 px-2.5 py-2 text-xs font-medium text-amber-300 transition-all hover:bg-amber-500/10 hover:border-amber-500/30"
              >
                <span>⚡ Wake</span>
              </button>
            )}

          {/* Mic toggle */}
          <button
            type="button"
            aria-label={isListening ? "Turn off mic" : "Turn on mic"}
            disabled={!supported}
            onClick={onToggleListening}
            className={`relative flex h-10 w-10 shrink-0 items-center justify-center rounded-xl transition-all duration-300 ${
              !supported
                ? "cursor-not-allowed opacity-40 bg-slate-800 text-slate-500"
                : isListening
                ? "bg-violet-600 text-white shadow-[0_0_20px_rgba(139,92,246,0.6)] ring-2 ring-violet-400"
                : "bg-slate-800 text-slate-400 hover:bg-slate-700 hover:text-white"
            }`}
          >
            <svg
              className="h-5 w-5"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
            >
              <path d="M12 1a3 3 0 0 0-3 3v8a3 3 0 0 0 6 0V4a3 3 0 0 0-3-3z" />
              <path d="M19 10v2a7 7 0 0 1-14 0v-2" />
              <line x1="12" y1="19" x2="12" y2="23" />
              <line x1="8" y1="23" x2="16" y2="23" />
            </svg>
          </button>
        </div>
      </div>

      {/* Model Selector + Language + TTS Toggle */}
      <div className="mt-4 flex flex-col gap-3 sm:flex-row sm:items-center sm:gap-4">
        <div className="flex items-center gap-2 flex-1">
          <label
            htmlFor="model-select"
            className="text-xs font-medium text-slate-400 whitespace-nowrap"
          >
            🧠 Model:
          </label>
          <select
            id="model-select"
            value={modelKey}
            onChange={(e) => onSetModelKey(e.target.value as ModelKey)}
            className="flex-1 rounded-lg border border-white/10 bg-slate-800/80 px-3 py-1.5 text-xs text-slate-200 outline-none focus:border-violet-500/50 focus:ring-1 focus:ring-violet-500/30"
          >
            {MODEL_OPTIONS.map((opt) => (
              <option key={opt.key} value={opt.key}>
                {opt.label} — {opt.description}
              </option>
            ))}
          </select>
        </div>

        <div className="flex items-center gap-2">
          {/* Language Toggle */}
          <button
            type="button"
            aria-label="Toggle speech language"
            onClick={() => onSetLanguage(language === "vi-VN" ? "en-US" : "vi-VN")}
            className="flex items-center gap-1.5 rounded-lg border border-white/10 bg-slate-800/80 px-3 py-1.5 text-xs font-medium text-slate-200 transition-all hover:bg-slate-700/80 hover:text-white"
          >
            <span>{language === "vi-VN" ? "🇻🇳 Tiếng Việt" : "🇺🇸 English"}</span>
          </button>

          {/* TTS Toggle */}
          <button
            type="button"
            aria-label="Toggle voice response"
            onClick={() => onSetTtsEnabled(!ttsEnabled)}
            className={`flex items-center gap-1.5 rounded-lg border px-3 py-1.5 text-xs font-medium transition-all ${
              ttsEnabled
                ? "border-emerald-500/30 bg-emerald-500/10 text-emerald-400"
                : "border-white/10 bg-slate-800/80 text-slate-400"
            }`}
          >
            <span>{ttsEnabled ? "🔊" : "🔇"}</span>
            <span>TTS {ttsEnabled ? "On" : "Off"}</span>
          </button>

          {/* Voice Selector when TTS is enabled */}
          {ttsEnabled && (
            <select
              id="tts-voice-select"
              value={ttsVoice}
              aria-label="Select TTS voice"
              onChange={(e) => onSetTtsVoice(e.target.value as TTSVoiceKey)}
              className="rounded-lg border border-white/10 bg-slate-800/80 px-2 py-1.5 text-xs text-slate-200 outline-none focus:border-violet-500/50 focus:ring-1 focus:ring-violet-500/30"
            >
              {TTS_VOICE_OPTIONS.map((opt) => (
                <option key={opt.key} value={opt.key}>
                  {opt.label}
                </option>
              ))}
            </select>
          )}
        </div>
      </div>

      {/* State Description & Live Feedback */}
      <div className="mt-4 flex flex-col gap-3 rounded-xl border border-white/5 bg-slate-950/50 p-4">
        <div className="flex items-center justify-between text-xs text-slate-400">
          <span className="font-medium text-slate-300">{getPromptHint()}</span>
          {isListening && (
            <div className="flex items-end gap-1" title="Microphone Active">
              {[3, 5, 4, 2].map((h, i) => (
                <span
                  key={i}
                  className={`w-1 rounded-full bg-violet-400 ${
                    voiceState === "AWAKE" || voiceState === "PROCESSING"
                      ? "animate-bounce"
                      : "opacity-40"
                  }`}
                  style={{
                    height: `${h * 4}px`,
                    animationDelay: `${i * 150}ms`,
                  }}
                />
              ))}
            </div>
          )}
        </div>

        {/* Live transcript */}
        {isListening && (
          <div className="flex items-center gap-2 rounded-lg border border-slate-800/80 bg-slate-900/90 px-3 py-2 text-xs">
            <span
              className={`h-2 w-2 shrink-0 rounded-full ${
                voiceState === "AWAKE"
                  ? "bg-amber-400 animate-ping"
                  : voiceState === "PROCESSING"
                  ? "bg-violet-400 animate-pulse"
                  : "bg-indigo-400 animate-pulse"
              }`}
            />
            <span className="text-slate-400">
              {liveTranscript ? (
                <>
                  Heard:{" "}
                  <strong className="text-violet-300 font-mono">
                    &quot;{liveTranscript}&quot;
                  </strong>
                </>
              ) : (
                <span className="italic text-slate-500">
                  {voiceState === "AWAKE"
                    ? language === "vi-VN"
                      ? "Mời bạn nói lệnh tiếp theo (hoặc nói 'ngủ đi')..."
                      : "Say follow-up command (or say 'sleep')..."
                    : language === "vi-VN"
                    ? 'Đang chờ từ khoá "Trợ lý ơi" / "Wake Up"...'
                    : 'Listening for "Wake Up"...'}
                </span>
              )}
            </span>
          </div>
        )}

        {/* AI Agent Response */}
        {(lastUserText || lastAssistantText || status === "streaming") && (
          <div className="flex flex-col gap-2 border-t border-white/5 pt-3">
            {lastUserText && (
              <div className="flex items-start gap-2 text-xs">
                <span className="shrink-0 rounded bg-indigo-500/20 px-1.5 py-0.5 font-semibold text-indigo-300">
                  You
                </span>
                <span className="text-slate-300">{lastUserText}</span>
              </div>
            )}
            {(lastAssistantText || status === "streaming") && (
              <div className="flex items-start gap-2 text-xs">
                <span className="shrink-0 rounded bg-emerald-500/20 px-1.5 py-0.5 font-semibold text-emerald-300">
                  AI
                </span>
                <span className="text-slate-200">
                  {lastAssistantText || (
                    <span className="italic text-slate-500 animate-pulse">
                      Thinking...
                    </span>
                  )}
                </span>
              </div>
            )}
          </div>
        )}
      </div>

      {/* Voice Commands Help */}
      <div className="mt-3 rounded-xl border border-white/5 bg-slate-950/40 p-3 text-xs text-slate-400">
        <div className="mb-1.5 flex items-center gap-1.5 font-medium text-slate-300">
          <span>🗣️ AI Voice Commands</span>
        </div>
        <div className="flex flex-wrap items-center gap-1.5">
          <span>Gọi:</span>
          <code className="rounded bg-slate-800 px-1.5 py-0.5 text-indigo-300">
            &quot;Trợ lý ơi&quot;
          </code>
          <span>/</span>
          <code className="rounded bg-slate-800 px-1.5 py-0.5 text-indigo-300">
            &quot;Wake Up&quot;
          </code>
          <span>• Lệnh:</span>
          <code className="rounded bg-slate-800 px-1.5 py-0.5 text-amber-300">
            &quot;Bật đèn lên&quot;
          </code>
          <span>/</span>
          <code className="rounded bg-slate-800 px-1.5 py-0.5 text-amber-300">
            &quot;Tắt đèn đi&quot;
          </code>
          <span>• Nghỉ:</span>
          <code className="rounded bg-slate-800 px-1.5 py-0.5 text-slate-300">
            &quot;Ngủ đi&quot;
          </code>
        </div>
      </div>
    </div>
  );
}
