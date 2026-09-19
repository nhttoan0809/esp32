"use client";

import { useState, useEffect, useCallback, useRef } from "react";
import type { DeviceStateResponse } from "@/lib/types";

export function useDevices(apiKey: string | null, onUnauthorized?: () => void) {
  const [devices, setDevices] = useState<DeviceStateResponse[]>([]);
  const [loading, setLoading] = useState<boolean>(true);
  const [error, setError] = useState<string | null>(null);
  const [pendingIds, setPendingIds] = useState<Set<string>>(new Set());

  const onUnauthorizedRef = useRef(onUnauthorized);
  useEffect(() => {
    onUnauthorizedRef.current = onUnauthorized;
  }, [onUnauthorized]);

  const refreshDevices = useCallback(async () => {
    if (!apiKey) return;
    try {
      const res = await fetch("/api/devices", {
        headers: {
          "X-API-Key": apiKey,
        },
      });

      if (res.status === 401) {
        if (onUnauthorizedRef.current) {
          onUnauthorizedRef.current();
        }
        return;
      }

      if (!res.ok) {
        throw new Error(`HTTP error ${res.status}`);
      }

      const data: DeviceStateResponse[] = await res.json();
      setDevices(data);
      setError(null);
    } catch (err) {
      setError(err instanceof Error ? err.message : "Failed to fetch devices");
    } finally {
      setLoading(false);
    }
  }, [apiKey]);

  const setDeviceState = useCallback(
    async (deviceId: string, desiredOn: boolean): Promise<boolean> => {
      if (!apiKey) return false;

      setPendingIds((prev) => new Set(prev).add(deviceId));
      try {
        const res = await fetch(`/api/devices/${encodeURIComponent(deviceId)}/state`, {
          method: "PUT",
          headers: {
            "Content-Type": "application/json",
            "X-API-Key": apiKey,
          },
          body: JSON.stringify({ on: desiredOn }),
        });

        if (res.status === 401) {
          if (onUnauthorizedRef.current) {
            onUnauthorizedRef.current();
          }
          return false;
        }

        if (!res.ok) {
          throw new Error(`HTTP error ${res.status}`);
        }

        await refreshDevices();
        return true;
      } catch (err) {
        setError(err instanceof Error ? err.message : "Failed to toggle device");
        return false;
      } finally {
        setPendingIds((prev) => {
          const next = new Set(prev);
          next.delete(deviceId);
          return next;
        });
      }
    },
    [apiKey, refreshDevices]
  );

  const toggleDevice = useCallback(
    async (deviceId: string, currentOn: boolean): Promise<boolean> => {
      return setDeviceState(deviceId, !currentOn);
    },
    [setDeviceState]
  );

  const setBrightness = useCallback(
    async (deviceId: string, brightness: number): Promise<boolean> => {
      if (!apiKey) return false;
      try {
        const res = await fetch(
          `/api/devices/${encodeURIComponent(deviceId)}/brightness`,
          {
            method: "PUT",
            headers: {
              "Content-Type": "application/json",
              "X-API-Key": apiKey,
            },
            body: JSON.stringify({ brightness }),
          }
        );
        if (res.status === 401 && onUnauthorizedRef.current) {
          onUnauthorizedRef.current();
          return false;
        }
        await refreshDevices();
        return res.ok;
      } catch {
        return false;
      }
    },
    [apiKey, refreshDevices]
  );

  const setSecurityMode = useCallback(
    async (deviceId: string, mode: "guard" | "eco"): Promise<boolean> => {
      if (!apiKey) return false;
      try {
        const res = await fetch(
          `/api/devices/${encodeURIComponent(deviceId)}/security-mode`,
          {
            method: "PUT",
            headers: {
              "Content-Type": "application/json",
              "X-API-Key": apiKey,
            },
            body: JSON.stringify({ mode }),
          }
        );
        if (res.status === 401 && onUnauthorizedRef.current) {
          onUnauthorizedRef.current();
          return false;
        }
        await refreshDevices();
        return res.ok;
      } catch {
        return false;
      }
    },
    [apiKey, refreshDevices]
  );

  useEffect(() => {
    let active = true;
    if (!apiKey) return;

    const poll = async () => {
      if (!active) return;
      await refreshDevices();
    };

    const timer = setTimeout(() => {
      void poll();
    }, 0);

    const interval = setInterval(() => {
      void poll();
    }, 3000);

    return () => {
      active = false;
      clearTimeout(timer);
      clearInterval(interval);
    };
  }, [apiKey, refreshDevices]);

  return {
    devices,
    loading,
    error,
    pendingIds,
    refreshDevices,
    toggleDevice,
    setDeviceState,
    setBrightness,
    setSecurityMode,
  };
}

