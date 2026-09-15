import type { Gesture, DeviceAction } from "../gesture/types";

type GestureStatusProps = {
  isActive: boolean;
  isHandDetected: boolean;
  gesture: Gesture;
  action: DeviceAction;
  fps: number;
  landmarkCount: number;
  error: string | null;
};

const GESTURE_EMOJI: Record<Gesture, string> = {
  OPEN_PALM: "🖐",
  FIST: "✊",
  THUMBS_UP: "👍",
  POINT: "☝️",
  UNKNOWN: "❓",
};

const ACTION_LABELS: Record<DeviceAction, string> = {
  TURN_ON: "Bật thiết bị",
  TURN_OFF: "Tắt thiết bị",
  INCREMENT: "Tăng bộ đếm",
  SELECT: "Chọn thiết bị",
  NONE: "—",
};

/**
 * Development diagnostics panel showing camera/hand/gesture status.
 */
export function GestureStatus({
  isActive,
  isHandDetected,
  gesture,
  action,
  fps,
  landmarkCount,
  error,
}: GestureStatusProps) {
  return (
    <div className="card-glass p-5 space-y-3">
      <h2 className="text-sm font-semibold uppercase tracking-wider text-slate-400 mb-3">
        Trạng thái Nhận diện
      </h2>

      {error && (
        <div className="bg-accent-red/10 border border-accent-red/30 text-accent-red rounded-lg px-3 py-2 text-sm">
          {error}
        </div>
      )}

      <div className="grid grid-cols-2 gap-y-2 text-sm">
        <StatusRow
          label="Camera"
          value={isActive ? "ON" : "OFF"}
          valueColor={isActive ? "text-accent-green" : "text-slate-500"}
        />
        <StatusRow
          label="Bàn tay"
          value={isHandDetected ? "Phát hiện" : "Không có"}
          valueColor={isHandDetected ? "text-accent-green" : "text-slate-500"}
        />
        <StatusRow
          label="Cử chỉ"
          value={`${GESTURE_EMOJI[gesture]} ${gesture}`}
          valueColor={
            gesture !== "UNKNOWN" ? "text-accent-cyan" : "text-slate-500"
          }
        />
        <StatusRow
          label="Hành động"
          value={ACTION_LABELS[action]}
          valueColor={action !== "NONE" ? "text-accent-amber" : "text-slate-500"}
        />
        <StatusRow
          label="FPS"
          value={String(fps)}
          valueColor={
            fps > 20
              ? "text-accent-green"
              : fps > 10
                ? "text-accent-amber"
                : "text-accent-red"
          }
        />
        <StatusRow
          label="Landmarks"
          value={String(landmarkCount)}
          valueColor="text-slate-300"
        />
      </div>
    </div>
  );
}

function StatusRow({
  label,
  value,
  valueColor,
}: {
  label: string;
  value: string;
  valueColor: string;
}) {
  return (
    <>
      <span className="text-slate-400">{label}</span>
      <span className={`font-medium ${valueColor}`}>{value}</span>
    </>
  );
}
