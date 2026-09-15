// ─── Gesture domain types ───

/** Recognized hand gestures */
export type Gesture =
  | "OPEN_PALM"
  | "FIST"
  | "THUMBS_UP"
  | "POINT"
  | "UNKNOWN";

/** Per-finger extension state */
export type FingerState = {
  thumb: boolean;
  index: boolean;
  middle: boolean;
  ring: boolean;
  pinky: boolean;
};

/** Result from gesture recognition */
export type GestureResult = {
  gesture: Gesture;
  fingerState: FingerState;
};

// ─── IoT action types ───

/** Actions that map from gestures */
export type DeviceAction =
  | "TURN_ON"
  | "TURN_OFF"
  | "INCREMENT"
  | "SELECT"
  | "NONE";

/** Simulated IoT device state */
export type DeviceState = {
  ledOn: boolean;
  counter: number;
  selected: boolean;
};

// ─── Hand tracking types ───

export type HandLandmark = {
  x: number;
  y: number;
  z: number;
};

export type HandTrackingState = {
  landmarks: HandLandmark[] | null;
  gesture: Gesture;
  isHandDetected: boolean;
  fps: number;
};

// ─── Transport interface (future ESP32 integration) ───

export type DeviceTransport = {
  send: (action: DeviceAction) => Promise<void> | void;
  disconnect?: () => void;
};
