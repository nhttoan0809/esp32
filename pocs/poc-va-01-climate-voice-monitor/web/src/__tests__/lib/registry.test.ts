import { describe, it, expect, vi } from "vitest";
import {
  DeviceRegistry,
  DeviceOfflineError,
} from "@/lib/registry";

describe("DeviceRegistry", () => {
  it("initializes device with offline state and null on", async () => {
    const registry = new DeviceRegistry(2.0);
    const snap = await registry.snapshot("lamp-esp32-01");
    expect(snap.deviceId).toBe("lamp-esp32-01");
    expect(snap.online).toBe(false);
    expect(snap.on).toBeNull();
    expect(snap.pendingOn).toBeNull();
  });

  it("lists all known device ids", async () => {
    const registry = new DeviceRegistry(2.0);
    await registry.queueState("lamp-01", true);
    await registry.queueState("lamp-02", false);
    const ids = await registry.allDeviceIds();
    expect(ids.size).toBe(2);
    expect(ids.has("lamp-01")).toBe(true);
    expect(ids.has("lamp-02")).toBe(true);
  });

  it("throws DeviceOfflineError when trying to send state command to offline device", async () => {
    const registry = new DeviceRegistry(0.5);
    await expect(registry.sendStateCommand("lamp-01", true)).rejects.toThrow(
      DeviceOfflineError
    );

    // After queueState, pendingOn should be saved
    await registry.queueState("lamp-01", true);
    const snap = await registry.snapshot("lamp-01");
    expect(snap.pendingOn).toBe(true);
  });

  it("attaches session, marks ready, and detaches cleanly", async () => {
    const registry = new DeviceRegistry(2.0);
    const mockWs = {
      send: vi.fn(),
      close: vi.fn(),
    };

    const session = await registry.attach("lamp-01", mockWs as never);
    expect(session.online).toBe(false);

    const readyOk = await registry.markReady(session, false);
    expect(readyOk).toBe(true);
    expect(session.online).toBe(true);

    const snap = await registry.snapshot("lamp-01");
    expect(snap.online).toBe(true);
    expect(snap.on).toBe(false);

    await registry.detach(session);
    const snapAfter = await registry.snapshot("lamp-01");
    expect(snapAfter.online).toBe(false);
  });

  it("sends state command and resolves when state report is recorded", async () => {
    const registry = new DeviceRegistry(2.0);
    let sentPayload = "";
    const mockWs = {
      send: vi.fn().mockImplementation((data: string) => {
        sentPayload = data;
      }),
      close: vi.fn(),
    };

    const session = await registry.attach("lamp-01", mockWs as never);
    await registry.markReady(session, false);

    // Initiate command
    const commandPromise = registry.sendStateCommand("lamp-01", true);

    // Verify WebSocket payload sent to ESP32
    expect(mockWs.send).toHaveBeenCalled();
    const parsed = JSON.parse(sentPayload);
    expect(parsed.v).toBe(1);
    expect(parsed.type).toBe("set_state");
    expect(parsed.device_id).toBe("lamp-01");
    expect(parsed.on).toBe(true);
    expect(parsed.command_id).toBeDefined();

    // Simulate ESP32 sending state_report back
    const matched = await registry.recordStateReport(
      session,
      parsed.command_id,
      true
    );
    expect(matched).toBe(true);

    const { commandId, confirmedOn } = await commandPromise;
    expect(commandId).toBe(parsed.command_id);
    expect(confirmedOn).toBe(true);

    const snap = await registry.snapshot("lamp-01");
    expect(snap.on).toBe(true);
  });

  it("reconciles pending state upon reconnection", async () => {
    const registry = new DeviceRegistry(2.0);
    await registry.queueState("lamp-01", true);

    let sentPayload = "";
    const mockWs = {
      send: vi.fn().mockImplementation((data: string) => {
        sentPayload = data;
      }),
      close: vi.fn(),
    };

    const session = await registry.attach("lamp-01", mockWs as never);
    await registry.markReady(session, false); // device currently reported OFF

    const reconciled = await registry.reconcilePending(session);
    expect(reconciled).toBe(true);

    const parsed = JSON.parse(sentPayload);
    expect(parsed.type).toBe("set_state");
    expect(parsed.on).toBe(true);
  });
});
