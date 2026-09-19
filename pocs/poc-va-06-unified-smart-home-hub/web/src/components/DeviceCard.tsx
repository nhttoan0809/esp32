"use client";

import React, { useState } from "react";
import type { DeviceStateResponse } from "@/lib/types";

interface DeviceCardProps {
  device: DeviceStateResponse;
  onToggle: (deviceId: string, currentOn: boolean) => void;
  onSetBrightness?: (deviceId: string, brightness: number) => void;
  onSetSecurityMode?: (deviceId: string, mode: "guard" | "eco") => void;
  isPending?: boolean;
}

export function DeviceCard({
  device,
  onToggle,
  onSetBrightness,
  onSetSecurityMode,
  isPending = false,
}: DeviceCardProps) {
  const isOnline = device.online;
  const isLampOn = device.on === true;
  const sensors = device.sensors || {};
  const currentBrightness = sensors.led_brightness ?? 50;
  const securityMode = sensors.security_mode ?? "eco";
  const [sliderVal, setSliderVal] = useState<number>(currentBrightness);

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
            <p className="text-xs text-slate-400">ESP32 DevKit V1 • 30-Pin Hub</p>
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

      {/* Sensor telemetry: Climate & Ambient */}
      <div className="mt-4 grid grid-cols-2 sm:grid-cols-4 gap-2">
        {/* Temp */}
        <div className="rounded-xl border border-cyan-500/20 bg-cyan-950/20 p-2.5">
          <div className="flex items-center gap-1.5">
            <span className="text-base">🌡️</span>
            <p className="text-[10px] font-medium uppercase tracking-wider text-slate-400">
              Nhiệt độ
            </p>
          </div>
          <p className="mt-1 text-sm font-bold text-cyan-300">
            {sensors.temperature !== undefined ? `${sensors.temperature.toFixed(1)}°C` : "--"}
          </p>
        </div>

        {/* Humidity */}
        <div className="rounded-xl border border-sky-500/20 bg-sky-950/20 p-2.5">
          <div className="flex items-center gap-1.5">
            <span className="text-base">💧</span>
            <p className="text-[10px] font-medium uppercase tracking-wider text-slate-400">
              Độ ẩm
            </p>
          </div>
          <p className="mt-1 text-sm font-bold text-sky-300">
            {sensors.humidity !== undefined ? `${sensors.humidity.toFixed(1)}%` : "--"}
          </p>
        </div>

        {/* LDR Light */}
        <div className="rounded-xl border border-amber-500/20 bg-amber-950/20 p-2.5">
          <div className="flex items-center gap-1.5">
            <span className="text-base">{sensors.light_level === "bright" ? "☀️" : "🌙"}</span>
            <p className="text-[10px] font-medium uppercase tracking-wider text-slate-400">
              Ánh sáng
            </p>
          </div>
          <p className="mt-1 text-sm font-bold text-amber-300 uppercase">
            {sensors.light_level || "--"}
          </p>
        </div>

        {/* PIR Motion */}
        <div
          className={`rounded-xl border p-2.5 transition-colors ${
            sensors.motion_detected
              ? "border-rose-500/40 bg-rose-950/30 text-rose-300"
              : "border-slate-800 bg-slate-950/20 text-slate-400"
          }`}
        >
          <div className="flex items-center gap-1.5">
            <span className="text-base">{sensors.motion_detected ? "🚨" : "🛡️"}</span>
            <p className="text-[10px] font-medium uppercase tracking-wider text-slate-400">
              Chuyển động
            </p>
          </div>
          <p className="mt-1 text-sm font-bold">
            {sensors.motion_detected ? "PHÁT HIỆN" : "YÊN TĨNH"}
          </p>
        </div>
      </div>

      {/* Actuator 1: Relay Lamp (GPIO 5) */}
      <div
        className={`my-4 flex items-center justify-between rounded-xl border p-3.5 transition-all duration-300 ${
          isLampOn
            ? "border-amber-500/30 bg-amber-500/5 shadow-[0_0_24px_rgba(245,158,11,0.1)]"
            : "border-slate-800 bg-slate-950/40"
        }`}
      >
        <div className="flex items-center gap-3">
          <div
            className={`flex h-10 w-10 items-center justify-center rounded-xl transition-all duration-300 ${
              isLampOn
                ? "bg-amber-400 text-slate-950 shadow-[0_0_20px_rgba(251,191,36,0.6)] ring-2 ring-amber-300"
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
              <path d="M9 18h6M10 22h4M12 2a7 7 0 0 0-7 7c0 2.38 1.19 4.47 3 5.74V17a1 1 0 0 0 1 1h6a1 1 0 0 0 1-1v-2.26c1.81-1.27 3-3.36 3-5.74a7 7 0 0 0-7-7z" />
            </svg>
          </div>
          <div>
            <div className="flex items-center gap-2">
              <span className="text-sm font-medium text-slate-200">Relay Main Lamp</span>
              <span className="rounded bg-slate-800 px-1.5 py-0.5 text-[10px] font-medium text-indigo-400">
                GPIO 5 (Relay)
              </span>
            </div>
            <div
              className={`text-xs font-medium transition-colors ${
                isLampOn ? "text-amber-400" : "text-slate-400"
              }`}
            >
              Trạng thái: {isLampOn ? "● BẬT (ON)" : "○ TẮT (OFF)"}
            </div>
          </div>
        </div>

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

      {/* Actuator 2: Dimmer LED (GPIO 18) Slider */}
      <div className="mb-4 rounded-xl border border-slate-800 bg-slate-950/40 p-3.5">
        <div className="flex items-center justify-between text-xs mb-2">
          <div className="flex items-center gap-2">
            <span className="text-amber-300 font-medium">💡 Dimmer LED</span>
            <span className="rounded bg-slate-800 px-1.5 py-0.5 text-[10px] font-medium text-indigo-400">
              GPIO 18 (PWM)
            </span>
          </div>
          <span className="font-bold text-amber-400">{sliderVal}%</span>
        </div>
        <input
          type="range"
          min="0"
          max="100"
          value={sliderVal}
          disabled={!isOnline}
          onChange={(e) => setSliderVal(Number(e.target.value))}
          onMouseUp={() => onSetBrightness?.(device.device_id, sliderVal)}
          onTouchEnd={() => onSetBrightness?.(device.device_id, sliderVal)}
          className="w-full accent-amber-400 h-2 bg-slate-800 rounded-lg cursor-pointer"
        />
      </div>

      {/* Actuator 3: Security Mode (Guard vs Eco) */}
      <div className="mb-4 flex items-center justify-between rounded-xl border border-slate-800 bg-slate-950/40 p-3">
        <div className="flex items-center gap-2">
          <span className="text-sm font-medium text-slate-200">🛡️ Chế độ an ninh</span>
          <span
            className={`text-xs px-2 py-0.5 rounded font-semibold ${
              securityMode === "guard"
                ? "bg-rose-500/20 text-rose-300 border border-rose-500/30"
                : "bg-emerald-500/20 text-emerald-300 border border-emerald-500/30"
            }`}
          >
            {securityMode === "guard" ? "GUARD (CÒI BẬT)" : "ECO (TIẾT KIỆM)"}
          </span>
        </div>
        <button
          type="button"
          disabled={!isOnline}
          onClick={() =>
            onSetSecurityMode?.(
              device.device_id,
              securityMode === "guard" ? "eco" : "guard"
            )
          }
          className="rounded-lg bg-slate-800 hover:bg-slate-700 px-2.5 py-1 text-xs text-slate-200 transition-colors"
        >
          Đổi chế độ
        </button>
      </div>

      {/* Voice Hints */}
      <div className="rounded-xl border border-white/5 bg-slate-950/40 p-3 text-xs text-slate-400">
        <div className="mb-1.5 flex items-center gap-1.5 font-medium text-slate-300">
          <span>🗣️ Câu lệnh mẫu (Voice)</span>
        </div>
        <div className="space-y-1">
          <div className="flex items-center gap-1 flex-wrap">
            <span className="text-slate-400">Khí hậu:</span>
            <code className="rounded bg-slate-800 px-1.5 py-0.5 text-cyan-300">
              &quot;Nhiệt độ phòng bao nhiêu?&quot;
            </code>
          </div>
          <div className="flex items-center gap-1 flex-wrap">
            <span className="text-slate-400">Đèn & Dimmer:</span>
            <code className="rounded bg-slate-800 px-1.5 py-0.5 text-amber-300">
              &quot;Bật đèn&quot;
            </code>
            <span>•</span>
            <code className="rounded bg-slate-800 px-1.5 py-0.5 text-amber-300">
              &quot;Đặt độ sáng 80%&quot;
            </code>
          </div>
          <div className="flex items-center gap-1 flex-wrap">
            <span className="text-slate-400">An ninh:</span>
            <code className="rounded bg-slate-800 px-1.5 py-0.5 text-rose-300">
              &quot;Bật chế độ bảo vệ&quot;
            </code>
            <span>•</span>
            <code className="rounded bg-slate-800 px-1.5 py-0.5 text-emerald-300">
              &quot;Chuyển sang eco mode&quot;
            </code>
          </div>
        </div>
      </div>
    </div>
  );
}
