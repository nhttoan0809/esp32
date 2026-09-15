import { useState, useEffect, useRef, useCallback } from "react";
import { useCamera } from "./hooks/useCamera";
import { useHandTracking } from "./hooks/useHandTracking";
import { CameraView } from "./components/CameraView";
import { HandCanvas } from "./components/HandCanvas";
import { GestureStatus } from "./components/GestureStatus";
import { DevicePanel } from "./components/DevicePanel";
import { mapGestureToAction } from "./gesture/actionMapper";
import { createActionController } from "./gesture/actionController";
import type { DeviceState, DeviceAction } from "./gesture/types";

const GESTURE_GUIDE = [
  { emoji: "🖐", gesture: "OPEN_PALM", action: "Bật LED", desc: "Mở bàn tay" },
  { emoji: "✊", gesture: "FIST", action: "Tắt LED", desc: "Nắm tay" },
  { emoji: "👍", gesture: "THUMBS_UP", action: "Tăng counter", desc: "Giơ ngón cái" },
  { emoji: "☝️", gesture: "POINT", action: "Chọn thiết bị", desc: "Chỉ ngón trỏ" },
];

export default function App() {
  const { videoRef, isActive, error, startCamera, stopCamera } = useCamera();
  const { landmarks, gesture, isHandDetected, fps } = useHandTracking({
    videoRef,
    isActive,
  });

  const actionControllerRef = useRef(createActionController());
  const [deviceState, setDeviceState] = useState<DeviceState>({
    ledOn: false,
    counter: 0,
    selected: false,
  });
  const [lastAction, setLastAction] = useState<DeviceAction>("NONE");

  // Map gesture → action → device state
  const action = mapGestureToAction(gesture);

  useEffect(() => {
    if (!isActive || !isHandDetected) return;

    const controller = actionControllerRef.current;
    if (!controller.shouldExecute(action)) return;

    setLastAction(action);

    setDeviceState((prev) => {
      switch (action) {
        case "TURN_ON":
          return { ...prev, ledOn: true };
        case "TURN_OFF":
          return { ...prev, ledOn: false };
        case "INCREMENT":
          return { ...prev, counter: prev.counter + 1 };
        case "SELECT":
          return { ...prev, selected: !prev.selected };
        default:
          return prev;
      }
    });
  }, [action, isActive, isHandDetected]);

  // Reset action controller when camera stops
  useEffect(() => {
    if (!isActive) {
      actionControllerRef.current.reset();
    }
  }, [isActive]);

  const handleToggleCamera = useCallback(() => {
    if (isActive) {
      stopCamera();
    } else {
      startCamera();
    }
  }, [isActive, startCamera, stopCamera]);

  return (
    <div className="min-h-screen bg-surface p-4 md:p-8">
      {/* Header */}
      <header className="max-w-6xl mx-auto mb-6">
        <div className="flex items-center gap-3 mb-1">
          <div className="w-10 h-10 rounded-xl bg-gradient-to-br from-brand-500 to-accent-cyan flex items-center justify-center text-lg">
            ✋
          </div>
          <div>
            <h1 className="text-xl md:text-2xl font-bold text-white">
              Hand Gesture Controller
            </h1>
            <p className="text-xs text-slate-400">
              Điều khiển thiết bị IoT bằng cử chỉ tay • MediaPipe Hand Landmarker
            </p>
          </div>
        </div>
      </header>

      {/* Main Layout */}
      <main className="max-w-6xl mx-auto grid grid-cols-1 lg:grid-cols-3 gap-5">
        {/* Left Column: Camera + Controls */}
        <div className="lg:col-span-2 space-y-4">
          {/* Camera Feed */}
          <div className="camera-container card-glass overflow-hidden">
            {!isActive && (
              <div className="absolute inset-0 flex flex-col items-center justify-center z-10 bg-surface-card/90">
                <div className="text-5xl mb-4">📷</div>
                <p className="text-slate-400 text-sm mb-4">
                  Nhấn nút bên dưới để bắt đầu
                </p>
              </div>
            )}
            <CameraView videoRef={videoRef} isActive={isActive} />
            <HandCanvas landmarks={landmarks} videoRef={videoRef} />

            {/* Live gesture badge */}
            {isActive && isHandDetected && gesture !== "UNKNOWN" && (
              <div className="absolute top-3 left-3 z-20 gesture-icon-bounce">
                <div className="bg-black/60 backdrop-blur-sm rounded-full px-3 py-1.5 flex items-center gap-2 border border-white/10">
                  <span className="text-lg">
                    {GESTURE_GUIDE.find((g) => g.gesture === gesture)?.emoji}
                  </span>
                  <span className="text-sm font-semibold text-white">
                    {gesture}
                  </span>
                </div>
              </div>
            )}

            {/* FPS badge */}
            {isActive && (
              <div className="absolute top-3 right-3 z-20">
                <div className="bg-black/60 backdrop-blur-sm rounded-full px-2.5 py-1 text-xs font-mono text-slate-300 border border-white/10">
                  {fps} FPS
                </div>
              </div>
            )}
          </div>

          {/* Camera Button */}
          <button
            id="toggle-camera-btn"
            onClick={handleToggleCamera}
            className={`w-full py-3 rounded-xl font-semibold text-sm transition-all duration-300 cursor-pointer ${
              isActive
                ? "bg-accent-red/20 text-accent-red border border-accent-red/30 hover:bg-accent-red/30"
                : "bg-gradient-to-r from-brand-600 to-accent-cyan text-white hover:from-brand-500 hover:to-accent-cyan/90 shadow-lg shadow-brand-600/20"
            }`}
          >
            {isActive ? "⏹ Dừng Camera" : "▶ Bắt đầu Camera"}
          </button>

          {/* Gesture Guide */}
          <div className="card-glass p-4">
            <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-400 mb-3">
              Hướng dẫn cử chỉ
            </h3>
            <div className="grid grid-cols-2 md:grid-cols-4 gap-3">
              {GESTURE_GUIDE.map((g) => (
                <div
                  key={g.gesture}
                  className={`text-center p-3 rounded-xl border transition-all duration-300 ${
                    gesture === g.gesture
                      ? "bg-brand-600/20 border-brand-500/50 scale-105"
                      : "bg-surface/40 border-white/5 hover:border-white/10"
                  }`}
                >
                  <div className="text-2xl mb-1">{g.emoji}</div>
                  <div className="text-xs font-semibold text-slate-200">
                    {g.desc}
                  </div>
                  <div className="text-[10px] text-slate-500 mt-0.5">
                    {g.action}
                  </div>
                </div>
              ))}
            </div>
          </div>
        </div>

        {/* Right Column: Status + Device */}
        <div className="space-y-4">
          <GestureStatus
            isActive={isActive}
            isHandDetected={isHandDetected}
            gesture={gesture}
            action={action}
            fps={fps}
            landmarkCount={landmarks?.length ?? 0}
            error={error}
          />
          <DevicePanel state={deviceState} lastAction={lastAction} />
        </div>
      </main>
    </div>
  );
}
