import { describe, it, expect } from "vitest";
import {
  SetStateRequestSchema,
  SetStateResponseSchema,
  HelloMessageSchema,
  StateReportMessageSchema,
  DeviceStateResponseSchema,
} from "@/lib/types";

describe("Types & Validation Schemas", () => {
  it("validates SetStateRequest with on: boolean", () => {
    expect(SetStateRequestSchema.parse({ on: true })).toEqual({ on: true });
    expect(SetStateRequestSchema.parse({ on: false })).toEqual({ on: false });
    // Non-boolean should fail
    expect(() => SetStateRequestSchema.parse({ on: "INVALID" })).toThrow();
    // Extra fields forbidden (strict)
    expect(() => SetStateRequestSchema.parse({ on: true, extra: 123 })).toThrow();
  });

  it("validates SetStateResponse", () => {
    const valid = {
      device_id: "lamp-esp32-01",
      on: true,
      synced: true,
      command_id: "123e4567-e89b-12d3-a456-426614174000",
      warning: null,
    };
    expect(SetStateResponseSchema.parse(valid)).toEqual(valid);
  });

  it("validates HelloMessage", () => {
    const valid = {
      v: 1 as const,
      type: "hello" as const,
      device_id: "lamp-esp32-01",
      firmware: "smart-lamp-voice-1.0.0",
      reported: { on: false },
    };
    expect(HelloMessageSchema.parse(valid)).toEqual(valid);

    // Missing device_id should fail
    expect(() =>
      HelloMessageSchema.parse({
        v: 1,
        type: "hello",
        firmware: "1.0.0",
        reported: { on: true },
      })
    ).toThrow();

    // Invalid v should fail
    expect(() =>
      HelloMessageSchema.parse({
        v: 2,
        type: "hello",
        device_id: "lamp-esp32-01",
        firmware: "1.0.0",
        reported: { on: true },
      })
    ).toThrow();
  });

  it("validates StateReportMessage", () => {
    const valid = {
      v: 1 as const,
      type: "state_report" as const,
      command_id: "123e4567-e89b-12d3-a456-426614174000",
      device_id: "lamp-esp32-01",
      on: true,
    };
    expect(StateReportMessageSchema.parse(valid)).toEqual(valid);
  });

  it("validates DeviceStateResponse", () => {
    const response = {
      device_id: "lamp-esp32-01",
      online: true,
      on: true,
      last_seen: "2026-09-16T04:00:00.000Z",
      pending_on: null,
    };
    expect(DeviceStateResponseSchema.parse(response)).toEqual(response);
  });
});
