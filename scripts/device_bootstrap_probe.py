#!/usr/bin/env python3
"""
Device Bootstrap Probe Controller
Manages bootstrap validation on physical Android devices
Part of Stage 4: Device Validation & Release (Days 11-14)
"""

import json
import sys
import subprocess
from typing import Dict, List, Tuple, Optional
from pathlib import Path
import hashlib
import time

class DeviceBootstrapProbe:
    """Controller for bootstrap probe execution on Android device"""

    def __init__(self, device_serial: Optional[str] = None):
        """
        Initialize device controller
        Args:
            device_serial: ADB device serial (None = first available)
        """
        self.device_serial = device_serial or self._get_first_device()
        self.adb_prefix = ["adb", "-s", self.device_serial] if self.device_serial else ["adb"]
        self.probe_path = "/data/local/tmp"
        self.results = []

    def _get_first_device(self) -> Optional[str]:
        """Get first connected device serial"""
        try:
            result = subprocess.run(["adb", "devices"], capture_output=True, text=True, timeout=5)
            lines = result.stdout.split('\n')
            for line in lines:
                parts = line.split()
                if len(parts) == 2 and parts[1] == "device":
                    return parts[0]
        except Exception:
            pass
        return None

    def is_connected(self) -> bool:
        """Check if device is connected"""
        try:
            result = subprocess.run(self.adb_prefix + ["shell", "echo", "OK"],
                                  capture_output=True, text=True, timeout=5)
            return result.returncode == 0
        except Exception:
            return False

    def get_device_info(self) -> Dict:
        """Get device information"""
        info = {
            "serial": self.device_serial,
            "connected": self.is_connected(),
            "api_level": None,
            "android_version": None,
            "arch": None,
            "abi_list": None
        }

        if not info["connected"]:
            return info

        try:
            # Get API level
            result = subprocess.run(self.adb_prefix + ["shell", "getprop", "ro.build.version.sdk"],
                                  capture_output=True, text=True, timeout=5)
            info["api_level"] = int(result.stdout.strip()) if result.stdout.strip() else None

            # Get Android version
            result = subprocess.run(self.adb_prefix + ["shell", "getprop", "ro.build.version.release"],
                                  capture_output=True, text=True, timeout=5)
            info["android_version"] = result.stdout.strip() if result.stdout.strip() else None

            # Get architecture
            result = subprocess.run(self.adb_prefix + ["shell", "getprop", "ro.product.cpu.abi"],
                                  capture_output=True, text=True, timeout=5)
            info["abi_list"] = result.stdout.strip() if result.stdout.strip() else None

            # Determine arch
            abi = info["abi_list"] or ""
            if "arm64" in abi or "aarch64" in abi:
                info["arch"] = "arm64-v8a"
            elif "armeabi-v7a" in abi or "armeabi" in abi:
                info["arch"] = "armeabi-v7a"
            else:
                info["arch"] = "unknown"

        except Exception as e:
            info["error"] = str(e)

        return info

    def push_probe_apk(self, apk_path: str) -> Tuple[bool, str]:
        """Push bootstrap probe APK to device"""
        try:
            result = subprocess.run(self.adb_prefix + ["push", apk_path, self.probe_path],
                                  capture_output=True, text=True, timeout=30)
            if result.returncode == 0:
                return True, f"APK pushed to {self.probe_path}"
            else:
                return False, f"Push failed: {result.stderr}"
        except Exception as e:
            return False, f"Push error: {str(e)}"

    def install_apk(self, package_name: str) -> Tuple[bool, str]:
        """Install APK on device"""
        try:
            result = subprocess.run(self.adb_prefix + ["install", "-r", "-g",
                                                      f"{self.probe_path}/{package_name}.apk"],
                                  capture_output=True, text=True, timeout=60)
            if "Success" in result.stdout:
                return True, "APK installed successfully"
            else:
                return False, f"Install failed: {result.stdout}"
        except Exception as e:
            return False, f"Install error: {str(e)}"

    def start_bootstrap_validator(self) -> Tuple[bool, str]:
        """Start bootstrap validator activity"""
        try:
            result = subprocess.run(
                self.adb_prefix + ["shell", "am", "start", "-n",
                                 "com.termux.rafacodephi/.BootstrapValidator"],
                capture_output=True, text=True, timeout=10)
            return result.returncode == 0, result.stdout + result.stderr
        except Exception as e:
            return False, str(e)

    def fetch_device_logs(self, log_tag: str = "BOOTSTRAP") -> List[str]:
        """Fetch logcat logs for specific tag"""
        try:
            result = subprocess.run(self.adb_prefix + ["logcat", "-d", "-s", log_tag],
                                  capture_output=True, text=True, timeout=10)
            return result.stdout.split('\n')
        except Exception:
            return []

    def fetch_receipt(self, device_path: str) -> Optional[Dict]:
        """Fetch bootstrap receipt from device"""
        try:
            # Pull receipt from device
            local_path = "/tmp/device-receipt.json"
            result = subprocess.run(self.adb_prefix + ["pull", device_path, local_path],
                                  capture_output=True, text=True, timeout=30)

            if result.returncode != 0:
                return None

            with open(local_path, 'r') as f:
                return json.load(f)
        except Exception:
            return None

    def execute_bootstrap_scenario(self, scenario: Dict) -> Dict:
        """Execute a single bootstrap test scenario"""
        result = {
            "scenario_id": scenario.get("id"),
            "scenario_name": scenario.get("name"),
            "status": "PENDING",
            "start_time": time.time(),
            "end_time": None,
            "duration_seconds": 0,
            "message": ""
        }

        try:
            # Start validator
            success, msg = self.start_bootstrap_validator()
            if not success:
                result["status"] = "FAILED"
                result["message"] = f"Failed to start validator: {msg}"
                result["end_time"] = time.time()
                result["duration_seconds"] = result["end_time"] - result["start_time"]
                return result

            # Wait for scenario completion
            timeout = scenario.get("timeout_seconds", 10)
            time.sleep(min(timeout, 5))  # Wait for scenario to complete

            # Fetch logs
            logs = self.fetch_device_logs()
            result["logs"] = logs[-20:] if logs else []  # Last 20 log lines

            # Check for success indicators in logs
            if any("PASS" in log for log in logs):
                result["status"] = "PASSED"
                result["message"] = "Scenario passed (log evidence)"
            else:
                result["status"] = "INCONCLUSIVE"
                result["message"] = "Scenario executed but no pass confirmation in logs"

        except Exception as e:
            result["status"] = "ERROR"
            result["message"] = str(e)

        result["end_time"] = time.time()
        result["duration_seconds"] = result["end_time"] - result["start_time"]
        self.results.append(result)
        return result

    def generate_device_receipt(self, scenarios_results: List[Dict]) -> Dict:
        """Generate device runtime receipt"""
        device_info = self.get_device_info()

        receipt = {
            "schema": "raf.device-bootstrap-receipt.v1",
            "timestamp": __import__('datetime').datetime.utcnow().isoformat() + 'Z',
            "device": {
                "serial": self.device_serial,
                "platform": device_info.get("arch", "unknown"),
                "android_version": device_info.get("android_version"),
                "api_level": device_info.get("api_level")
            },
            "scenarios_executed": len(scenarios_results),
            "scenarios_passed": sum(1 for s in scenarios_results if s.get("status") == "PASSED"),
            "scenarios_failed": sum(1 for s in scenarios_results if s.get("status") == "FAILED"),
            "scenarios_inconclusive": sum(1 for s in scenarios_results if s.get("status") == "INCONCLUSIVE"),
            "test_results": scenarios_results,
            "overall_status": "PASSED" if all(s.get("status") == "PASSED" for s in scenarios_results) else "FAILED"
        }

        return receipt

def main():
    if len(sys.argv) < 2:
        print("Usage: device_bootstrap_probe.py [--list-devices | --probe SERIAL | --execute-scenario ID]",
              file=sys.stderr)
        sys.exit(1)

    if "--list-devices" in sys.argv:
        probe = DeviceBootstrapProbe()
        try:
            result = subprocess.run(["adb", "devices"], capture_output=True, text=True)
            print(result.stdout)
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            sys.exit(1)

    elif "--probe" in sys.argv:
        idx = sys.argv.index("--probe")
        serial = sys.argv[idx + 1] if idx + 1 < len(sys.argv) else None

        probe = DeviceBootstrapProbe(serial)
        device_info = probe.get_device_info()

        print(json.dumps(device_info, indent=2))
        sys.exit(0 if device_info["connected"] else 1)

    elif "--execute-scenario" in sys.argv:
        idx = sys.argv.index("--execute-scenario")
        scenario_id = sys.argv[idx + 1] if idx + 1 < len(sys.argv) else None
        serial = sys.argv[idx + 2] if idx + 2 < len(sys.argv) else None

        if not scenario_id:
            print("Error: --execute-scenario requires scenario ID", file=sys.stderr)
            sys.exit(1)

        probe = DeviceBootstrapProbe(serial)

        if not probe.is_connected():
            print("Error: No device connected", file=sys.stderr)
            sys.exit(1)

        # Dummy scenario for demonstration
        scenario = {
            "id": scenario_id,
            "name": f"Scenario {scenario_id}",
            "timeout_seconds": 10
        }

        result = probe.execute_bootstrap_scenario(scenario)
        print(json.dumps(result, indent=2))
        sys.exit(0 if result["status"] == "PASSED" else 1)

    else:
        print("Error: Unknown command", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()
