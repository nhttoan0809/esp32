import { describe, it, expect, vi } from "vitest";
import { render, screen, fireEvent } from "@testing-library/react";
import React from "react";
import { VoiceWidget } from "@/components/VoiceWidget";

describe("VoiceWidget (English Only)", () => {
  it("renders inactive state when voice is turned off", () => {
    const handleToggle = vi.fn();
    render(
      <VoiceWidget
        voiceState="INACTIVE"
        transcript=""
        countdown={null}
        isListening={false}
        onToggleListening={handleToggle}
        supported={true}
      />
    );

    expect(screen.getByRole("heading", { name: "Voice Control" })).toBeInTheDocument();
    expect(screen.getByText("OFF")).toBeInTheDocument();

    const btn = screen.getByRole("button", { name: /turn on mic/i });
    fireEvent.click(btn);
    expect(handleToggle).toHaveBeenCalled();
  });

  it("renders sleeping state when listening for wake word", () => {
    render(
      <VoiceWidget
        voiceState="SLEEPING"
        transcript="something background"
        countdown={null}
        isListening={true}
        onToggleListening={vi.fn()}
        supported={true}
      />
    );

    expect(screen.getByText("SLEEPING")).toBeInTheDocument();
    expect(screen.getByText(/Say "Wake Up" or "Hey Lamp"/i)).toBeInTheDocument();
  });

  it("renders awake state with countdown timer and sound waves", () => {
    render(
      <VoiceWidget
        voiceState="AWAKE"
        transcript="turn on"
        countdown={6}
        isListening={true}
        onToggleListening={vi.fn()}
        supported={true}
      />
    );

    expect(screen.getByText("LISTENING...")).toBeInTheDocument();
    expect(screen.getByText("6s")).toBeInTheDocument();
    expect(screen.getByText(/Heard:/i)).toBeInTheDocument();
  });

  it("shows unsupported banner if Web Speech API is missing", () => {
    render(
      <VoiceWidget
        voiceState="INACTIVE"
        transcript=""
        countdown={null}
        isListening={false}
        onToggleListening={vi.fn()}
        supported={false}
      />
    );

    expect(screen.getByText(/Web Speech API not supported in this browser/i)).toBeInTheDocument();
  });
});
