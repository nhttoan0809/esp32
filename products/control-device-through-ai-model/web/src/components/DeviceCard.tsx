"use client";

import React from "react";
import type { DeviceStateResponse } from "@/lib/types";

interface DeviceCardProps {
  device: DeviceStateResponse;
  onToggle: (deviceId: string, currentOn: boolean) => void;
  isPending?: boolean;
}

export function DeviceCard({ device, onToggle, isPending = false }: DeviceCardProps) {
  const isOnline = device.online;
  const isLampOn = device.on === true;

  return (
    <div
      id={`card-${device.device_id}`}
      className="relative flex flex-col justify-between rounded-2xl border border-white/10 bg-slate-900/80 p-6 shadow-xl backdrop-blur-md transition-all duration-300 hover:border-slate-700/80 hover:shadow-2xl"
    >
      {/* Header */}
      <div className="flex items-start justify-between gap-4">
        <div className="flex items-center gap-3">
          <div className="flex h-11 w-11 items-center justify-center rounded-xl bg-indigo-500/10 text-indigo-400 ring-1 ring-indigo-500/20">
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
            <h3 className="font-semibold tracking-wide text-white">{device.device_id}</h3>
            <p className="text-xs text-slate-400">ESP32 DevKit V1 • 30-Pin</p>
          </div>
        </div>

        {/* Online / Offline badge */}
        <div
          className={`flex items-center gap-1.5 rounded-full px-2.5 py-1 text-xs font-medium ${
            isOnline
              ? "bg-emerald-500/10 text-emerald-400 ring-1 ring-emerald-500/30"
              : "bg-slate-800 text-slate-400 ring-1 ring-slate-700"
          }`}
        >
          <span
            className={`h-1.5 w-1.5 rounded-full ${
              isOnline ? "bg-emerald-400 shadow-[0_0_8px_rgba(52,211,153,0.8)]" : "bg-slate-500"
            }`}
          />
          <span>{isOnline ? "ONLINE" : "OFFLINE"}</span>
        </div>
      </div>

      {/* Lamp Control Box */}
      <div
        className={`my-6 flex items-center justify-between rounded-xl border p-4 transition-all duration-300 ${
          isLampOn
            ? "border-amber-500/30 bg-amber-500/5 shadow-[0_0_24px_rgba(245,158,11,0.1)]"
            : "border-slate-800 bg-slate-950/40"
        }`}
      >
        <div className="flex items-center gap-4">
          <div
            className={`flex h-12 w-12 items-center justify-center rounded-xl transition-all duration-300 ${
              isLampOn
                ? "bg-amber-400 text-slate-950 shadow-[0_0_20px_rgba(251,191,36,0.6)] ring-2 ring-amber-300"
                : "bg-slate-800 text-slate-500"
            }`}
          >
            <svg
              className="h-6 w-6"
              viewBox="0 0 24 24"
              fill="none"
              stroke="currentColor"
              strokeWidth="2"
              strokeLinecap="round"
              strokeLinejoin="round"
            >
              <path d="M9 18h6M10 22h4M12 2a7 7 0 0 0-7 7c0 2.38 1.19 4.47 3 5.74V17a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1v-2.26c1.81-1.27 3-3.36 3-5.74a7 7 0 0 0-7-7z" />
            </svg>
          </div>
          <div>
            <div className="flex items-center gap-2">
              <span className="font-medium text-slate-200">Smart Lamp</span>
              <span className="rounded bg-slate-800 px-1.5 py-0.5 text-[10px] font-medium text-indigo-400">
                GPIO 26 (Relay)
              </span>
            </div>
            <div
              className={`mt-0.5 text-xs font-medium transition-colors ${
                isLampOn ? "text-amber-400" : "text-slate-400"
              }`}
            >
              State: {isLampOn ? "● LIGHT ON" : "○ LIGHT OFF"}
            </div>
          </div>
        </div>

        {/* Modern Toggle Switch */}
        <label
          className={`relative inline-flex cursor-pointer items-center ${
            !isOnline || isPending ? "cursor-not-allowed opacity-50" : ""
          }`}
        >
          <input
            type="checkbox"
            checked={isLampOn}
            disabled={!isOnline || isPending}
            onChange={() => onToggle(device.device_id, isLampOn)}
            className="peer sr-only"
          />
          <div className="peer h-7 w-12 rounded-full bg-slate-700 transition-colors duration-200 after:absolute after:left-[3px] after:top-[3px] after:h-5 after:w-5 after:rounded-full after:bg-white after:transition-all after:content-[''] peer-checked:bg-amber-500 peer-checked:after:translate-x-5 peer-focus:outline-none peer-focus:ring-2 peer-focus:ring-amber-400/50" />
        </label>
      </div>

      {/* Voice Hints */}
      <div className="rounded-xl border border-white/5 bg-slate-950/40 p-3 text-xs text-slate-400">
        <div className="mb-1.5 flex items-center gap-1.5 font-medium text-slate-300">
          <span>🗣️ Voice Commands</span>
        </div>
        <div className="space-x-1">
          <span>Wake:</span>
          <code className="rounded bg-slate-800 px-1.5 py-0.5 text-indigo-300">
            &quot;Wake Up&quot;
          </code>
          <span>&bull; Toggle:</span>
          <code className="rounded bg-slate-800 px-1.5 py-0.5 text-indigo-300">
            &quot;Change status&quot;
          </code>
          <span>&bull; Direct:</span>
          <code className="rounded bg-slate-800 px-1.5 py-0.5 text-amber-300">
            &quot;Turn on&quot;
          </code>
          <span>/</span>
          <code className="rounded bg-slate-800 px-1.5 py-0.5 text-slate-400">
            &quot;Turn off&quot;
          </code>
        </div>
      </div>
    </div>
  );
}
