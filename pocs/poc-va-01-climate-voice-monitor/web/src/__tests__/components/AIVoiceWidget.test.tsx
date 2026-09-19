import { describe, it, expect, vi } from "vitest";
import { render, screen, fireEvent } from "@testing-library/react";
import React from "react";
import { AIVoiceWidget } from "@/components/AIVoiceWidget";

describe("AIVoiceWidget", () => {
  const defaultProps = {
    voiceState: "INACTIVE" as const,
    liveTranscript: "",
    countdown: null,
    isListening: false,
    supported: true,
    messages: [],
    lastAssistantText: "",
    status: "ready",
    modelKey: "sglang-qwen38-27b" as const,
    ttsEnabled: true,
    ttsVoice: "auto" as const,
    language: "vi-VN" as const,
    onSetLanguage: vi.fn(),
    onSetModelKey: vi.fn(),
    onSetTtsEnabled: vi.fn(),
    onSetTtsVoice: vi.fn(),
    onToggleListening: vi.fn(),
    onTriggerWakeUp: vi.fn(),
  };

  it("renders inactive state with mic toggle button", () => {
    render(<AIVoiceWidget {...defaultProps} />);

    expect(screen.getByRole("heading", { name: "AI Voice Control" })).toBeInTheDocument();
    expect(screen.getByText("OFF")).toBeInTheDocument();

    const turnOnBtn = screen.getByRole("button", { name: /turn on mic/i });
    fireEvent.click(turnOnBtn);
    expect(defaultProps.onToggleListening).toHaveBeenCalled();
  });

  it("renders sleeping state and wake up button", () => {
    render(
      <AIVoiceWidget
        {...defaultProps}
        voiceState="SLEEPING"
        isListening={true}
      />
    );

    expect(screen.getByText("SLEEPING")).toBeInTheDocument();
    expect(screen.getByText(/Nói "Trợ lý ơi"/i)).toBeInTheDocument();

    const wakeBtn = screen.getByRole("button", { name: /wake/i });
    fireEvent.click(wakeBtn);
    expect(defaultProps.onTriggerWakeUp).toHaveBeenCalled();
  });

  it("renders awake state with countdown timer", () => {
    render(
      <AIVoiceWidget
        {...defaultProps}
        voiceState="AWAKE"
        isListening={true}
        countdown={5}
        liveTranscript="bật đèn phòng"
      />
    );

    expect(screen.getByText("LISTENING...")).toBeInTheDocument();
    expect(screen.getByText("5s")).toBeInTheDocument();
    expect(screen.getByText(/bật đèn phòng/i)).toBeInTheDocument();
  });

  it("renders processing state when AI is thinking", () => {
    render(
      <AIVoiceWidget
        {...defaultProps}
        voiceState="PROCESSING"
        isListening={true}
      />
    );

    expect(screen.getByText("AI THINKING...")).toBeInTheDocument();
    expect(screen.getByText(/AI đang xử lý|AI is understanding/i)).toBeInTheDocument();
  });

  it("allows selecting a different model from the dropdown", () => {
    render(<AIVoiceWidget {...defaultProps} />);

    const select = screen.getByLabelText(/model/i);
    fireEvent.change(select, { target: { value: "ollama-qwen3-4b" } });
    expect(defaultProps.onSetModelKey).toHaveBeenCalledWith("ollama-qwen3-4b");
  });

  it("allows selecting a different TTS voice from dropdown", () => {
    render(<AIVoiceWidget {...defaultProps} ttsEnabled={true} />);

    const voiceSelect = screen.getByLabelText(/select tts voice/i);
    fireEvent.change(voiceSelect, { target: { value: "vi-VN-NamMinhNeural" } });
    expect(defaultProps.onSetTtsVoice).toHaveBeenCalledWith("vi-VN-NamMinhNeural");
  });

  it("toggles speech recognition language", () => {
    render(<AIVoiceWidget {...defaultProps} language="vi-VN" />);

    const langBtn = screen.getByRole("button", { name: /toggle speech language/i });
    expect(langBtn).toHaveTextContent("🇻🇳 Tiếng Việt");
    fireEvent.click(langBtn);
    expect(defaultProps.onSetLanguage).toHaveBeenCalledWith("en-US");
  });

  it("toggles TTS speech output", () => {
    render(<AIVoiceWidget {...defaultProps} ttsEnabled={true} />);

    const ttsBtn = screen.getByRole("button", { name: /toggle voice response/i });
    fireEvent.click(ttsBtn);
    expect(defaultProps.onSetTtsEnabled).toHaveBeenCalledWith(false);
  });

  it("shows unsupported banner if Web Speech API is not available", () => {
    render(<AIVoiceWidget {...defaultProps} supported={false} />);

    expect(screen.getByText(/Web Speech API not supported/i)).toBeInTheDocument();
  });
});
