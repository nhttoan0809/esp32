"use client";

import { useState, useEffect, useRef, useCallback, useMemo } from "react";
import { useChat } from "@ai-sdk/react";
import { DefaultChatTransport, type UIMessage } from "ai";
import { useAudio } from "./useAudio";
import { cleanSpeechText, detectWakeWord, detectCommand } from "@/lib/voice_matcher";
import type { ModelKey } from "@/lib/ai/ai-config";

export type AIVoiceState =
  | "INACTIVE"
  | "SLEEPING"
  | "AWAKE"
  | "PROCESSING"
  | "SPEAKING";

export type VoiceLanguage = "vi-VN" | "en-US";

interface SpeechRecognitionEvent {
  resultIndex: number;
  results: {
    [index: number]: {
      [index: number]: { transcript: string };
      isFinal?: boolean;
    };
    length: number;
  };
}

interface SpeechRecognitionInstance {
  continuous: boolean;
  interimResults: boolean;
  lang: string;
  onresult: ((event: SpeechRecognitionEvent) => void) | null;
  onerror: ((event: { error: string }) => void) | null;
  onend: (() => void) | null;
  start: () => void;
  stop: () => void;
  abort: () => void;
}

interface UseAIVoiceOptions {
  onShowToast: (message: string) => void;
  onDevicesChanged?: () => void;
}

export function useAIVoice({ onShowToast, onDevicesChanged }: UseAIVoiceOptions) {
  const [voiceState, setVoiceState] = useState<AIVoiceState>("INACTIVE");
  const [liveTranscript, setLiveTranscript] = useState<string>("");
  const [countdown, setCountdown] = useState<number | null>(null);
  const [modelKey, setModelKey] = useState<ModelKey>("sglang-qwen38-27b");
  const [ttsEnabled, setTtsEnabled] = useState<boolean>(true);
  const [language, setLanguageState] = useState<VoiceLanguage>("vi-VN");

  const [supported] = useState<boolean>(() => {
    if (typeof window === "undefined") return true;
    const SR =
      (window as unknown as { SpeechRecognition?: unknown }).SpeechRecognition ||
      (window as unknown as { webkitSpeechRecognition?: unknown })
        .webkitSpeechRecognition;
    return Boolean(SR);
  });

  const { resumeAudio, playWakeChime, playSuccessChime, playTimeoutChime } =
    useAudio();

  // Refs for stable access across callbacks
  const optionsRef = useRef({ onShowToast, onDevicesChanged });
  useEffect(() => {
    optionsRef.current = { onShowToast, onDevicesChanged };
  }, [onShowToast, onDevicesChanged]);

  const voiceStateRef = useRef<AIVoiceState>("INACTIVE");
  useEffect(() => {
    voiceStateRef.current = voiceState;
  }, [voiceState]);

  const languageRef = useRef<VoiceLanguage>("vi-VN");
  useEffect(() => {
    languageRef.current = language;
  }, [language]);

  const recognitionRef = useRef<SpeechRecognitionInstance | null>(null);
  const countdownTimerRef = useRef<NodeJS.Timeout | null>(null);
  const restartTimerRef = useRef<NodeJS.Timeout | null>(null);
  const speechDebounceTimerRef = useRef<NodeJS.Timeout | null>(null);
  const ignoreUntilRef = useRef<number>(0);
  const isListeningRef = useRef<boolean>(false);
  const startSessionRef = useRef<() => void>(() => {});

  // --- Timers & Cleansers ---
  const clearCountdown = useCallback(() => {
    if (countdownTimerRef.current) {
      clearInterval(countdownTimerRef.current);
      countdownTimerRef.current = null;
    }
    setCountdown(null);
  }, []);

  const clearRestartTimer = useCallback(() => {
    if (restartTimerRef.current) {
      clearTimeout(restartTimerRef.current);
      restartTimerRef.current = null;
    }
  }, []);

  const clearSpeechDebounceTimer = useCallback(() => {
    if (speechDebounceTimerRef.current) {
      clearTimeout(speechDebounceTimerRef.current);
      speechDebounceTimerRef.current = null;
    }
  }, []);

  // --- Transition to AWAKE state with Countdown ---
  const enterAwakeState = useCallback(
    (options: { playChime?: boolean } = { playChime: true }) => {
      clearCountdown();
      clearSpeechDebounceTimer();
      setLiveTranscript("");
      // Ignore trailing chunks from current utterance for 800ms
      ignoreUntilRef.current = Date.now() + 800;

      void resumeAudio();
      if (options.playChime) {
        playWakeChime();
      }
      setVoiceState("AWAKE");
      setCountdown(8);

      let secondsLeft = 8;
      countdownTimerRef.current = setInterval(() => {
        secondsLeft -= 1;
        if (secondsLeft <= 0) {
          clearCountdown();
          clearSpeechDebounceTimer();
          playTimeoutChime();
          setVoiceState("SLEEPING");
          setLiveTranscript("");
          optionsRef.current.onShowToast(
            languageRef.current === "vi-VN"
              ? "Đã hết thời gian chờ lệnh (8s). Đang lắng nghe từ khoá..."
              : "Voice command window timed out (8s). Listening for wake word..."
          );
        } else {
          setCountdown(secondsLeft);
        }
      }, 1000);
    },
    [clearCountdown, clearSpeechDebounceTimer, playWakeChime, playTimeoutChime, resumeAudio]
  );

  // --- TTS: Speak agent response ---
  const speakText = useCallback(
    (text: string) => {
      const finishSpeech = () => {
        // Cooldown period (1.2s) to avoid computer speaker echo picking up on mic
        ignoreUntilRef.current = Date.now() + 1200;
        setLiveTranscript("");
        if (isListeningRef.current) {
          startSessionRef.current();
          // After model response is completely returned and spoken, start countdown!
          enterAwakeState({ playChime: false });
          optionsRef.current.onShowToast(
            languageRef.current === "vi-VN"
              ? "Mời bạn nói lệnh tiếp theo (8s)..."
              : "Listening for follow-up command (8s)..."
          );
        } else {
          setVoiceState("INACTIVE");
        }
      };

      if (!ttsEnabled || !text || typeof window === "undefined") {
        finishSpeech();
        return;
      }
      const synth = window.speechSynthesis;
      if (!synth) {
        finishSpeech();
        return;
      }

      synth.cancel();

      const utterance = new SpeechSynthesisUtterance(text);
      const hasVietnamese = /[àáảãạăắằẳẵặâấầẩẫậèéẻẽẹêếềểễệìíỉĩịòóỏõọôốồổỗộơớờởỡợùúủũụưứừửữựỳýỷỹỵđ]/i.test(
        text
      );
      utterance.lang = hasVietnamese || languageRef.current === "vi-VN" ? "vi-VN" : "en-US";
      utterance.rate = 1.05;
      utterance.pitch = 1.0;

      utterance.onend = finishSpeech;
      utterance.onerror = finishSpeech;

      setVoiceState("SPEAKING");
      synth.speak(utterance);
    },
    [enterAwakeState, ttsEnabled]
  );

  const onFinishRef = useRef<(options: { message: UIMessage }) => void>(() => {});
  const onErrorRef = useRef<(error: Error) => void>(() => {});

  useEffect(() => {
    onFinishRef.current = ({ message }) => {
      const texts: string[] = [];
      if (message && Array.isArray(message.parts)) {
        for (const part of message.parts) {
          if (part.type === "text" && part.text) {
            texts.push(part.text);
          }
        }
      }
      const text = texts.join(" ").trim();
      if (text) {
        playSuccessChime();
        speakText(text);
        optionsRef.current.onDevicesChanged?.();
      } else {
        // No spoken text returned, model response is fully complete -> start countdown!
        ignoreUntilRef.current = Date.now() + 1000;
        setLiveTranscript("");
        if (isListeningRef.current) {
          startSessionRef.current();
          enterAwakeState({ playChime: false });
          optionsRef.current.onShowToast(
            languageRef.current === "vi-VN"
              ? "Mời bạn nói lệnh tiếp theo (8s)..."
              : "Listening for follow-up command (8s)..."
          );
        } else {
          setVoiceState("INACTIVE");
        }
      }
    };
  }, [enterAwakeState, playSuccessChime, speakText]);

  useEffect(() => {
    onErrorRef.current = (error: Error) => {
      console.error("AI chat error:", error);
      optionsRef.current.onShowToast(`AI error: ${error.message || "Request failed"}`);
      ignoreUntilRef.current = Date.now() + 1000;
      setVoiceState("SLEEPING");
      setLiveTranscript("");
      if (isListeningRef.current) {
        startSessionRef.current();
      }
    };
  }, []);

  // --- AI SDK useChat with dynamic transport ---
  const transport = useMemo(
    () =>
      new DefaultChatTransport({
        api: "/api/chat",
        body: { modelKey },
      }),
    [modelKey]
  );

  const { messages, sendMessage, status } = useChat({
    transport,
    onFinish: (options) => onFinishRef.current(options),
    onError: (err) => onErrorRef.current(err),
  });

  // Extract latest assistant text for display
  let lastAssistantText = "";
  for (let i = messages.length - 1; i >= 0; i--) {
    const msg = messages[i];
    if (msg && msg.role === "assistant") {
      const texts: string[] = [];
      for (const part of msg.parts) {
        if (part.type === "text" && part.text) {
          texts.push(part.text);
        }
      }
      lastAssistantText = texts.join(" ").trim();
      break;
    }
  }

  // --- Send command to AI agent ---
  const sendToAgent = useCallback(
    (text: string) => {
      clearCountdown();
      clearSpeechDebounceTimer();
      setVoiceState("PROCESSING");
      setLiveTranscript(text);
      optionsRef.current.onShowToast(`🤖 Processing: "${text}"`);

      // Pause speech recognition while AI is thinking/speaking to eliminate feedback loop
      if (recognitionRef.current) {
        try {
          recognitionRef.current.stop();
        } catch {
          // ignore
        }
      }

      sendMessage({ text });
    },
    [clearCountdown, clearSpeechDebounceTimer, sendMessage]
  );

  // --- Reset speech debounce timer ---
  const resetSpeechDebounceTimer = useCallback(
    (command: string) => {
      clearSpeechDebounceTimer();
      speechDebounceTimerRef.current = setTimeout(() => {
        if (voiceStateRef.current === "AWAKE" && command.trim().length >= 2) {
          sendToAgent(command.trim());
        }
      }, 1200);
    },
    [clearSpeechDebounceTimer, sendToAgent]
  );

  // --- Process speech input ---
  const handleSpeechResult = useCallback(
    (rawText: string, isFinal: boolean) => {
      const currentState = voiceStateRef.current;
      if (
        currentState === "INACTIVE" ||
        currentState === "PROCESSING" ||
        currentState === "SPEAKING"
      ) {
        return;
      }

      const cleaned = cleanSpeechText(rawText);
      if (!cleaned) return;

      // 1. In SLEEPING state: listen for Wake Word or Direct Command
      if (currentState === "SLEEPING") {
        if (Date.now() < ignoreUntilRef.current) {
          return;
        }

        const wakeResult = detectWakeWord(cleaned);
        if (wakeResult.detected) {
          if (wakeResult.remainder && wakeResult.remainder.length >= 3) {
            // Single-breath command (e.g. "Wake up turn on the lamp" or "Trợ lý ơi bật đèn lên")
            setLiveTranscript(cleaned);
            if (isFinal) {
              clearSpeechDebounceTimer();
              void resumeAudio();
              playWakeChime();
              sendToAgent(wakeResult.remainder);
            } else {
              resetSpeechDebounceTimer(wakeResult.remainder);
            }
            return;
          }

          // Pure wake word! ("Wake up", "Trợ lý ơi", "Hey lamp")
          // Enter AWAKE state with wake chime and start countdown!
          optionsRef.current.onShowToast(
            languageRef.current === "vi-VN"
              ? "✨ Trợ lý đã thức! Mời bạn nói lệnh..."
              : "✨ Assistant Awake! Say your command..."
          );
          enterAwakeState({ playChime: true });
          return;
        }

        // Direct command without wake word (only execute when finalized)
        const directCmd = detectCommand(cleaned);
        if (directCmd && isFinal) {
          void resumeAudio();
          playWakeChime();
          sendToAgent(cleaned);
        }
        return;
      }

      // 2. In AWAKE state: user is speaking the command or a sleep word
      if (currentState === "AWAKE") {
        const wakeResult = detectWakeWord(cleaned);
        let command = cleaned;

        if (wakeResult.detected) {
          if (wakeResult.remainder && wakeResult.remainder.length >= 2) {
            // User prefixed wake word again: e.g. "Trợ lý ơi bật đèn" -> strip wake word
            command = wakeResult.remainder;
          } else {
            // Pure wake word repeated
            if (Date.now() < ignoreUntilRef.current) {
              // Trailing chunk from the initial wake word -> ignore
              return;
            }
            // Re-uttered wake word -> refresh 8s countdown
            enterAwakeState({ playChime: false });
            return;
          }
        }

        // Check for quick sleep/stop phrases in AWAKE state
        const SLEEP_PHRASES = [
          "đi ngủ đi",
          "ngủ đi",
          "tắt mic",
          "tat mic",
          "dừng lại",
          "dung lai",
          "tạm biệt",
          "tam biet",
          "bye",
          "goodbye",
          "sleep",
          "stop",
          "cancel",
        ];
        if (SLEEP_PHRASES.some((p) => command === p || command.startsWith(p))) {
          clearCountdown();
          clearSpeechDebounceTimer();
          playTimeoutChime();
          setVoiceState("SLEEPING");
          setLiveTranscript("");
          optionsRef.current.onShowToast(
            languageRef.current === "vi-VN"
              ? "Trợ lý đã chuyển sang chế độ ngủ. Nói 'Trợ lý ơi' khi cần nhé!"
              : "Assistant is now sleeping. Say 'Wake Up' when needed!"
          );
          return;
        }

        if (command.length < 2) return;

        // Show live transcript as user speaks
        setLiveTranscript(command);

        if (isFinal) {
          clearSpeechDebounceTimer();
          sendToAgent(command);
        } else {
          // Interim result: debounce 1200ms of silence
          resetSpeechDebounceTimer(command);
        }
      }
    },
    [
      clearCountdown,
      clearSpeechDebounceTimer,
      enterAwakeState,
      playTimeoutChime,
      playWakeChime,
      resetSpeechDebounceTimer,
      resumeAudio,
      sendToAgent,
    ]
  );

  // --- Speech Recognition session ---
  useEffect(() => {
    startSessionRef.current = () => {
      if (typeof window === "undefined") return;
      const SpeechRecognition =
        (
          window as unknown as {
            SpeechRecognition: new () => SpeechRecognitionInstance;
          }
        ).SpeechRecognition ||
        (
          window as unknown as {
            webkitSpeechRecognition: new () => SpeechRecognitionInstance;
          }
        ).webkitSpeechRecognition;

      if (!SpeechRecognition) {
        optionsRef.current.onShowToast(
          "Web Speech API not supported in this browser"
        );
        return;
      }

      if (recognitionRef.current) {
        try {
          recognitionRef.current.stop();
        } catch {
          // ignore
        }
        recognitionRef.current = null;
      }

      try {
        const rec = new SpeechRecognition();
        rec.continuous = true;
        rec.interimResults = true;
        rec.lang = languageRef.current;

        rec.onresult = (event: SpeechRecognitionEvent) => {
          let latest = "";
          let isFinal = false;

          for (let i = event.resultIndex; i < event.results.length; i++) {
            const res = event.results[i];
            if (res?.[0]?.transcript) {
              latest += " " + res[0].transcript;
              if (res.isFinal) {
                isFinal = true;
              }
            }
          }

          const text = latest.trim();
          if (text) {
            handleSpeechResult(text, isFinal);
          }
        };

        rec.onerror = (event: { error: string }) => {
          if (event.error === "not-allowed") {
            optionsRef.current.onShowToast(
              "Microphone permission denied. Please allow microphone in browser."
            );
            isListeningRef.current = false;
            setVoiceState("INACTIVE");
          } else if (event.error !== "no-speech") {
            console.warn("Speech recognition notice:", event.error);
          }
        };

        rec.onend = () => {
          // Auto-restart recognition only if listening and currently in SLEEPING or AWAKE state
          if (
            isListeningRef.current &&
            (voiceStateRef.current === "SLEEPING" || voiceStateRef.current === "AWAKE")
          ) {
            clearRestartTimer();
            restartTimerRef.current = setTimeout(() => {
              if (
                isListeningRef.current &&
                (voiceStateRef.current === "SLEEPING" || voiceStateRef.current === "AWAKE")
              ) {
                startSessionRef.current();
              }
            }, 250);
          }
        };

        rec.start();
        recognitionRef.current = rec;
      } catch (err) {
        console.warn("SpeechRecognition start failed:", err);
      }
    };
  }, [clearRestartTimer, handleSpeechResult]);

  // --- Public controls ---
  const setLanguage = useCallback((newLang: VoiceLanguage) => {
    setLanguageState(newLang);
    languageRef.current = newLang;
    if (recognitionRef.current) {
      try {
        recognitionRef.current.lang = newLang;
      } catch {
        // ignore
      }
    }
    optionsRef.current.onShowToast(
      newLang === "vi-VN"
        ? "🇻🇳 Đã chuyển sang Tiếng Việt (vi-VN)"
        : "🇺🇸 Switched to English (en-US)"
    );
  }, []);

  const startListening = useCallback(() => {
    void resumeAudio();
    clearRestartTimer();
    clearSpeechDebounceTimer();
    isListeningRef.current = true;
    setVoiceState("SLEEPING");
    optionsRef.current.onShowToast(
      languageRef.current === "vi-VN"
        ? '🎤 Đã bật AI Voice Control! Nói "Trợ lý ơi" hoặc "Wake Up" để bắt đầu'
        : '🎤 AI Voice Control active! Say "Wake Up" to start'
    );
    startSessionRef.current();
  }, [clearRestartTimer, clearSpeechDebounceTimer, resumeAudio]);

  const stopListening = useCallback(() => {
    isListeningRef.current = false;
    clearRestartTimer();
    clearCountdown();
    clearSpeechDebounceTimer();

    if (typeof window !== "undefined") {
      window.speechSynthesis?.cancel();
    }

    if (recognitionRef.current) {
      try {
        recognitionRef.current.stop();
      } catch {
        // ignore
      }
      recognitionRef.current = null;
    }

    setVoiceState("INACTIVE");
    setLiveTranscript("");
    optionsRef.current.onShowToast("Voice control stopped");
  }, [clearCountdown, clearRestartTimer, clearSpeechDebounceTimer]);

  const toggleListening = useCallback(() => {
    if (isListeningRef.current) {
      stopListening();
    } else {
      startListening();
    }
  }, [startListening, stopListening]);

  const triggerWakeUp = useCallback(() => {
    if (voiceStateRef.current === "INACTIVE") {
      startListening();
    }
    enterAwakeState({ playChime: true });
    optionsRef.current.onShowToast(
      languageRef.current === "vi-VN"
        ? "⚡ Đã kích hoạt! Mời bạn nói lệnh."
        : "⚡ Activated! Say your command."
    );
  }, [enterAwakeState, startListening]);

  // Cleanup on unmount
  useEffect(() => {
    return () => {
      clearCountdown();
      clearRestartTimer();
      clearSpeechDebounceTimer();
      if (typeof window !== "undefined") {
        window.speechSynthesis?.cancel();
      }
      if (recognitionRef.current) {
        try {
          recognitionRef.current.stop();
        } catch {
          // ignore
        }
      }
    };
  }, [clearCountdown, clearRestartTimer, clearSpeechDebounceTimer]);

  return {
    voiceState,
    liveTranscript,
    countdown,
    supported,
    isListening: voiceState !== "INACTIVE",
    messages,
    lastAssistantText,
    status,
    modelKey,
    setModelKey,
    ttsEnabled,
    setTtsEnabled,
    language,
    setLanguage,
    startListening,
    stopListening,
    toggleListening,
    triggerWakeUp,
    sendToAgent,
  };
}
