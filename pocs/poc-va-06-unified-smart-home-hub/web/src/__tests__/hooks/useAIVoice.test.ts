import { describe, it, expect, vi, beforeEach, afterEach } from "vitest";
import { renderHook, act } from "@testing-library/react";
import { useAIVoice } from "@/hooks/useAIVoice";

// Mock @ai-sdk/react useChat
const mockSendMessage = vi.fn();
let mockChatOnFinish: ((options: { message: unknown }) => void) | null = null;
let mockChatOnError: ((error: Error) => void) | null = null;

vi.mock("@ai-sdk/react", () => ({
  useChat: vi.fn(({ onFinish, onError }) => {
    mockChatOnFinish = onFinish;
    mockChatOnError = onError;
    return {
      messages: [],
      sendMessage: mockSendMessage,
      status: "ready",
    };
  }),
}));

// Mock useAudio
const mockResumeAudio = vi.fn().mockResolvedValue(undefined);
const mockPlayWakeChime = vi.fn();
const mockPlaySuccessChime = vi.fn();
const mockPlayTimeoutChime = vi.fn();

vi.mock("@/hooks/useAudio", () => ({
  useAudio: () => ({
    resumeAudio: mockResumeAudio,
    playWakeChime: mockPlayWakeChime,
    playSuccessChime: mockPlaySuccessChime,
    playTimeoutChime: mockPlayTimeoutChime,
  }),
}));

interface MockSpeechRecognitionInstance {
  continuous: boolean;
  interimResults: boolean;
  lang: string;
  onresult: ((event: unknown) => void) | null;
  onerror: ((event: unknown) => void) | null;
  onend: (() => void) | null;
  start: ReturnType<typeof vi.fn>;
  stop: ReturnType<typeof vi.fn>;
  abort: ReturnType<typeof vi.fn>;
}

let latestRecognition: MockSpeechRecognitionInstance | null = null;

class MockSpeechRecognition implements MockSpeechRecognitionInstance {
  continuous = false;
  interimResults = false;
  lang = "vi-VN";
  onresult: ((event: unknown) => void) | null = null;
  onerror: ((event: unknown) => void) | null = null;
  onend: (() => void) | null = null;
  start = vi.fn();
  stop = vi.fn();
  abort = vi.fn();

  constructor() {
    MockSpeechRecognition.register(this);
  }

  static register(instance: MockSpeechRecognitionInstance) {
    latestRecognition = instance;
  }
}

let latestUtterance: MockUtterance | null = null;

class MockUtterance {
  text: string;
  lang = "vi-VN";
  rate = 1;
  pitch = 1;
  onend: (() => void) | null = null;
  onerror: (() => void) | null = null;
  constructor(text: string) {
    this.text = text;
    MockUtterance.register(this);
  }
  static register(instance: MockUtterance) {
    latestUtterance = instance;
  }
}

let latestAudio: MockAudio | null = null;

class MockAudio {
  src: string;
  onplay: (() => void) | null = null;
  onended: (() => void) | null = null;
  onerror: (() => void) | null = null;
  currentTime = 0;
  pause = vi.fn();
  play = vi.fn().mockImplementation(async () => {
    this.onplay?.();
  });
  constructor(src: string) {
    this.src = src;
    MockAudio.register(this);
  }
  static register(instance: MockAudio) {
    latestAudio = instance;
  }
}

describe("useAIVoice Hook", () => {
  beforeEach(() => {
    vi.useFakeTimers();
    vi.clearAllMocks();
    latestRecognition = null;
    latestUtterance = null;

    // Mock SpeechRecognition in window
    (window as unknown as { SpeechRecognition: unknown }).SpeechRecognition = MockSpeechRecognition;

    // Mock SpeechSynthesis
    (window as unknown as { speechSynthesis: unknown }).speechSynthesis = {
      speak: vi.fn(),
      cancel: vi.fn(),
    };
    (window as unknown as { SpeechSynthesisUtterance: unknown }).SpeechSynthesisUtterance = MockUtterance;

    // Mock Audio & fetch for Edge TTS
    latestAudio = null;
    (window as unknown as { Audio: unknown }).Audio = MockAudio;
    global.Audio = MockAudio as unknown as typeof Audio;
    if (typeof URL.createObjectURL === "undefined") {
      URL.createObjectURL = vi.fn().mockReturnValue("blob:mock-audio-url");
      URL.revokeObjectURL = vi.fn();
    } else {
      vi.spyOn(URL, "createObjectURL").mockReturnValue("blob:mock-audio-url");
      vi.spyOn(URL, "revokeObjectURL").mockImplementation(() => {});
    }

    global.fetch = vi.fn().mockResolvedValue({
      ok: true,
      status: 200,
      blob: async () => new Blob(["mock-mp3"], { type: "audio/mpeg" }),
    } as unknown as Response);
  });

  afterEach(() => {
    vi.useRealTimers();
  });

  it("initializes with INACTIVE state and supported true", () => {
    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    expect(result.current.voiceState).toBe("INACTIVE");
    expect(result.current.isListening).toBe(false);
    expect(result.current.supported).toBe(true);
    expect(result.current.language).toBe("vi-VN");
  });

  it("transitions to SLEEPING on startListening and starts recognition", () => {
    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    act(() => {
      result.current.startListening();
    });

    expect(result.current.voiceState).toBe("SLEEPING");
    expect(result.current.isListening).toBe(true);
    expect(latestRecognition?.start).toHaveBeenCalled();
  });

  it("wakes up to AWAKE state with 8s countdown upon detecting wake word, and DOES NOT call AI agent", () => {
    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    act(() => {
      result.current.startListening();
    });

    // Simulate wake word "trợ lý ơi" from Web Speech API
    act(() => {
      latestRecognition?.onresult?.({
        resultIndex: 0,
        results: [
          {
            0: { transcript: "trợ lý ơi" },
            isFinal: true,
            length: 1,
          },
        ],
      });
    });

    // MUST be in AWAKE state, NOT SLEEPING, NOT PROCESSING!
    expect(result.current.voiceState).toBe("AWAKE");
    expect(result.current.countdown).toBe(8);
    expect(mockPlayWakeChime).toHaveBeenCalled();
    // CRITICAL: Agent must NOT have been called with "trợ lý ơi"
    expect(mockSendMessage).not.toHaveBeenCalled();
  });

  it("ignores trailing wake word echo during AWAKE state", () => {
    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    act(() => {
      result.current.startListening();
    });

    // Initial wake word
    act(() => {
      latestRecognition?.onresult?.({
        resultIndex: 0,
        results: [
          {
            0: { transcript: "wake up" },
            isFinal: false,
            length: 1,
          },
        ],
      });
    });

    expect(result.current.voiceState).toBe("AWAKE");

    // Trailing final event for the same "wake up" immediately after
    act(() => {
      latestRecognition?.onresult?.({
        resultIndex: 0,
        results: [
          {
            0: { transcript: "wake up" },
            isFinal: true,
            length: 1,
          },
        ],
      });
    });

    // Assistant MUST stay in AWAKE and NOT send "wake up" to agent
    expect(result.current.voiceState).toBe("AWAKE");
    expect(mockSendMessage).not.toHaveBeenCalled();
  });

  it("dispatches command when user speaks in AWAKE state", () => {
    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    act(() => {
      result.current.startListening();
    });

    // Wake up
    act(() => {
      latestRecognition?.onresult?.({
        resultIndex: 0,
        results: [{ 0: { transcript: "trợ lý ơi" }, isFinal: true, length: 1 }],
      });
    });

    expect(result.current.voiceState).toBe("AWAKE");

    // Advance past the 800ms ignore window
    act(() => {
      vi.advanceTimersByTime(900);
    });

    // User speaks their actual command
    act(() => {
      latestRecognition?.onresult?.({
        resultIndex: 0,
        results: [{ 0: { transcript: "bật đèn phòng khách" }, isFinal: true, length: 1 }],
      });
    });

    // MUST transition to PROCESSING and send text to AI agent
    expect(result.current.voiceState).toBe("PROCESSING");
    expect(mockSendMessage).toHaveBeenCalledWith({ text: "bật đèn phòng khách" });
  });

  it("handles single-breath wake + command directly from SLEEPING state", () => {
    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    act(() => {
      result.current.startListening();
    });

    // Single breath: "wake up turn off the light"
    act(() => {
      latestRecognition?.onresult?.({
        resultIndex: 0,
        results: [{ 0: { transcript: "wake up turn off the light" }, isFinal: true, length: 1 }],
      });
    });

    expect(result.current.voiceState).toBe("PROCESSING");
    expect(mockSendMessage).toHaveBeenCalledWith({ text: "turn off the light" });
  });

  it("handles AI response completion, enters AWAKE and starts countdown ONLY AFTER response finishes", async () => {
    const onShowToast = vi.fn();
    const onDevicesChanged = vi.fn();
    const { result } = renderHook(() =>
      useAIVoice({ onShowToast, onDevicesChanged })
    );

    act(() => {
      result.current.startListening();
    });

    act(() => {
      result.current.sendToAgent("bật đèn");
    });

    expect(result.current.voiceState).toBe("PROCESSING");
    // During processing, countdown is not running
    expect(result.current.countdown).toBeNull();

    // Simulate AI response finishing stream
    await act(async () => {
      mockChatOnFinish?.({
        message: {
          parts: [{ type: "text", text: "Đã bật đèn thành công." }],
        },
      });
    });

    expect(mockPlaySuccessChime).toHaveBeenCalled();
    expect(result.current.voiceState).toBe("SPEAKING");
    expect(onDevicesChanged).toHaveBeenCalled();
    // While speaking response, countdown is still not running
    expect(result.current.countdown).toBeNull();

    // NOW simulate speech/audio completing
    act(() => {
      if (latestAudio?.onended) {
        latestAudio.onended();
      } else {
        latestUtterance?.onend?.();
      }
    });

    // CRITICAL: Countdown starts ONLY AFTER response is completely finished and spoken!
    expect(result.current.voiceState).toBe("AWAKE");
    expect(result.current.countdown).toBe(8);

    // If 8 seconds elapse without follow-up input:
    act(() => {
      vi.advanceTimersByTime(8000);
    });

    expect(result.current.voiceState).toBe("SLEEPING");
    expect(mockPlayTimeoutChime).toHaveBeenCalled();
  });

  it("accepts follow-up command during post-response countdown", async () => {
    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    act(() => {
      result.current.startListening();
      result.current.sendToAgent("bật đèn");
    });

    await act(async () => {
      mockChatOnFinish?.({
        message: { parts: [{ type: "text", text: "Đã bật đèn." }] },
      });
    });

    act(() => {
      if (latestAudio?.onended) {
        latestAudio.onended();
      } else {
        latestUtterance?.onend?.();
      }
    });

    expect(result.current.voiceState).toBe("AWAKE");
    expect(result.current.countdown).toBe(8);

    // Advance 900ms past ignore window
    act(() => {
      vi.advanceTimersByTime(900);
    });

    // User gives follow-up command
    act(() => {
      latestRecognition?.onresult?.({
        resultIndex: 0,
        results: [{ 0: { transcript: "tắt đèn đi" }, isFinal: true, length: 1 }],
      });
    });

    expect(result.current.voiceState).toBe("PROCESSING");
    expect(mockSendMessage).toHaveBeenCalledWith({ text: "tắt đèn đi" });
  });

  it("handles sleep phrases during AWAKE countdown", () => {
    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    act(() => {
      result.current.startListening();
      result.current.triggerWakeUp();
    });

    expect(result.current.voiceState).toBe("AWAKE");

    // Advance past ignore window
    act(() => {
      vi.advanceTimersByTime(900);
    });

    act(() => {
      latestRecognition?.onresult?.({
        resultIndex: 0,
        results: [{ 0: { transcript: "ngủ đi" }, isFinal: true, length: 1 }],
      });
    });

    expect(result.current.voiceState).toBe("SLEEPING");
    expect(mockPlayTimeoutChime).toHaveBeenCalled();
  });

  it("handles AI chat errors gracefully", () => {
    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    act(() => {
      result.current.startListening();
    });

    act(() => {
      mockChatOnError?.(new Error("AI connection timeout"));
    });

    expect(result.current.voiceState).toBe("SLEEPING");
    expect(onShowToast).toHaveBeenCalledWith(expect.stringContaining("AI connection timeout"));
  });

  it("falls back to browser SpeechSynthesis if /api/tts fails", async () => {
    (global.fetch as unknown as ReturnType<typeof vi.fn>).mockRejectedValueOnce(
      new Error("Network offline")
    );

    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    act(() => {
      result.current.startListening();
      result.current.sendToAgent("bật đèn");
    });

    await act(async () => {
      mockChatOnFinish?.({
        message: { parts: [{ type: "text", text: "Đã bật đèn qua fallback." }] },
      });
    });

    expect(result.current.voiceState).toBe("SPEAKING");
    expect(latestUtterance).not.toBeNull();
    expect(latestUtterance?.text).toBe("Đã bật đèn qua fallback.");

    act(() => {
      latestUtterance?.onend?.();
    });

    expect(result.current.voiceState).toBe("AWAKE");
    expect(result.current.countdown).toBe(8);
  });

  it("supports switching ttsVoice and selecting browser-native directly", async () => {
    const onShowToast = vi.fn();
    const { result } = renderHook(() => useAIVoice({ onShowToast }));

    act(() => {
      result.current.setTtsVoice("browser-native");
    });
    expect(result.current.ttsVoice).toBe("browser-native");

    act(() => {
      result.current.startListening();
      result.current.sendToAgent("bật đèn");
    });

    await act(async () => {
      mockChatOnFinish?.({
        message: { parts: [{ type: "text", text: "Chế độ browser native." }] },
      });
    });

    expect(result.current.voiceState).toBe("SPEAKING");
    expect(latestUtterance?.text).toBe("Chế độ browser native.");
    // Fetch should not have been called because browser-native was selected
    expect(global.fetch).not.toHaveBeenCalled();
  });
});
