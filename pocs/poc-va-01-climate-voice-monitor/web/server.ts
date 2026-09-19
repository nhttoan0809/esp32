import { createServer, type IncomingMessage } from "http";
import { parse } from "url";
import next from "next";
import { WebSocketServer, WebSocket } from "ws";
import { getSettings } from "./src/lib/config";
import { verifyDeviceToken } from "./src/lib/auth";
import { getGlobalRegistry } from "./src/lib/registry";
import {
  HelloMessageSchema,
  StateReportMessageSchema,
  SensorDataMessageSchema,
} from "./src/lib/types";

const dev = process.env.NODE_ENV !== "production";
const hostname = "0.0.0.0";
const port = parseInt(process.env.PORT || "8000", 10);

const app = next({ dev, hostname, port });
const handle = app.getRequestHandler();

export async function createCustomServer() {
  await app.prepare();
  const settings = getSettings();
  const registry = getGlobalRegistry(settings.commandTimeoutSeconds);

  const server = createServer(async (req, res) => {
    try {
      const parsedUrl = parse(req.url || "", true);
      await handle(req, res, parsedUrl);
    } catch (err) {
      console.error("Error handling request:", err);
      res.statusCode = 500;
      res.end("Internal Server Error");
    }
  });

  const wss = new WebSocketServer({ noServer: true });

  server.on("upgrade", (req, socket, head) => {
    const { pathname } = parse(req.url || "", true);
    const match = pathname?.match(/^\/ws\/devices\/([^/?#]+)$/);

    if (!match || !match[1]) {
      socket.write("HTTP/1.1 404 Not Found\r\n\r\n");
      socket.destroy();
      return;
    }

    const deviceId = decodeURIComponent(match[1]);
    const authHeader = req.headers["authorization"] || "";
    let token: string | undefined;

    if (authHeader.startsWith("Bearer ")) {
      token = authHeader.slice(7).trim();
    }

    if (!verifyDeviceToken(deviceId, token, settings.deviceTokens)) {
      socket.write("HTTP/1.1 401 Unauthorized\r\n\r\n");
      socket.destroy();
      return;
    }

    wss.handleUpgrade(req, socket, head, (ws) => {
      wss.emit("connection", ws, req, deviceId);
    });
  });

  wss.on(
    "connection",
    async (ws: WebSocket, req: IncomingMessage, deviceId: string) => {
    const session = await registry.attach(deviceId, ws);
    let isReady = false;

    const helloTimer = setTimeout(() => {
      if (!isReady) {
        try {
          ws.close(1008, "hello_timeout");
        } catch {
          // ignore
        }
      }
    }, settings.helloTimeoutSeconds * 1000);

    ws.on("message", async (data, isBinary) => {
      if (isBinary) {
        ws.close(1003, "binary_not_supported");
        return;
      }

      const raw = data.toString();
      if (Buffer.byteLength(raw, "utf8") > settings.maxWebsocketMessageBytes) {
        ws.close(1003, "message_too_large");
        return;
      }

      let payload: unknown;
      try {
        payload = JSON.parse(raw);
      } catch {
        ws.close(1003, "invalid_json");
        return;
      }

      try {
        if (!isReady) {
          const hello = HelloMessageSchema.parse(payload);
          if (hello.device_id !== deviceId) {
            ws.close(1008, "hello_device_id_mismatch");
            return;
          }

          clearTimeout(helloTimer);
          const marked = await registry.markReady(
            session,
            hello.reported.on ?? false,
            hello.reported.sensors
          );
          if (!marked) {
            ws.close(1008, "session_replaced");
            return;
          }

          isReady = true;
          ws.send(JSON.stringify({ v: 1, type: "ready", device_id: deviceId }));
          await registry.reconcilePending(session);
        } else {
          const messageType = (payload as { type?: string })?.type;
          if (messageType === "sensor_data") {
            const sensorMsg = SensorDataMessageSchema.parse(payload);
            if (sensorMsg.device_id !== deviceId) {
              ws.close(1008, "sensor_device_id_mismatch");
              return;
            }
            await registry.recordSensorReport(session, sensorMsg.sensors);
          } else if (messageType === "state_report") {
            const report = StateReportMessageSchema.parse(payload);
            if (report.device_id !== deviceId) {
              ws.close(1008, "report_device_id_mismatch");
              return;
            }

            await registry.recordStateReport(
              session,
              report.command_id,
              report.on
            );
          } else {
            ws.close(1003, "unsupported_message_type");
            return;
          }
        }
      } catch (err) {
        console.warn(`WebSocket protocol error for ${deviceId}:`, err);
        ws.close(1003, "invalid_message");
      }
    });

    ws.on("close", async () => {
      clearTimeout(helloTimer);
      await registry.detach(session);
    });

    ws.on("error", async () => {
      clearTimeout(helloTimer);
      await registry.detach(session);
    });
  });

  return { server, wss, registry, settings };
}

// Start server when run directly
if (process.env.NODE_ENV !== "test") {
  createCustomServer().then(({ server }) => {
    server.listen(port, hostname, () => {
      console.log(`> Ready on http://${hostname}:${port}`);
    });
  });
}
