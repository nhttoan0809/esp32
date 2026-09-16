"use client";

import { useState, useEffect, useRef, useCallback } from "react";
import { useAudio } from "./useAudio";
import type { VoiceState } from "@/components/VoiceWidget";
import {
  cleanSpeechText,
  detectWakeWord,
  detectCommand,
} from "@/lib/voice_matcher";

interface UseVoiceOptions {
  onExecuteCommand: (desiredOn: boolean) => Promise<boolean>;
  onToggleCommand: () => Promise<boolean>;
  onShowToast: (message: string) => void;
}

interface SpeechRecognitionEvent {
  resultIndex: number;
  results: {
    [index: number]: {
      [index: number]: {
        transcript: string;
      };
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

export function useVoice({
  onExecuteCommand,
  onToggleCommand,
  onShowToast,
}: UseVoiceOptions) {
  const [voiceState, setVoiceState] = useState<VoiceState>("INACTIVE");
  const [transcript, setTranscript] = useState<string>("");
  const [liveTranscript, setLiveTranscript] = useState<string>("");
  const [countdown, setCountdown] = useState<number | null>(null);

  const [supported] = useState<boolean>(() => {
    if (typeof window === "undefined") return true;
    const SpeechRecognition =
      (window as unknown as { SpeechRecognition?: unknown }).SpeechRecognition ||
      (window as unknown as { webkitSpeechRecognition?: unknown }).webkitSpeechRecognition;
    return Boolean(SpeechRecognition);
  });

  const { resumeAudio, playWakeChime, playSuccessChime, playTimeoutChime } = useAudio();

  // Keep references to always use latest callbacks without recreating listeners
  const optionsRef = useRef({
    onExecuteCommand,
    onToggleCommand,
    onShowToast,
  });
  useEffect(() => {
    optionsRef.current = {
      onExecuteCommand,
      onToggleCommand,
      onShowToast,
    };
  }, [onExecuteCommand, onToggleCommand, onShowToast]);

  const recognitionRef = useRef<SpeechRecognitionInstance | null>(null);
  const countdownTimerRef = useRef<NodeJS.Timeout | null>(null);
  const restartTimerRef = useRef<NodeJS.Timeout | null>(null);
  const isListeningRef = useRef<boolean>(false);
  const voiceStateRef = useRef<VoiceState>("INACTIVE");
  const lastExecutionTimeRef = useRef<number>(0);
  const startSessionRef = useRef<() => void>(() => {});

  useEffect(() => {
    voiceStateRef.current = voiceState;
  }, [voiceState]);

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

  const enterAwakeState = useCallback(() => {
    clearCountdown();
    void resumeAudio();
    playWakeChime();
    setVoiceState("AWAKE");
    setCountdown(8);

    let secondsLeft = 8;
    countdownTimerRef.current = setInterval(() => {
      secondsLeft -= 1;
      if (secondsLeft <= 0) {
        clearCountdown();
        playTimeoutChime();
        setVoiceState("SLEEPING");
        optionsRef.current.onShowToast(
          "Voice command window timed out (8s). Listening for wake word..."
        );
      } else {
        setCountdown(secondsLeft);
      }
    }, 1000);
  }, [clearCountdown, playWakeChime, playTimeoutChime, resumeAudio]);

  const executeAction = useCallback(
    async (type: "on" | "off" | "toggle") => {
      // Set timestamp to lock duplicate triggers within 1.5s
      lastExecutionTimeRef.current = Date.now();
      clearCountdown();
      setVoiceState("EXECUTING");

      let ok = false;
      if (type === "toggle") {
        ok = await optionsRef.current.onToggleCommand();
      } else {
        ok = await optionsRef.current.onExecuteCommand(type === "on");
      }

      if (ok) {
        playSuccessChime();
        const message =
          type === "toggle"
            ? "Lamp state toggled via voice"
            : `Lamp turned ${type === "on" ? "ON" : "OFF"}`;
        optionsRef.current.onShowToast(message);
      } else {
        playTimeoutChime();
      }

      setVoiceState("SLEEPING");
    },
    [clearCountdown, playSuccessChime, playTimeoutChime]
  );

  const processText = useCallback(
    async (text: string) => {
      // Cooldown check: ignore speech echoes or trailing finals immediately after execution
      if (Date.now() - lastExecutionTimeRef.current < 1500) {
        return;
      }

      const cleaned = cleanSpeechText(text);
      if (!cleaned) return;

      setTranscript(cleaned);
      const currentState = voiceStateRef.current;

      // 1. If currently in SLEEPING state:
      if (currentState === "SLEEPING") {
        const wakeResult = detectWakeWord(cleaned);
        if (wakeResult.detected) {
          // If user uttered single-breath phrase like "Wake up turn on"
          if (wakeResult.remainder) {
            const remainderCmd = detectCommand(wakeResult.remainder, {
              allowSingleWords: true,
            });
            if (remainderCmd) {
              optionsRef.current.onShowToast("✨ Voice command recognized!");
              void resumeAudio();
              playWakeChime();
              void executeAction(remainderCmd);
              return;
            }
          }

          // Just wake word uttered -> enter AWAKE state
          optionsRef.current.onShowToast("✨ Assistant Awake! Listening for command...");
          enterAwakeState();
          return;
        }

        // Direct command without wake word (e.g. user says "Turn on", "Turn off", "Toggle")
        const directCmd = detectCommand(cleaned, { allowSingleWords: false });
        if (directCmd) {
          optionsRef.current.onShowToast("✨ Direct voice command recognized!");
          void resumeAudio();
          playWakeChime();
          void executeAction(directCmd);
          return;
        }
        return;
      }

      // 2. If currently in AWAKE state:
      if (currentState === "AWAKE") {
        const cmd = detectCommand(cleaned, { allowSingleWords: true });
        if (cmd) {
          void executeAction(cmd);
          return;
        }
      }
    },
    [enterAwakeState, executeAction, playWakeChime, resumeAudio]
  );

  // Set up the session start function through a ref to avoid circular dependency in ESLint
  useEffect(() => {
    startSessionRef.current = () => {
      if (typeof window === "undefined") return;
      const SpeechRecognition =
        (window as unknown as { SpeechRecognition: new () => SpeechRecognitionInstance }).SpeechRecognition ||
        (window as unknown as { webkitSpeechRecognition: new () => SpeechRecognitionInstance }).webkitSpeechRecognition;

      if (!SpeechRecognition) {
        optionsRef.current.onShowToast("Web Speech API not supported in this browser");
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
        rec.lang = "en-US";

        rec.onresult = (event: SpeechRecognitionEvent) => {
          let latest = "";
          let full = "";

          for (let i = event.resultIndex; i < event.results.length; i++) {
            const res = event.results[i];
            if (res?.[0]?.transcript) {
              latest += " " + res[0].transcript;
            }
          }
          for (let i = 0; i < event.results.length; i++) {
            const res = event.results[i];
            if (res?.[0]?.transcript) {
              full += " " + res[0].transcript;
            }
          }

          const textToProcess = (latest || full).trim();
          if (textToProcess) {
            setLiveTranscript(textToProcess);
            void processText(textToProcess);
          }
        };

        rec.onerror = (event: { error: string }) => {
          if (event.error === "not-allowed") {
            optionsRef.current.onShowToast(
              "Microphone permission denied. Please allow microphone in browser."
            );
            isListeningRef.current = false;
            setVoiceState("INACTIVE");
          } else if (event.error === "no-speech") {
            // Normal silence timeout in Chrome, onend will fire and restart
          } else {
            console.warn("Speech recognition notice:", event.error);
          }
        };

        rec.onend = () => {
          // Fresh instance restart debounced by 250ms
          if (isListeningRef.current) {
            clearRestartTimer();
            restartTimerRef.current = setTimeout(() => {
              if (isListeningRef.current) {
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
  }, [clearRestartTimer, processText]);

  const startListening = useCallback(() => {
    void resumeAudio();
    clearRestartTimer();
    isListeningRef.current = true;
    setVoiceState("SLEEPING");

    optionsRef.current.onShowToast(
      '🎙️ Always-Listening active! Say "Wake Up", "Turn on", or "Turn off"'
    );

    startSessionRef.current();
  }, [clearRestartTimer, resumeAudio]);

  const stopListening = useCallback(() => {
    isListeningRef.current = false;
    clearRestartTimer();
    clearCountdown();

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
  }, [clearCountdown, clearRestartTimer]);

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
    enterAwakeState();
    optionsRef.current.onShowToast("⚡ Activated! Say command or click test.");
  }, [enterAwakeState, startListening]);

  const simulateCommand = useCallback(
    async (action: "on" | "off" | "toggle") => {
      void resumeAudio();
      await executeAction(action);
    },
    [executeAction, resumeAudio]
  );

  useEffect(() => {
    return () => {
      clearCountdown();
      clearRestartTimer();
      if (recognitionRef.current) {
        try {
          recognitionRef.current.stop();
        } catch {
          // ignore
        }
      }
    };
  }, [clearCountdown, clearRestartTimer]);

  return {
    voiceState,
    transcript,
    liveTranscript,
    countdown,
    supported,
    isListening: voiceState !== "INACTIVE",
    toggleListening,
    triggerWakeUp,
    simulateCommand,
  };
}
