import type { DeviceAction } from "./types";

const DEFAULT_COOLDOWN_MS = 600;

/**
 * Action controller that prevents duplicate or rapid-fire action execution.
 *
 * - State transitions (TURN_ON, TURN_OFF, SELECT): fire on gesture change only.
 * - One-shot actions (INCREMENT): fire once, then enforce a cooldown period.
 * - NONE actions are always ignored.
 */
export function createActionController(cooldownMs: number = DEFAULT_COOLDOWN_MS) {
  let lastAction: DeviceAction = "NONE";
  let lastActionTime = 0;

  function shouldExecute(action: DeviceAction): boolean {
    if (action === "NONE") return false;

    const now = Date.now();

    // State transition actions: only fire when action changes
    if (action === "TURN_ON" || action === "TURN_OFF" || action === "SELECT") {
      if (action !== lastAction) {
        lastAction = action;
        lastActionTime = now;
        return true;
      }
      return false;
    }

    // One-shot with cooldown (INCREMENT)
    if (action === "INCREMENT") {
      if (action !== lastAction || now - lastActionTime >= cooldownMs) {
        lastAction = action;
        lastActionTime = now;
        return true;
      }
      return false;
    }

    return false;
  }

  function reset(): void {
    lastAction = "NONE";
    lastActionTime = 0;
  }

  return { shouldExecute, reset };
}

export type ActionController = ReturnType<typeof createActionController>;
