import type { Gesture, DeviceAction } from "./types";

/** Pure mapping from gesture to device action */
const GESTURE_ACTION_MAP: Record<Gesture, DeviceAction> = {
  OPEN_PALM: "TURN_ON",
  FIST: "TURN_OFF",
  THUMBS_UP: "INCREMENT",
  POINT: "SELECT",
  UNKNOWN: "NONE",
};

/**
 * Map a recognized gesture to an IoT device action.
 * This is a pure function with no side effects.
 */
export function mapGestureToAction(gesture: Gesture): DeviceAction {
  return GESTURE_ACTION_MAP[gesture] ?? "NONE";
}
