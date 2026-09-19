export interface Settings {
  dashboardApiKey: string;
  deviceTokens: Record<string, string>;
  commandTimeoutSeconds: number;
  helloTimeoutSeconds: number;
  maxWebsocketMessageBytes: number;
}

export function loadSettings(): Settings {
  const rawTokens =
    process.env.LAMP_DEVICE_TOKENS_JSON ||
    process.env.POC5_DEVICE_TOKENS_JSON ||
    '{"esp32-smart-lamp":"lamp-secret-token"}';

  let parsedTokens: unknown;
  try {
    parsedTokens = JSON.parse(rawTokens);
  } catch (error) {
    throw new Error("DEVICE_TOKENS_JSON is not valid JSON: " + String(error));
  }

  if (
    typeof parsedTokens !== "object" ||
    parsedTokens === null ||
    Array.isArray(parsedTokens) ||
    Object.keys(parsedTokens).length === 0
  ) {
    throw new Error("DEVICE_TOKENS_JSON must be a non-empty object");
  }

  const deviceTokens: Record<string, string> = {};
  for (const [deviceId, token] of Object.entries(parsedTokens)) {
    if (!deviceId || typeof deviceId !== "string") {
      throw new Error("Every device ID must be a non-empty string");
    }
    if (!token || typeof token !== "string") {
      throw new Error("Every device token must be a non-empty string");
    }
    deviceTokens[deviceId] = token;
  }

  const dashboardApiKey =
    process.env.LAMP_DASHBOARD_API_KEY ||
    process.env.POC5_DASHBOARD_API_KEY ||
    "lamp-dashboard-secret";

  if (!dashboardApiKey) {
    throw new Error("DASHBOARD_API_KEY must not be empty");
  }

  return {
    dashboardApiKey,
    deviceTokens,
    commandTimeoutSeconds: 3.0,
    helloTimeoutSeconds: 5.0,
    maxWebsocketMessageBytes: 2048,
  };
}

let cachedSettings: Settings | null = null;

export function getSettings(): Settings {
  if (!cachedSettings) {
    cachedSettings = loadSettings();
  }
  return cachedSettings;
}
