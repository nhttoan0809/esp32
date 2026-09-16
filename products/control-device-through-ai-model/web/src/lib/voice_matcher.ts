/**
 * English speech normalizer and intent matcher
 * Designed for real-time web speech recognition (Web Speech API)
 */

export function cleanSpeechText(str: string): string {
  return str
    .toLowerCase()
    .replace(/[.,/#!$%^&*;:{}=\-_`~()?!"]/g, " ")
    .replace(/\s+/g, " ")
    .trim();
}

const WAKE_WORDS = [
  "wake up",
  "hey lamp",
  "smart lamp",
  "hello lamp",
  "hello",
  "alexa",
];

export function detectWakeWord(text: string): {
  detected: boolean;
  remainder: string;
  matchedWord?: string;
} {
  const cleaned = cleanSpeechText(text);

  for (let i = 0; i < WAKE_WORDS.length; i++) {
    const word = WAKE_WORDS[i];
    if (!word) continue;
    const index = cleaned.indexOf(word);
    if (index !== -1) {
      const remainder = cleaned.slice(index + word.length).trim();
      return { detected: true, remainder, matchedWord: word };
    }
  }

  return { detected: false, remainder: "" };
}

const NEGATION_WORDS = ["don't", "dont", "do not", "never", "not", "no"];

const TOGGLE_PHRASES = [
  "change status",
  "switch state",
  "change state",
  "toggle lamp",
  "toggle light",
  "toggle",
];

const TURN_ON_PHRASES = [
  "turn on the light",
  "turn on the lamp",
  "turn on lamp",
  "turn on light",
  "turn on",
  "light on",
  "lamp on",
  "power on",
  "switch on",
];

const TURN_OFF_PHRASES = [
  "turn off the light",
  "turn off the lamp",
  "turn off lamp",
  "turn off light",
  "turn off",
  "light off",
  "lamp off",
  "power off",
  "switch off",
];

export interface DetectCommandOptions {
  allowSingleWords?: boolean;
}

export function detectCommand(
  text: string,
  options: DetectCommandOptions = {}
): "on" | "off" | "toggle" | null {
  const cleaned = cleanSpeechText(text);
  if (!cleaned) return null;

  // 1. Check for negative intent ("don't turn on", "do not turn off")
  const words = cleaned.split(" ");
  for (const neg of NEGATION_WORDS) {
    if (cleaned.includes(neg) || words.includes(neg)) {
      return null;
    }
  }

  // 2. Check toggle first
  if (TOGGLE_PHRASES.some((p) => cleaned.includes(p))) {
    return "toggle";
  }

  // 3. Check turn on phrases
  if (TURN_ON_PHRASES.some((p) => cleaned.includes(p))) {
    return "on";
  }

  // 4. Check turn off phrases
  if (TURN_OFF_PHRASES.some((p) => cleaned.includes(p))) {
    return "off";
  }

  // 5. Allow single word triggers only if explicitly enabled (e.g. in AWAKE state)
  if (options.allowSingleWords) {
    if (words.includes("on")) {
      return "on";
    }
    if (words.includes("off")) {
      return "off";
    }
    if (words.includes("toggle")) {
      return "toggle";
    }
  }

  return null;
}
