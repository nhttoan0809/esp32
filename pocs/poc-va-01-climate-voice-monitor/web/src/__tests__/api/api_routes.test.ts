import { describe, it, expect } from "vitest";
import { NextRequest } from "next/server";
import { GET as healthGet } from "@/app/api/health/route";
import { POST as verifyPost } from "@/app/api/auth/verify/route";
import { GET as devicesGet } from "@/app/api/devices/route";
import { GET as deviceDetailGet } from "@/app/api/devices/[deviceId]/route";
import { PUT as deviceStatePut } from "@/app/api/devices/[deviceId]/state/route";
import { getSettings } from "@/lib/config";

describe("API Routes", () => {
  const settings = getSettings();
  const validKey = settings.dashboardApiKey;

  it("GET /api/health returns 200 { status: 'ok' }", async () => {
    const res = await healthGet();
    expect(res.status).toBe(200);
    const data = await res.json();
    expect(data).toEqual({ status: "ok" });
  });

  it("POST /api/auth/verify verifies dashboard key", async () => {
    const validReq = new NextRequest("http://localhost:8000/api/auth/verify", {
      method: "POST",
      headers: { "x-api-key": validKey },
    });
    const validRes = await verifyPost(validReq);
    expect(validRes.status).toBe(200);
    const validData = await validRes.json();
    expect(validData).toEqual({ status: "ok", authenticated: true });

    const invalidReq = new NextRequest("http://localhost:8000/api/auth/verify", {
      method: "POST",
      headers: { "x-api-key": "wrong-key" },
    });
    const invalidRes = await verifyPost(invalidReq);
    expect(invalidRes.status).toBe(401);
  });

  it("GET /api/devices requires API key", async () => {
    const unauthReq = new NextRequest("http://localhost:8000/api/devices");
    const unauthRes = await devicesGet(unauthReq);
    expect(unauthRes.status).toBe(401);

    const authReq = new NextRequest("http://localhost:8000/api/devices", {
      headers: { "x-api-key": validKey },
    });
    const authRes = await devicesGet(authReq);
    expect(authRes.status).toBe(200);
    const list = await authRes.json();
    expect(Array.isArray(list)).toBe(true);
    expect(list.length).toBeGreaterThan(0);
  });

  it("GET /api/devices/[deviceId] returns device snapshot", async () => {
    const req = new NextRequest("http://localhost:8000/api/devices/esp32-smart-lamp", {
      headers: { "x-api-key": validKey },
    });
    const res = await deviceDetailGet(req, {
      params: Promise.resolve({ deviceId: "esp32-smart-lamp" }),
    });
    expect(res.status).toBe(200);
    const data = await res.json();
    expect(data.device_id).toBe("esp32-smart-lamp");
    expect(data.online).toBe(false);
  });

  it("PUT /api/devices/[deviceId]/state queues command when offline", async () => {
    const req = new NextRequest(
      "http://localhost:8000/api/devices/esp32-smart-lamp/state",
      {
        method: "PUT",
        headers: {
          "x-api-key": validKey,
          "content-type": "application/json",
        },
        body: JSON.stringify({ on: true }),
      }
    );
    const res = await deviceStatePut(req, {
      params: Promise.resolve({ deviceId: "esp32-smart-lamp" }),
    });
    expect(res.status).toBe(200);
    const data = await res.json();
    expect(data.device_id).toBe("esp32-smart-lamp");
    expect(data.on).toBe(true);
    expect(data.synced).toBe(false);
    expect(data.warning).toBe("device_offline_queued");
  });
});
