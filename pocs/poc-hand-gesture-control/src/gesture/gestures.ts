import type { HandLandmark, FingerState } from "./types";
import { LANDMARK, distance2D, angleBetween } from "./landmarks";

// ─── Thresholds ───

/** Angle threshold (radians) below which a finger is considered "curled" / closed */
const CURL_ANGLE_THRESHOLD = 2.6; // ~149 degrees — fingers more bent than this are closed

/**
 * Determine if each finger is extended (open) or curled (closed).
 *
 * For the four fingers (index, middle, ring, pinky):
 *   We check the angle at the PIP joint (MCP-PIP-TIP).
 *   A straight (extended) finger has an angle close to π (180°).
 *   A curled finger has a smaller angle.
 *
 * For the thumb:
 *   The thumb moves laterally, so we compare the distance from
 *   thumb tip to index MCP vs thumb IP to index MCP.
 *   If the tip is farther out than the IP joint, the thumb is extended.
 */
export function detectFingerState(landmarks: HandLandmark[]): FingerState {
  // ── Thumb ──
  // Compare thumb tip distance to palm center vs thumb IP distance
  const thumbTipToIndexMcp = distance2D(
    landmarks[LANDMARK.THUMB_TIP],
    landmarks[LANDMARK.INDEX_MCP]
  );
  const thumbIPToIndexMcp = distance2D(
    landmarks[LANDMARK.THUMB_IP],
    landmarks[LANDMARK.INDEX_MCP]
  );
  const thumb = thumbTipToIndexMcp > thumbIPToIndexMcp * 1.1;

  // ── Index ──
  const indexAngle = angleBetween(
    landmarks[LANDMARK.INDEX_MCP],
    landmarks[LANDMARK.INDEX_PIP],
    landmarks[LANDMARK.INDEX_TIP]
  );
  const index = indexAngle > CURL_ANGLE_THRESHOLD;

  // ── Middle ──
  const middleAngle = angleBetween(
    landmarks[LANDMARK.MIDDLE_MCP],
    landmarks[LANDMARK.MIDDLE_PIP],
    landmarks[LANDMARK.MIDDLE_TIP]
  );
  const middle = middleAngle > CURL_ANGLE_THRESHOLD;

  // ── Ring ──
  const ringAngle = angleBetween(
    landmarks[LANDMARK.RING_MCP],
    landmarks[LANDMARK.RING_PIP],
    landmarks[LANDMARK.RING_TIP]
  );
  const ring = ringAngle > CURL_ANGLE_THRESHOLD;

  // ── Pinky ──
  const pinkyAngle = angleBetween(
    landmarks[LANDMARK.PINKY_MCP],
    landmarks[LANDMARK.PINKY_PIP],
    landmarks[LANDMARK.PINKY_TIP]
  );
  const pinky = pinkyAngle > CURL_ANGLE_THRESHOLD;

  return { thumb, index, middle, ring, pinky };
}
