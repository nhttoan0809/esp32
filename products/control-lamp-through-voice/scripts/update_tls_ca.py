#!/usr/bin/env python3
"""
Auto-generate and update Root CA certificate bundle (tls_ca.h) for ESP32 WSS Client.

Usage:
  python3 update_tls_ca.py --host <hostname_or_url>
  python3 update_tls_ca.py --host products-roses-ticket-predict.trycloudflare.com
  python3 update_tls_ca.py --host https://xyz.trycloudflare.com/ws/devices --build
"""

import argparse
import datetime
import os
import re
import socket
import ssl
import subprocess
import sys
import urllib.parse
import urllib.request
from pathlib import Path

# Known reliable Root CAs for common IoT Tunnel Providers
WELL_KNOWN_ROOTS = {
    "GTS_ROOT_R4": {
        "name": "Google Trust Services GTS Root R4 (ECC)",
        "pem": (
            "-----BEGIN CERTIFICATE-----\n"
            "MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD\n"
            "VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG\n"
            "A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw\n"
            "WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz\n"
            "IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi\n"
            "AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi\n"
            "QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR\n"
            "HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW\n"
            "BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D\n"
            "9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8\n"
            "p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD\n"
            "-----END CERTIFICATE-----"
        ),
    },
    "GLOBALSIGN_ROOT_CA": {
        "name": "GlobalSign Root CA",
        "pem": (
            "-----BEGIN CERTIFICATE-----\n"
            "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\n"
            "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\n"
            "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\n"
            "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\n"
            "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\n"
            "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\n"
            "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\n"
            "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n"
            "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\n"
            "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\n"
            "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n"
            "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\n"
            "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\n"
            "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\n"
            "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n"
            "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\n"
            "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\n"
            "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\n"
            "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"
            "-----END CERTIFICATE-----"
        ),
    },
    "GTS_ROOT_R1": {
        "name": "Google Trust Services GTS Root R1 (RSA)",
        "pem": (
            "-----BEGIN CERTIFICATE-----\n"
            "MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw\n"
            "CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n"
            "MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw\n"
            "MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n"
            "Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA\n"
            "A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo\n"
            "27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w\n"
            "Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw\n"
            "TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl\n"
            "qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH\n"
            "szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8\n"
            "Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk\n"
            "MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92\n"
            "wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p\n"
            "aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN\n"
            "VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID\n"
            "AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\n"
            "FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb\n"
            "C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe\n"
            "QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy\n"
            "h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4\n"
            "7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J\n"
            "ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef\n"
            "MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/\n"
            "Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT\n"
            "6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ\n"
            "0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm\n"
            "2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb\n"
            "bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n"
            "-----END CERTIFICATE-----"
        ),
    },
    "ISRG_ROOT_X1": {
        "name": "ISRG Root X1 (Let's Encrypt / ngrok)",
        "pem": (
            "-----BEGIN CERTIFICATE-----\n"
            "MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw\n"
            "TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh\n"
            "cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4\n"
            "WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu\n"
            "ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY\n"
            "MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc\n"
            "h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+\n"
            "0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U\n"
            "A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW\n"
            "T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH\n"
            "B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC\n"
            "B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv\n"
            "KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn\n"
            "OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn\n"
            "jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw\n"
            "qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI\n"
            "rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV\n"
            "HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq\n"
            "hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL\n"
            "ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ\n"
            "3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK\n"
            "NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5\n"
            "ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur\n"
            "TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC\n"
            "jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc\n"
            "oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq\n"
            "4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA\n"
            "mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d\n"
            "emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=\n"
            "-----END CERTIFICATE-----"
        ),
    },
}


def sanitize_host(raw_host: str) -> str:
    """Normalize input host: strip URL protocols, paths, and trailing colons."""
    raw = raw_host.strip()
    if raw.startswith("http://") or raw.startswith("https://") or raw.startswith("wss://") or raw.startswith("ws://"):
        parsed = urllib.parse.urlparse(raw)
        return parsed.hostname or raw
    if "/" in raw:
        raw = raw.split("/")[0]
    if ":" in raw:
        raw = raw.split(":")[0]
    return raw


def detect_active_tunnel_host() -> str:
    """Try to detect active cloudflared or tunnel host from environment/processes/secrets."""
    script_dir = Path(__file__).resolve().parent
    secrets_path = script_dir.parent / "include" / "secrets.h"
    if secrets_path.exists():
        content = secrets_path.read_text(encoding="utf-8")
        match = re.search(r'WOKWI_PRECONFIG_SERVER_HOST\[\]\s*=\s*"([^"]+)"', content)
        if match and match.group(1) and "REPLACE" not in match.group(1):
            return match.group(1)
    return ""


def extract_live_certificates(host: str, port: int = 443) -> list:
    """Extract full certificate chain from live endpoint using OpenSSL."""
    certs = []
    try:
        cmd = [
            "openssl", "s_client",
            "-showcerts",
            "-servername", host,
            "-connect", f"{host}:{port}"
        ]
        proc = subprocess.run(
            cmd,
            input=b"",
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=10
        )
        stdout_text = proc.stdout.decode("utf-8", errors="ignore")

        cert_matches = re.findall(
            r"-----BEGIN CERTIFICATE-----[\s\S]+?-----END CERTIFICATE-----",
            stdout_text
        )
        for c in cert_matches:
            c_clean = c.strip()
            if c_clean:
                certs.append(c_clean)
    except Exception as e:
        print(f"[WARN] OpenSSL extraction failed: {e}")

    return certs


def get_cert_subject_issuer(pem_cert: str) -> tuple:
    """Use openssl x509 to get Subject and Issuer descriptions."""
    try:
        proc = subprocess.run(
            ["openssl", "x509", "-noout", "-subject", "-issuer"],
            input=pem_cert.encode("utf-8"),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=5
        )
        out = proc.stdout.decode("utf-8", errors="ignore").splitlines()
        subj = out[0] if len(out) > 0 else "Unknown Subject"
        issuer = out[1] if len(out) > 1 else "Unknown Issuer"
        return subj, issuer
    except Exception:
        return "Subject info unavailable", "Issuer info unavailable"


def build_bundle(host: str, live_certs: list, provider: str) -> list:
    """Compose the optimal Multi-Root CA Bundle."""
    bundle_entries = []
    seen_certs = set()

    def add_cert(name: str, pem: str):
        normalized = pem.strip()
        if normalized not in seen_certs:
            seen_certs.add(normalized)
            bundle_entries.append((name, normalized))

    is_cloudflare = "trycloudflare.com" in host or "cloudflare.com" in host or provider == "cloudflare"
    is_ngrok = "ngrok" in host or provider == "ngrok"

    if is_cloudflare or provider == "auto":
        add_cert(WELL_KNOWN_ROOTS["GTS_ROOT_R4"]["name"], WELL_KNOWN_ROOTS["GTS_ROOT_R4"]["pem"])
        add_cert(WELL_KNOWN_ROOTS["GLOBALSIGN_ROOT_CA"]["name"], WELL_KNOWN_ROOTS["GLOBALSIGN_ROOT_CA"]["pem"])
        add_cert(WELL_KNOWN_ROOTS["GTS_ROOT_R1"]["name"], WELL_KNOWN_ROOTS["GTS_ROOT_R1"]["pem"])

    if is_ngrok or provider == "auto" or is_cloudflare:
        add_cert(WELL_KNOWN_ROOTS["ISRG_ROOT_X1"]["name"], WELL_KNOWN_ROOTS["ISRG_ROOT_X1"]["pem"])

    for idx, pem in enumerate(live_certs):
        subj, issuer = get_cert_subject_issuer(pem)
        is_self_signed = (subj == issuer) or ("Root" in issuer)
        if is_self_signed:
            add_cert(f"Live Discovered Root/Intermediate ({subj})", pem)

    return bundle_entries


def update_secrets_file(secrets_path: Path, new_host: str):
    """Update WOKWI_PRECONFIG_SERVER_HOST in secrets.h if it exists."""
    if not secrets_path.exists():
        return
    content = secrets_path.read_text(encoding="utf-8")
    new_content = re.sub(
        r'WOKWI_PRECONFIG_SERVER_HOST\[\]\s*=\s*"[^"]*"',
        f'WOKWI_PRECONFIG_SERVER_HOST[] = "{new_host}"',
        content
    )
    if new_content != content:
        secrets_path.write_text(new_content, encoding="utf-8")
        print(f"[OK] 📝 Updated default host in: {secrets_path.name}")


def main():
    parser = argparse.ArgumentParser(
        description="Auto-extract TLS Root CAs and generate tls_ca.h for ESP32 WSS Client."
    )
    parser.add_argument(
        "--host",
        help="Domain or URL of tunnel/server (e.g. products-roses-ticket-predict.trycloudflare.com)"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=443,
        help="Port number (default: 443)"
    )
    parser.add_argument(
        "--provider",
        choices=["auto", "cloudflare", "ngrok", "letsencrypt"],
        default="auto",
        help="Tunnel or SSL provider preset (default: auto)"
    )
    parser.add_argument(
        "--output",
        help="Path to tls_ca.h header file"
    )
    parser.add_argument(
        "--build",
        action="store_true",
        help="Automatically compile firmware via PlatformIO after updating"
    )
    parser.add_argument(
        "--upload",
        action="store_true",
        help="Automatically compile and upload firmware via PlatformIO"
    )
    parser.add_argument(
        "--upload-port",
        default="/dev/cu.usbserial-0001",
        help="Serial port for firmware upload (default: /dev/cu.usbserial-0001)"
    )

    args = parser.parse_args()

    script_dir = Path(__file__).resolve().parent
    product_dir = script_dir.parent
    if args.output:
        header_path = Path(args.output).resolve()
    else:
        header_path = product_dir / "include" / "tls_ca.h"

    secrets_path = product_dir / "include" / "secrets.h"

    target_host = args.host
    if not target_host:
        target_host = detect_active_tunnel_host()
        if not target_host:
            print("[ERROR] ❌ No --host provided and no active tunnel detected.")
            print("Usage: python3 update_tls_ca.py --host <tunnel-domain.trycloudflare.com>")
            sys.exit(1)
        print(f"[INFO] 🔍 Auto-detected host from secrets: {target_host}")

    target_host = sanitize_host(target_host)
    print("=" * 70)
    print(f"🔒 ESP32 TLS CA Bundle Generator")
    print(f"   Target Host : {target_host}:{args.port}")
    print(f"   Provider    : {args.provider}")
    print(f"   Target File : {header_path}")
    print("=" * 70)

    print(f"[1/4] 🌐 Inspecting live TLS certificate chain on {target_host}:{args.port}...")
    live_certs = extract_live_certificates(target_host, args.port)
    if live_certs:
        print(f"      ✅ Received {len(live_certs)} certificate(s) from server handshake.")
        for i, cert in enumerate(live_certs):
            subj, issuer = get_cert_subject_issuer(cert)
            print(f"      Cert #{i}: {subj} | {issuer}")
    else:
        print("      ⚠️ Could not fetch live chain directly (network/sandbox limit). Using verified provider trust anchors.")

    print("[2/4] 📦 Composing Multi-Root CA bundle...")
    bundle_entries = build_bundle(target_host, live_certs, args.provider)
    print(f"      ✅ Bundle assembled with {len(bundle_entries)} Root CAs:")
    for name, _ in bundle_entries:
        print(f"         • {name}")

    now_str = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    header_content = [
        "#pragma once",
        "",
        "// =============================================================================",
        "// AUTO-GENERATED ROOT CA BUNDLE FOR ESP32 WSS TLS VERIFICATION",
        f"// Generated at : {now_str}",
        f"// Target Host  : {target_host}:{args.port}",
        f"// Provider     : {args.provider}",
        "// Generator    : products/control-lamp-through-voice/scripts/update_tls_ca.py",
        "// =============================================================================",
        "// Contains trusted public root certificates for seamless WSS handshake",
        "// across Cloudflare Tunnels (Google Trust Services / GlobalSign) and ngrok (ISRG).",
        "constexpr char SERVER_ROOT_CA[] = R\"CERT("
    ]

    for name, pem in bundle_entries:
        header_content.append(pem)

    header_content.append(")CERT\";\n")

    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text("\n".join(header_content), encoding="utf-8")
    print(f"[3/4] 💾 Saved {header_path.name} ({len('\n'.join(header_content))} bytes).")

    update_secrets_file(secrets_path, target_host)

    if args.upload:
        print(f"[4/4] 🚀 Compiling and uploading firmware to {args.upload_port}...")
        cmd = [
            "pio", "run",
            "-d", str(product_dir),
            "-e", "esp32dev",
            "-t", "upload",
            "--upload-port", args.upload_port
        ]
        res = subprocess.run(cmd)
        if res.returncode != 0:
            print("[ERROR] ❌ Upload failed!")
            sys.exit(res.returncode)
        print("[SUCCESS] 🎉 Firmware uploaded successfully!")
    elif args.build:
        print("[4/4] 🔨 Compiling firmware via PlatformIO...")
        cmd = ["pio", "run", "-d", str(product_dir), "-e", "esp32dev"]
        res = subprocess.run(cmd)
        if res.returncode != 0:
            print("[ERROR] ❌ Build failed!")
            sys.exit(res.returncode)
        print("[SUCCESS] 🎉 Firmware built successfully!")
    else:
        print("[4/4] 📌 Next steps:")
        print(f"      1. Recompile and flash firmware:")
        print(f"         pio run -d products/control-lamp-through-voice -e esp32dev -t upload --upload-port {args.upload_port}")
        print(f"      2. If you changed the host domain, update it in the ESP32 setup portal (192.168.4.1)")
        print(f"         or restart ESP32 to connect to: {target_host}")

    print("=" * 70)


if __name__ == "__main__":
    main()
