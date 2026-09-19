import type { WebSocket } from "ws";
import crypto from "crypto";
import type { SensorReadings } from "./types";

export class DeviceOfflineError extends Error {
  constructor(message = "device_offline") {
    super(message);
    this.name = "DeviceOfflineError";
  }
}

export class DeviceDisconnectedError extends Error {
  constructor(message = "device_disconnected") {
    super(message);
    this.name = "DeviceDisconnectedError";
  }
}

export class DeviceAckTimeoutError extends Error {
  constructor(message = "device_ack_timeout") {
    super(message);
    this.name = "DeviceAckTimeoutError";
  }
}

export class DeviceDidNotApplyError extends Error {
  constructor(message = "device_did_not_apply_state") {
    super(message);
    this.name = "DeviceDidNotApplyError";
  }
}

export interface DeviceSnapshot {
  deviceId: string;
  online: boolean;
  on: boolean | null;
  lastSeen: string | null;
  pendingOn: boolean | null;
  sensors?: SensorReadings;
}

export interface PendingCommand {
  commandId: string;
  requestedOn: boolean;
  resolve: (on: boolean) => void;
  reject: (err: Error) => void;
  timer: NodeJS.Timeout;
}

export interface DeviceSession {
  deviceId: string;
  ws: WebSocket;
  generation: number;
  online: boolean;
  pending?: PendingCommand | null;
}

export class DeviceRegistry {
  private commandTimeoutSeconds: number;
  private sessions = new Map<string, DeviceSession>();
  private snapshots = new Map<string, DeviceSnapshot>();
  private generation = 0;

  constructor(commandTimeoutSeconds = 3.0) {
    this.commandTimeoutSeconds = commandTimeoutSeconds;
  }

  public async attach(deviceId: string, ws: WebSocket): Promise<DeviceSession> {
    const oldSession = this.sessions.get(deviceId);
    this.generation += 1;
    const session: DeviceSession = {
      deviceId,
      ws,
      generation: this.generation,
      online: false,
      pending: null,
    };
    this.sessions.set(deviceId, session);

    let snap = this.snapshots.get(deviceId);
    if (!snap) {
      snap = {
        deviceId,
        online: false,
        on: null,
        lastSeen: null,
        pendingOn: null,
      };
      this.snapshots.set(deviceId, snap);
    } else {
      snap.online = false;
    }

    if (oldSession) {
      this.failPending(oldSession, new DeviceDisconnectedError());
      try {
        oldSession.ws.close(1012, "replaced_by_new_connection");
      } catch {
        // quiet close
      }
    }

    return session;
  }

  public async markReady(
    session: DeviceSession,
    on: boolean,
    sensors?: SensorReadings
  ): Promise<boolean> {
    if (this.sessions.get(session.deviceId) !== session) {
      return false;
    }
    session.online = true;

    let snap = this.snapshots.get(session.deviceId);
    if (!snap) {
      snap = {
        deviceId: session.deviceId,
        online: true,
        on,
        lastSeen: new Date().toISOString(),
        pendingOn: null,
        sensors: sensors ? { ...sensors } : {},
      };
      this.snapshots.set(session.deviceId, snap);
    } else {
      snap.online = true;
      snap.on = on;
      snap.lastSeen = new Date().toISOString();
      if (sensors) {
        snap.sensors = { ...(snap.sensors || {}), ...sensors };
      }
    }
    return true;
  }

  public async recordSensorReport(
    session: DeviceSession,
    sensors: SensorReadings
  ): Promise<boolean> {
    if (this.sessions.get(session.deviceId) !== session) {
      return false;
    }

    let snap = this.snapshots.get(session.deviceId);
    if (!snap) {
      snap = {
        deviceId: session.deviceId,
        online: session.online,
        on: null,
        lastSeen: new Date().toISOString(),
        pendingOn: null,
        sensors: { ...sensors },
      };
      this.snapshots.set(session.deviceId, snap);
    } else {
      snap.online = session.online;
      snap.sensors = { ...(snap.sensors || {}), ...sensors };
      snap.lastSeen = new Date().toISOString();
    }
    return true;
  }

  public async detach(session: DeviceSession): Promise<void> {
    if (this.sessions.get(session.deviceId) !== session) {
      return;
    }
    this.sessions.delete(session.deviceId);
    const snap = this.snapshots.get(session.deviceId);
    if (snap) {
      snap.online = false;
      snap.lastSeen = new Date().toISOString();
    }
    this.failPending(session, new DeviceDisconnectedError());
  }

  public async recordStateReport(
    session: DeviceSession,
    commandId: string,
    on: boolean
  ): Promise<boolean> {
    if (this.sessions.get(session.deviceId) !== session) {
      return false;
    }

    let snap = this.snapshots.get(session.deviceId);
    if (!snap) {
      snap = {
        deviceId: session.deviceId,
        online: session.online,
        on,
        lastSeen: new Date().toISOString(),
        pendingOn: null,
      };
      this.snapshots.set(session.deviceId, snap);
    } else {
      snap.online = session.online;
      snap.on = on;
      snap.lastSeen = new Date().toISOString();
    }

    if (snap.pendingOn !== null && snap.pendingOn === on) {
      snap.pendingOn = null;
    }

    const pending = session.pending;
    if (!pending || pending.commandId !== commandId) {
      return false;
    }

    clearTimeout(pending.timer);
    session.pending = null;

    if (pending.requestedOn !== on) {
      pending.reject(new DeviceDidNotApplyError());
    } else {
      pending.resolve(on);
    }
    return true;
  }

  public async snapshot(deviceId: string): Promise<DeviceSnapshot> {
    const snap = this.snapshots.get(deviceId);
    if (!snap) {
      return {
        deviceId,
        online: false,
        on: null,
        lastSeen: null,
        pendingOn: null,
      };
    }
    return { ...snap };
  }

  public async allSnapshots(): Promise<Record<string, DeviceSnapshot>> {
    const res: Record<string, DeviceSnapshot> = {};
    for (const [id, snap] of this.snapshots.entries()) {
      res[id] = { ...snap };
    }
    return res;
  }

  public async allDeviceIds(): Promise<Set<string>> {
    return new Set(this.snapshots.keys());
  }

  public async queueState(deviceId: string, on: boolean): Promise<void> {
    let snap = this.snapshots.get(deviceId);
    if (!snap) {
      snap = {
        deviceId,
        online: false,
        on: null,
        lastSeen: null,
        pendingOn: on,
      };
      this.snapshots.set(deviceId, snap);
    } else {
      snap.pendingOn = on;
    }
  }

  public async reconcilePending(session: DeviceSession): Promise<boolean> {
    if (this.sessions.get(session.deviceId) !== session) {
      return false;
    }
    const snap = this.snapshots.get(session.deviceId);
    if (!snap || snap.pendingOn === null) {
      return false;
    }
    const queued = snap.pendingOn;
    if (snap.on === queued) {
      snap.pendingOn = null;
      return false;
    }

    const commandId = crypto.randomUUID();
    const command = {
      v: 1,
      type: "set_state",
      command_id: commandId,
      device_id: session.deviceId,
      on: queued,
    };

    try {
      session.ws.send(JSON.stringify(command));
      return true;
    } catch {
      return false;
    }
  }

  public async sendStateCommand(
    deviceId: string,
    on: boolean
  ): Promise<{ commandId: string; confirmedOn: boolean }> {
    const session = this.sessions.get(deviceId);
    if (!session || !session.online) {
      throw new DeviceOfflineError();
    }

    const commandId = crypto.randomUUID();
    const command = {
      v: 1,
      type: "set_state",
      command_id: commandId,
      device_id: deviceId,
      on,
    };

    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        if (session.pending?.commandId === commandId) {
          session.pending = null;
        }
        void this.queueState(deviceId, on);
        reject(new DeviceAckTimeoutError());
      }, this.commandTimeoutSeconds * 1000);

      session.pending = {
        commandId,
        requestedOn: on,
        resolve: (confirmedOn: boolean) => {
          clearTimeout(timer);
          void this.setPending(deviceId, null);
          resolve({ commandId, confirmedOn });
        },
        reject: (err: Error) => {
          clearTimeout(timer);
          void this.queueState(deviceId, on);
          reject(err);
        },
        timer,
      };

      try {
        session.ws.send(JSON.stringify(command));
      } catch (err) {
        clearTimeout(timer);
        session.pending = null;
        void this.queueState(deviceId, on);
        reject(new DeviceDisconnectedError(String(err)));
      }
    });
  }

  public async sendBrightnessCommand(
    deviceId: string,
    brightness: number
  ): Promise<{ commandId: string; confirmedBrightness: number }> {
    const session = this.sessions.get(deviceId);
    if (!session || !session.online) {
      throw new DeviceOfflineError();
    }

    const commandId = crypto.randomUUID();
    const command = {
      v: 1,
      type: "set_brightness",
      command_id: commandId,
      device_id: deviceId,
      brightness,
    };

    session.ws.send(JSON.stringify(command));
    const snap = this.snapshots.get(deviceId);
    if (snap) {
      snap.sensors = { ...(snap.sensors || {}), led_brightness: brightness };
    }
    return { commandId, confirmedBrightness: brightness };
  }

  public async sendSecurityModeCommand(
    deviceId: string,
    mode: "guard" | "eco"
  ): Promise<{ commandId: string; mode: string }> {
    const session = this.sessions.get(deviceId);
    if (!session || !session.online) {
      throw new DeviceOfflineError();
    }

    const commandId = crypto.randomUUID();
    const command = {
      v: 1,
      type: "set_security_mode",
      command_id: commandId,
      device_id: deviceId,
      mode,
    };

    session.ws.send(JSON.stringify(command));
    const snap = this.snapshots.get(deviceId);
    if (snap) {
      snap.sensors = { ...(snap.sensors || {}), security_mode: mode };
    }
    return { commandId, mode };
  }

  private failPending(session: DeviceSession, error: Error): void {
    if (session.pending) {
      clearTimeout(session.pending.timer);
      session.pending.reject(error);
      session.pending = null;
    }
  }

  private async setPending(deviceId: string, on: boolean | null): Promise<void> {
    const snap = this.snapshots.get(deviceId);
    if (snap) {
      snap.pendingOn = on;
    }
  }
}

// Global registry instance shared across Next.js API routes and custom server
const globalForRegistry = globalThis as unknown as {
  deviceRegistry?: DeviceRegistry;
};

export function getGlobalRegistry(commandTimeoutSeconds = 3.0): DeviceRegistry {
  if (!globalForRegistry.deviceRegistry) {
    globalForRegistry.deviceRegistry = new DeviceRegistry(commandTimeoutSeconds);
  }
  return globalForRegistry.deviceRegistry;
}
