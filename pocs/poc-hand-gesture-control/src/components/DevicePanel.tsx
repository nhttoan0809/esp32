import type { DeviceState, DeviceAction } from "../gesture/types";

type DevicePanelProps = {
  state: DeviceState;
  lastAction: DeviceAction;
};

const ACTION_LABELS: Record<DeviceAction, string> = {
  TURN_ON: "🔌 TURN_ON",
  TURN_OFF: "🔌 TURN_OFF",
  INCREMENT: "➕ INCREMENT",
  SELECT: "🎯 SELECT",
  NONE: "— NONE",
};

/**
 * Simulated IoT device panel showing LED state, counter, and selection status.
 */
export function DevicePanel({ state, lastAction }: DevicePanelProps) {
  const { ledOn, counter, selected } = state;

  return (
    <div className="card-glass p-5 space-y-4">
      <h2 className="text-sm font-semibold uppercase tracking-wider text-slate-400">
        Thiết bị IoT ảo
      </h2>

      {/* LED Indicator */}
      <div className="flex items-center gap-4">
        <div className="flex items-center gap-3 flex-1">
          <span className="text-slate-300 text-sm font-medium w-16">LED</span>
          <div
            className={`w-6 h-6 rounded-full transition-all duration-300 ${
              ledOn
                ? "bg-accent-green led-glow text-accent-green"
                : "bg-slate-600"
            }`}
          />
          <span
            className={`text-sm font-semibold ${
              ledOn ? "text-accent-green" : "text-slate-500"
            }`}
          >
            {ledOn ? "ON" : "OFF"}
          </span>
        </div>
      </div>

      {/* Counter */}
      <div className="flex items-center gap-3">
        <span className="text-slate-300 text-sm font-medium w-16">Counter</span>
        <div className="bg-surface/60 border border-white/5 rounded-lg px-4 py-2 min-w-[64px] text-center">
          <span className="text-2xl font-bold text-brand-400 tabular-nums">
            {counter}
          </span>
        </div>
      </div>

      {/* Selected */}
      <div className="flex items-center gap-3">
        <span className="text-slate-300 text-sm font-medium w-16">Selected</span>
        <div
          className={`px-3 py-1 rounded-full text-xs font-semibold transition-all duration-300 ${
            selected
              ? "bg-accent-amber/20 text-accent-amber border border-accent-amber/30"
              : "bg-slate-700/50 text-slate-500 border border-white/5"
          }`}
        >
          {selected ? "YES" : "NO"}
        </div>
      </div>

      {/* Last Action */}
      <div className="border-t border-white/5 pt-3">
        <div className="flex items-center justify-between">
          <span className="text-slate-400 text-xs">Hành động cuối</span>
          <span className="text-sm font-medium text-slate-200">
            {ACTION_LABELS[lastAction]}
          </span>
        </div>
      </div>
    </div>
  );
}
