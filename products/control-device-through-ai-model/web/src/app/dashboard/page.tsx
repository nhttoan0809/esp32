"use client";

import React, { useState, useEffect, useCallback } from "react";
import { useRouter } from "next/navigation";
import { useDevices } from "@/hooks/useDevices";
import { useAIVoice } from "@/hooks/useAIVoice";
import { DeviceCard } from "@/components/DeviceCard";
import { AIVoiceWidget } from "@/components/AIVoiceWidget";

export default function DashboardPage() {
  const router = useRouter();
  const [apiKey] = useState<string | null>(() => {
    if (typeof window !== "undefined") {
      return localStorage.getItem("lamp_dashboard_api_key");
    }
    return null;
  });
  const [toastMessage, setToastMessage] = useState<string | null>(null);

  const showToast = useCallback((msg: string) => {
    setToastMessage(msg);
    setTimeout(() => {
      setToastMessage((current) => (current === msg ? null : current));
    }, 4000);
  }, []);

  const handleUnauthorized = useCallback(() => {
    localStorage.removeItem("lamp_dashboard_api_key");
    router.push("/login");
  }, [router]);

  useEffect(() => {
    if (!apiKey) {
      router.push("/login");
    }
  }, [apiKey, router]);

  const { devices, loading, pendingIds, toggleDevice, refreshDevices } = useDevices(
    apiKey,
    handleUnauthorized
  );

  const {
    voiceState,
    liveTranscript,
    countdown,
    supported,
    isListening,
    messages,
    lastAssistantText,
    status,
    modelKey,
    setModelKey,
    ttsEnabled,
    setTtsEnabled,
    language,
    setLanguage,
    toggleListening,
    triggerWakeUp,
  } = useAIVoice({
    onShowToast: showToast,
    onDevicesChanged: refreshDevices,
  });

  const handleLogout = () => {
    localStorage.removeItem("lamp_dashboard_api_key");
    router.push("/login");
  };

  if (!apiKey) {
    return null;
  }

  const onlineCount = devices.filter((d) => d.online).length;

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 antialiased">
      {/* Toast Notification */}
      {toastMessage && (
        <div className="fixed bottom-6 right-6 z-50 flex items-center gap-3 rounded-2xl border border-indigo-500/30 bg-slate-900/95 px-5 py-3.5 shadow-2xl backdrop-blur-md">
          <span className="flex h-2 w-2 rounded-full bg-indigo-400 animate-ping" />
          <span className="text-sm font-medium text-white">{toastMessage}</span>
        </div>
      )}

      {/* Main Container */}
      <div className="mx-auto max-w-6xl px-4 py-8 sm:px-6 lg:px-8">
        {/* Navigation Bar */}
        <header className="mb-8 flex flex-col gap-4 rounded-2xl border border-white/10 bg-slate-900/80 p-6 shadow-xl backdrop-blur-md sm:flex-row sm:items-center sm:justify-between">
          <div className="flex items-center gap-4">
            <div className="flex h-12 w-12 items-center justify-center rounded-2xl bg-indigo-500/10 text-indigo-400 ring-1 ring-indigo-500/20">
              <svg
                className="h-6 w-6"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2"
                strokeLinecap="round"
                strokeLinejoin="round"
              >
                <rect x="4" y="4" width="16" height="16" rx="2" />
                <rect x="9" y="9" width="6" height="6" />
                <line x1="9" y1="1" x2="9" y2="4" />
                <line x1="15" y1="1" x2="15" y2="4" />
                <line x1="9" y1="20" x2="9" y2="23" />
                <line x1="15" y1="20" x2="15" y2="23" />
                <line x1="20" y1="9" x2="23" y2="9" />
                <line x1="20" y1="14" x2="23" y2="14" />
                <line x1="1" y1="9" x2="4" y2="9" />
                <line x1="1" y1="14" x2="4" y2="14" />
              </svg>
            </div>
            <div>
              <div className="flex items-center gap-2">
                <h1 className="text-xl font-bold tracking-tight text-white">Smart Lamp Control</h1>
                <span className="flex items-center gap-1.5 rounded-full bg-emerald-500/10 px-2 py-0.5 text-[11px] font-medium text-emerald-400 ring-1 ring-emerald-500/30">
                  <span className="h-1.5 w-1.5 rounded-full bg-emerald-400 shadow-[0_0_8px_rgba(52,211,153,0.8)]" />
                  SYSTEM ONLINE
                </span>
              </div>
              <p className="text-xs text-slate-400">WebSocket WSS • Next.js Real-time Dashboard</p>
            </div>
          </div>

          <div className="flex items-center gap-3">
            <div className="hidden rounded-xl border border-white/5 bg-slate-950/60 px-3 py-1.5 text-xs text-slate-400 sm:block">
              Connected: <strong className="text-white">{onlineCount}</strong>/{devices.length}
            </div>
            <button
              type="button"
              onClick={handleLogout}
              className="flex items-center gap-2 rounded-xl border border-white/10 bg-slate-800 px-4 py-2 text-xs font-semibold text-slate-300 transition-all hover:border-red-500/40 hover:bg-red-500/10 hover:text-red-400"
            >
              <svg
                className="h-4 w-4"
                viewBox="0 0 24 24"
                fill="none"
                stroke="currentColor"
                strokeWidth="2"
                strokeLinecap="round"
                strokeLinejoin="round"
              >
                <path d="M9 21H5a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h4" />
                <polyline points="16 17 21 12 16 7" />
                <line x1="21" y1="12" x2="9" y2="12" />
              </svg>
              <span>Disconnect</span>
            </button>
          </div>
        </header>

        {/* AI Voice Control Section */}
        <section className="mb-8">
          <AIVoiceWidget
            voiceState={voiceState}
            liveTranscript={liveTranscript}
            countdown={countdown}
            isListening={isListening}
            supported={supported}
            messages={messages}
            lastAssistantText={lastAssistantText}
            status={status}
            modelKey={modelKey}
            ttsEnabled={ttsEnabled}
            language={language}
            onSetLanguage={setLanguage}
            onSetModelKey={setModelKey}
            onSetTtsEnabled={setTtsEnabled}
            onToggleListening={toggleListening}
            onTriggerWakeUp={triggerWakeUp}
          />
        </section>

        {/* Devices Section */}
        <section>
          <div className="mb-4 flex items-center justify-between">
            <h2 className="text-sm font-semibold uppercase tracking-wider text-slate-400">
              Registered Devices ({devices.length})
            </h2>
            {loading && (
              <span className="text-xs text-slate-500 animate-pulse">Syncing...</span>
            )}
          </div>

          {devices.length === 0 ? (
            <div className="flex flex-col items-center justify-center rounded-2xl border border-dashed border-slate-800 bg-slate-900/30 p-12 text-center">
              <div className="mb-3 flex h-12 w-12 items-center justify-center rounded-2xl bg-slate-800 text-slate-500">
                <svg
                  className="h-6 w-6"
                  viewBox="0 0 24 24"
                  fill="none"
                  stroke="currentColor"
                  strokeWidth="2"
                >
                  <circle cx="12" cy="12" r="10" />
                  <line x1="12" y1="8" x2="12" y2="12" />
                  <line x1="12" y1="16" x2="12.01" y2="16" />
                </svg>
              </div>
              <p className="max-w-md text-sm text-slate-400">
                No ESP32 devices found on server. Ensure firmware is configured and online.
              </p>
            </div>
          ) : (
            <div className="grid grid-cols-1 gap-6 md:grid-cols-2">
              {devices.map((dev) => (
                <DeviceCard
                  key={dev.device_id}
                  device={dev}
                  onToggle={toggleDevice}
                  isPending={pendingIds.has(dev.device_id)}
                />
              ))}
            </div>
          )}
        </section>
      </div>
    </div>
  );
}
