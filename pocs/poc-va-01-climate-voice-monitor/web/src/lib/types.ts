import { z } from "zod";

export const SensorReadingsSchema = z
  .object({
    temperature: z.number().optional(),
    humidity: z.number().optional(),
    light_level: z.enum(["bright", "dark"]).optional(),
    led_brightness: z.number().min(0).max(100).optional(),
    motion_detected: z.boolean().optional(),
    security_mode: z.enum(["guard", "eco"]).optional(),
    pot_value: z.number().optional(),
  })
  .passthrough();

export type SensorReadings = z.infer<typeof SensorReadingsSchema>;

export const DeviceOutputSchema = z
  .object({
    on: z.boolean().optional().default(false),
    sensors: SensorReadingsSchema.optional(),
  })
  .passthrough();

export type DeviceOutput = z.infer<typeof DeviceOutputSchema>;

export const DeviceStateResponseSchema = z
  .object({
    device_id: z.string(),
    online: z.boolean(),
    on: z.boolean().nullable().optional().default(null),
    last_seen: z.string().nullable().optional().default(null),
    pending_on: z.boolean().nullable().optional().default(null),
    sensors: SensorReadingsSchema.optional(),
  })
  .strict();

export type DeviceStateResponse = z.infer<typeof DeviceStateResponseSchema>;

export const HelloMessageSchema = z
  .object({
    v: z.literal(1),
    type: z.literal("hello"),
    device_id: z.string().min(1).max(64),
    firmware: z.string().min(1).max(64),
    reported: DeviceOutputSchema,
  })
  .passthrough();

export type HelloMessage = z.infer<typeof HelloMessageSchema>;

export const StateReportMessageSchema = z
  .object({
    v: z.literal(1),
    type: z.literal("state_report"),
    command_id: z.string().uuid(),
    device_id: z.string().min(1).max(64),
    on: z.boolean(),
  })
  .strict();

export type StateReportMessage = z.infer<typeof StateReportMessageSchema>;

export const SensorDataMessageSchema = z
  .object({
    v: z.literal(1),
    type: z.literal("sensor_data"),
    device_id: z.string().min(1).max(64),
    sensors: SensorReadingsSchema,
  })
  .strict();

export type SensorDataMessage = z.infer<typeof SensorDataMessageSchema>;

export const SetStateRequestSchema = z
  .object({
    on: z.boolean(),
  })
  .strict();

export type SetStateRequest = z.infer<typeof SetStateRequestSchema>;

export const SetStateResponseSchema = z
  .object({
    device_id: z.string(),
    on: z.boolean(),
    synced: z.boolean(),
    command_id: z.string().uuid().nullable().optional().default(null),
    warning: z.string().nullable().optional().default(null),
  })
  .strict();

export type SetStateResponse = z.infer<typeof SetStateResponseSchema>;
