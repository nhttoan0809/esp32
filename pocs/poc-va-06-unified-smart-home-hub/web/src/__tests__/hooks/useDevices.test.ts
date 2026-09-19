import { describe, it, expect, vi, beforeEach, afterEach } from "vitest";
import { renderHook, act } from "@testing-library/react";
import { useDevices } from "@/hooks/useDevices";
import type { DeviceStateResponse } from "@/lib/types";

describe("useDevices hook", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
  });

  afterEach(() => {
    vi.restoreAllMocks();
  });

  it("fetches device list successfully", async () => {
    const mockDevices: DeviceStateResponse[] = [
      {
        device_id: "lamp-esp32-01",
        online: true,
        on: true,
        last_seen: new Date().toISOString(),
        pending_on: null,
      },
    ];

    global.fetch = vi.fn().mockResolvedValue({
      ok: true,
      status: 200,
      json: async () => mockDevices,
    } as Response);

    const onUnauthorized = vi.fn();
    const { result } = renderHook(() => useDevices("test-api-key", onUnauthorized));

    await act(async () => {
      await result.current.refreshDevices();
    });

    expect(result.current.devices).toEqual(mockDevices);
    expect(onUnauthorized).not.toHaveBeenCalled();
  });

  it("triggers onUnauthorized when API returns 401", async () => {
    global.fetch = vi.fn().mockResolvedValue({
      ok: false,
      status: 401,
      json: async () => ({ detail: "invalid_dashboard_api_key" }),
    } as Response);

    const onUnauthorized = vi.fn();
    const { result } = renderHook(() => useDevices("wrong-key", onUnauthorized));

    await act(async () => {
      await result.current.refreshDevices();
    });

    expect(onUnauthorized).toHaveBeenCalled();
  });

  it("sends PUT request on toggleDevice", async () => {
    global.fetch = vi.fn().mockResolvedValue({
      ok: true,
      status: 200,
      json: async () => ({
        device_id: "lamp-esp32-01",
        on: false,
        synced: true,
        command_id: "test-uuid",
        warning: null,
      }),
    } as Response);

    const onUnauthorized = vi.fn();
    const { result } = renderHook(() => useDevices("test-api-key", onUnauthorized));

    await act(async () => {
      await result.current.toggleDevice("lamp-esp32-01", true); // current is true, so desired is false
    });

    expect(global.fetch).toHaveBeenCalledWith(
      "/api/devices/lamp-esp32-01/state",
      expect.objectContaining({
        method: "PUT",
        headers: expect.objectContaining({
          "Content-Type": "application/json",
          "X-API-Key": "test-api-key",
        }),
        body: JSON.stringify({ on: false }),
      })
    );
  });
});
