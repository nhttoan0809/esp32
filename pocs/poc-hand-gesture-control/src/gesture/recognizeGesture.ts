import type { HandLandmark, GestureResult, Gesture } from "./types";
import { detectFingerState } from "./gestures";

/**
 * Recognize a gesture from 21 hand landmarks.
 *
 * Rules:
 *   OPEN_PALM  — all five fingers extended
 *   FIST       — all five fingers closed
 *   POINT      — only index extended (thumb state ignored for flexibility)
 *   THUMBS_UP  — only thumb extended, all other fingers closed
 *   UNKNOWN    — anything else
 */
export function recognizeGesture(landmarks: HandLandmark[]): GestureResult {
  const fingerState = detectFingerState(landmarks);
  const { thumb, index, middle, ring, pinky } = fingerState;

  let gesture: Gesture = "UNKNOWN";

  if (thumb && index && middle && ring && pinky) {
    gesture = "OPEN_PALM";
  } else if (!thumb && !index && !middle && !ring && !pinky) {
    gesture = "FIST";
  } else if (thumb && !index && !middle && !ring && !pinky) {
    gesture = "THUMBS_UP";
  } else if (index && !middle && !ring && !pinky) {
    // POINT: index is extended, other 3 fingers closed. Thumb can be either.
    gesture = "POINT";
  }

  return { gesture, fingerState };
}
