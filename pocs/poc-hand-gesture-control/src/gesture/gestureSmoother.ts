import type { Gesture } from "./types";

const DEFAULT_WINDOW_SIZE = 7;
const DEFAULT_MIN_CONSECUTIVE = 4;

/**
 * Temporal smoothing for gesture predictions.
 *
 * Keeps a rolling window of recent gestures and only changes
 * the "stable" gesture when a sufficient number of consecutive
 * frames agree on a new gesture.
 */
export function createGestureSmoother(
  windowSize: number = DEFAULT_WINDOW_SIZE,
  minConsecutive: number = DEFAULT_MIN_CONSECUTIVE
) {
  const buffer: Gesture[] = [];
  let stableGesture: Gesture = "UNKNOWN";

  function update(gesture: Gesture): Gesture {
    buffer.push(gesture);
    if (buffer.length > windowSize) {
      buffer.shift();
    }

    // Count the most recent consecutive same gestures
    let consecutiveCount = 0;
    for (let i = buffer.length - 1; i >= 0; i--) {
      if (buffer[i] === gesture) {
        consecutiveCount++;
      } else {
        break;
      }
    }

    // Only switch the stable gesture when we have enough consecutive observations
    if (consecutiveCount >= minConsecutive) {
      stableGesture = gesture;
    }

    return stableGesture;
  }

  function reset(): void {
    buffer.length = 0;
    stableGesture = "UNKNOWN";
  }

  function getStable(): Gesture {
    return stableGesture;
  }

  return { update, reset, getStable };
}

export type GestureSmoother = ReturnType<typeof createGestureSmoother>;
