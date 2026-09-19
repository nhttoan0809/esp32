import { describe, it, expect } from "vitest";
import { timingSafeEqualString, verifyDashboardApiKey, verifyDeviceToken } from "@/lib/auth";

describe("Auth Utilities", () => {
  it("timingSafeEqualString correctly compares strings", () => {
    expect(timingSafeEqualString("secret-key", "secret-key")).toBe(true);
    expect(timingSafeEqualString("secret-key", "wrong-key")).toBe(false);
    expect(timingSafeEqualString("short", "longer-string")).toBe(false);
  });

  it("verifyDashboardApiKey validates against configured key", () => {
    const validKey = "test-dashboard-key";
    expect(verifyDashboardApiKey("test-dashboard-key", validKey)).toBe(true);
    expect(verifyDashboardApiKey("wrong-key", validKey)).toBe(false);
    expect(verifyDashboardApiKey(undefined, validKey)).toBe(false);
  });

  it("verifyDeviceToken validates device token mapping", () => {
    const tokens = { "lamp-01": "token-123" };
    expect(verifyDeviceToken("lamp-01", "token-123", tokens)).toBe(true);
    expect(verifyDeviceToken("lamp-01", "wrong-token", tokens)).toBe(false);
    expect(verifyDeviceToken("unknown-device", "token-123", tokens)).toBe(false);
  });
});
