import crypto from "crypto";

export function timingSafeEqualString(a: string, b: string): boolean {
  const bufA = Buffer.from(a);
  const bufB = Buffer.from(b);
  if (bufA.length !== bufB.length) {
    return false;
  }
  return crypto.timingSafeEqual(bufA, bufB);
}

export function verifyDashboardApiKey(
  provided: string | undefined,
  expected: string
): boolean {
  if (!provided || typeof provided !== "string") {
    return false;
  }
  return timingSafeEqualString(provided, expected);
}

export function verifyDeviceToken(
  deviceId: string,
  token: string | undefined,
  deviceTokens: Record<string, string>
): boolean {
  if (!token || typeof token !== "string") {
    return false;
  }
  const expected = deviceTokens[deviceId];
  if (!expected) {
    return false;
  }
  return timingSafeEqualString(token, expected);
}
