"use client";

import React from "react";

export type VoiceState = "INACTIVE" | "SLEEPING" | "AWAKE" | "EXECUTING";

interface VoiceWidgetProps {
  voiceState: VoiceState;
  transcript: string;
  liveTranscript?: string;
  countdown: number | null;
  isListening: boolean;
  onToggleListening: () => void;
  supported: boolean;
  onTriggerWakeUp?: () => void;
  onSimulateCommand?: (cmd: "on" | "off" | "toggle") => void;
}

export function VoiceWidget({
  voiceState,
  transcript,
  liveTranscript,
  countdown,
  isListening,
  onToggleListening,
  supported,
  onTriggerWakeUp,
  onSimulateCommand,
}: VoiceWidgetProps) {
  const getBadgeStyle = () => {
    switch (voiceState) {
      case "SLEEPING":
        return "bg-indigo-500/10 text-indigo-400 ring-1 ring-indigo-500/30";
      case "AWAKE":
        return "bg-amber-500/10 text-amber-400 ring-1 ring-amber-500/30 animate-pulse";
      case "EXECUTING":
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
      case "EXECUTING":
        return "EXECUTING...";
      case "INACTIVE":
      default:
        return "OFF";
    }
  };

  const getPromptHint = () => {
    if (!supported) {
      return "Web Speech API not supported in this browser. Please use Chrome or Edge.";
    }
    switch (voiceState) {
      case "SLEEPING":
        return 'Say "Wake Up" or "Hey Lamp" to activate';
      case "AWAKE":
        return 'Listening for "turn on", "turn off", or "change status"';
      case "EXECUTING":
        return "Applying command to smart lamp...";
      case "INACTIVE":
      default:
        return "Turn on microphone to enable always-listening voice control";
    }
  };

  const activeTranscript = liveTranscript || transcript;

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
            <h2 className="text-base font-semibold text-white">Voice Control</h2>
            <p className="text-xs text-slate-400">
              Always-Listening Engine (Web Speech & Audio API)
            </p>
          </div>
        </div>

        {/* Right Controls: Timer, Badge, Quick Actions & Mic Button */}
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

          {/* Quick manual wake button */}
          {onTriggerWakeUp && voiceState !== "AWAKE" && (
            <button
              type="button"
              onClick={onTriggerWakeUp}
              title="Activate voice command window immediately"
              className="flex items-center gap-1 rounded-xl border border-white/10 bg-slate-800/80 px-2.5 py-2 text-xs font-medium text-amber-300 transition-all hover:bg-amber-500/10 hover:border-amber-500/30"
            >
              <span>⚡ Wake</span>
            </button>
          )}

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

      {/* State Description & Soundwaves */}
      <div className="mt-4 flex flex-col gap-3 rounded-xl border border-white/5 bg-slate-950/50 p-4">
        <div className="flex items-center justify-between text-xs text-slate-400">
          <span className="font-medium text-slate-300">{getPromptHint()}</span>
          {isListening && (
            <div className="flex items-end gap-1" title="Microphone Active">
              <span
                className={`h-3 w-1 rounded-full bg-violet-400 ${
                  voiceState === "AWAKE" ? "animate-bounce" : "opacity-40"
                }`}
                style={{ animationDelay: "0ms" }}
              />
              <span
                className={`h-5 w-1 rounded-full bg-violet-400 ${
                  voiceState === "AWAKE" ? "animate-bounce" : "opacity-40"
                }`}
                style={{ animationDelay: "150ms" }}
              />
              <span
                className={`h-4 w-1 rounded-full bg-violet-400 ${
                  voiceState === "AWAKE" ? "animate-bounce" : "opacity-40"
                }`}
                style={{ animationDelay: "300ms" }}
              />
              <span
                className={`h-2 w-1 rounded-full bg-violet-400 ${
                  voiceState === "AWAKE" ? "animate-bounce" : "opacity-40"
                }`}
                style={{ animationDelay: "450ms" }}
              />
            </div>
          )}
        </div>

        {/* Live speech feedback */}
        {isListening && (
          <div className="flex items-center gap-2 rounded-lg border border-slate-800/80 bg-slate-900/90 px-3 py-2 text-xs">
            <span
              className={`h-2 w-2 shrink-0 rounded-full ${
                voiceState === "AWAKE"
                  ? "bg-amber-400 animate-ping"
                  : "bg-indigo-400 animate-pulse"
              }`}
            />
            <span className="text-slate-400">
              {activeTranscript ? (
                <>
                  Heard: <strong className="text-violet-300 font-mono">&quot;{activeTranscript}&quot;</strong>
                </>
              ) : (
                <span className="italic text-slate-500">
                  {voiceState === "AWAKE"
                    ? 'Say command now (e.g. "turn on", "turn off")...'
                    : 'Listening for "Wake Up" or direct command...'}
                </span>
              )}
            </span>
          </div>
        )}

        {/* Quick Simulator Buttons when in AWAKE state */}
        {voiceState === "AWAKE" && onSimulateCommand && (
          <div className="flex items-center gap-2 pt-1 border-t border-white/5">
            <span className="text-[11px] font-medium text-slate-400">Quick Test:</span>
            <button
              type="button"
              onClick={() => onSimulateCommand("on")}
              className="rounded-lg bg-amber-500/20 px-2.5 py-1 text-[11px] font-medium text-amber-300 hover:bg-amber-500/30 transition-colors"
            >
              💡 Turn ON
            </button>
            <button
              type="button"
              onClick={() => onSimulateCommand("off")}
              className="rounded-lg bg-slate-800 px-2.5 py-1 text-[11px] font-medium text-slate-300 hover:bg-slate-700 transition-colors"
            >
              🌑 Turn OFF
            </button>
            <button
              type="button"
              onClick={() => onSimulateCommand("toggle")}
              className="rounded-lg bg-indigo-500/20 px-2.5 py-1 text-[11px] font-medium text-indigo-300 hover:bg-indigo-500/30 transition-colors"
            >
              🔄 Toggle
            </button>
          </div>
        )}
      </div>
    </div>
  );
}
