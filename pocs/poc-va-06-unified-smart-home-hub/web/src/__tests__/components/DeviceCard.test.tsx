import { describe, it, expect, vi } from "vitest";
import { render, screen, fireEvent } from "@testing-library/react";
import React from "react";
import { DeviceCard } from "@/components/DeviceCard";
import type { DeviceStateResponse } from "@/lib/types";

describe("DeviceCard", () => {
  const onlineDevice: DeviceStateResponse = {
    device_id: "lamp-esp32-01",
    online: true,
    on: true,
    last_seen: new Date().toISOString(),
    pending_on: null,
  };

  const offlineDevice: DeviceStateResponse = {
    device_id: "lamp-esp32-02",
    online: false,
    on: null,
    last_seen: null,
    pending_on: null,
  };

  it("renders online device with lit lamp visual and active toggle", () => {
    const handleToggle = vi.fn();
    render(<DeviceCard device={onlineDevice} onToggle={handleToggle} isPending={false} />);

    expect(screen.getByText("lamp-esp32-01")).toBeInTheDocument();
    expect(screen.getByText("ONLINE")).toBeInTheDocument();
    expect(screen.getByText(/GPIO 5/i)).toBeInTheDocument();

    const checkbox = screen.getByRole("checkbox") as HTMLInputElement;
    expect(checkbox).toBeChecked();
    expect(checkbox).not.toBeDisabled();

    fireEvent.click(checkbox);
    expect(handleToggle).toHaveBeenCalledWith("lamp-esp32-01", true);
  });

  it("renders offline device with disabled toggle and unlit lamp", () => {
    const handleToggle = vi.fn();
    render(<DeviceCard device={offlineDevice} onToggle={handleToggle} isPending={false} />);

    expect(screen.getByText("lamp-esp32-02")).toBeInTheDocument();
    expect(screen.getByText("OFFLINE")).toBeInTheDocument();

    const checkbox = screen.getByRole("checkbox") as HTMLInputElement;
    expect(checkbox).not.toBeChecked();
    expect(checkbox).toBeDisabled();
  });

  it("displays pending state while toggle is applying", () => {
    render(<DeviceCard device={onlineDevice} onToggle={vi.fn()} isPending={true} />);
    const checkbox = screen.getByRole("checkbox") as HTMLInputElement;
    expect(checkbox).toBeDisabled();
  });
});
