import { describe, it, expect, vi, beforeEach } from "vitest";
import { render, screen, fireEvent, waitFor } from "@testing-library/react";
import React from "react";
import LoginPage from "@/app/login/page";

const mockPush = vi.fn();
vi.mock("next/navigation", () => ({
  useRouter: () => ({
    push: mockPush,
  }),
}));

describe("LoginPage", () => {
  beforeEach(() => {
    vi.restoreAllMocks();
    mockPush.mockReset();
    localStorage.clear();
  });

  it("renders login form with API Key input and connect button", () => {
    render(<LoginPage />);
    expect(screen.getByPlaceholderText(/enter your api key/i)).toBeInTheDocument();
    expect(screen.getByRole("button", { name: /connect/i })).toBeInTheDocument();
  });

  it("authenticates and redirects to /dashboard on valid key", async () => {
    global.fetch = vi.fn().mockResolvedValue({
      ok: true,
      status: 200,
      json: async () => ({ status: "ok", authenticated: true }),
    } as Response);

    render(<LoginPage />);
    const input = screen.getByPlaceholderText(/enter your api key/i);
    const button = screen.getByRole("button", { name: /connect/i });

    fireEvent.change(input, { target: { value: "valid-secret-key" } });
    fireEvent.click(button);

    await waitFor(() => {
      expect(localStorage.getItem("lamp_dashboard_api_key")).toBe("valid-secret-key");
      expect(mockPush).toHaveBeenCalledWith("/dashboard");
    });
  });

  it("displays error message on invalid key", async () => {
    global.fetch = vi.fn().mockResolvedValue({
      ok: false,
      status: 401,
      json: async () => ({ detail: "Invalid dashboard API key" }),
    } as Response);

    render(<LoginPage />);
    const input = screen.getByPlaceholderText(/enter your api key/i);
    const button = screen.getByRole("button", { name: /connect/i });

    fireEvent.change(input, { target: { value: "wrong-key" } });
    fireEvent.click(button);

    await waitFor(() => {
      expect(screen.getByText(/Invalid dashboard API key/i)).toBeInTheDocument();
      expect(localStorage.getItem("lamp_dashboard_api_key")).toBeNull();
      expect(mockPush).not.toHaveBeenCalled();
    });
  });
});
