import { describe, it, expect, vi, beforeEach } from "vitest";
import { render, screen, fireEvent, waitFor } from "@testing-library/react";
import React from "react";
import DashboardPage from "@/app/dashboard/page";

const mockPush = vi.fn();
vi.mock("next/navigation", () => ({
  useRouter: () => ({
    push: mockPush,
  }),
}));

describe("DashboardPage", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
    mockPush.mockReset();
    localStorage.clear();
  });

  it("redirects to /login if no API key is present in localStorage", async () => {
    render(<DashboardPage />);
    await waitFor(() => {
      expect(mockPush).toHaveBeenCalledWith("/login");
    });
  });

  it("renders dashboard and handles logout when API key is present", async () => {
    localStorage.setItem("lamp_dashboard_api_key", "test-key");

    global.fetch = vi.fn().mockResolvedValue({
      ok: true,
      status: 200,
      json: async () => [
        {
          device_id: "lamp-esp32-01",
          online: true,
          state: "OFF",
          gpio_pin: 26,
          last_seen: new Date().toISOString(),
        },
      ],
    } as Response);

    render(<DashboardPage />);

    await waitFor(() => {
      expect(screen.getByText(/Smart Lamp Control/i)).toBeInTheDocument();
    });

    const logoutBtn = screen.getByRole("button", { name: /disconnect|logout/i });
    fireEvent.click(logoutBtn);

    expect(localStorage.getItem("lamp_dashboard_api_key")).toBeNull();
    expect(mockPush).toHaveBeenCalledWith("/login");
  });
});
