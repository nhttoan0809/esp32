import "@testing-library/jest-dom/vitest";
import { vi } from "vitest";

// Mock Web Speech API
class MockSpeechRecognition {
  continuous = false;
  interimResults = false;
  lang = "en-US";
  onresult: ((event: unknown) => void) | null = null;
  onerror: ((event: unknown) => void) | null = null;
  onend: (() => void) | null = null;
  onstart: (() => void) | null = null;

  start = vi.fn();
  stop = vi.fn();
  abort = vi.fn();
}

Object.defineProperty(window, "webkitSpeechRecognition", {
  writable: true,
  value: MockSpeechRecognition,
});

Object.defineProperty(window, "SpeechRecognition", {
  writable: true,
  value: MockSpeechRecognition,
});

// Mock Web Audio API
class MockAudioContext {
  state = "suspended";
  sampleRate = 44100;
  resume = vi.fn().mockResolvedValue(undefined);
  close = vi.fn().mockResolvedValue(undefined);
  createOscillator = vi.fn().mockReturnValue({
    type: "sine",
    frequency: { setValueAtTime: vi.fn(), value: 440 },
    connect: vi.fn(),
    start: vi.fn(),
    stop: vi.fn(),
  });
  createGain = vi.fn().mockReturnValue({
    gain: {
      setValueAtTime: vi.fn(),
      exponentialRampToValueAtTime: vi.fn(),
      linearRampToValueAtTime: vi.fn(),
      value: 1,
    },
    connect: vi.fn(),
  });
  destination = {};
  currentTime = 0;
}

Object.defineProperty(window, "AudioContext", {
  writable: true,
  value: MockAudioContext,
});

Object.defineProperty(window, "webkitAudioContext", {
  writable: true,
  value: MockAudioContext,
});
