#!/usr/bin/env python3
import argparse
import concurrent.futures
import ipaddress
import json
import re
import sys
from dataclasses import dataclass
from typing import Optional

import requests

INFO_PATH = "/api/device/info"
PROVISION_PATH = "/api/device/provision"
HOSTNAME_PATH = "/api/device/hostname"
DEFAULT_PORT = 80
DEFAULT_TIMEOUT = 1.5
DEFAULT_PREFIX = "esp32-"
DEFAULT_DIGITS = 3


@dataclass
class DeviceInfo:
    ip: str
    hardware_id: str
    device_id: str
    effective_id: str
    provisioned: bool
    hostname: str = ""
    effective_hostname: str = ""

    @property
    def display_id(self) -> str:
        return self.device_id or self.effective_id or self.hardware_id


def fetch_device_info(ip: str, port: int, timeout: float) -> Optional[DeviceInfo]:
    url = f"http://{ip}:{port}{INFO_PATH}"
    try:
        response = requests.get(url, timeout=timeout)
        response.raise_for_status()
        data = response.json()

        return DeviceInfo(
            ip=ip,
            hardware_id=str(data.get("hardware_id", "")),
            device_id=str(data.get("device_id", "")),
            effective_id=str(data.get("effective_id", "")),
            provisioned=bool(data.get("provisioned", False)),
            hostname=str(data.get("hostname", "")),
            effective_hostname=str(data.get("effective_hostname", "")),
        )
    except (requests.RequestException, ValueError, json.JSONDecodeError):
        return None


def provision_device(
    ip: str,
    port: int,
    timeout: float,
    new_id: str,
    hostname: Optional[str] = None,
) -> tuple[bool, str]:
    url = f"http://{ip}:{port}{PROVISION_PATH}"
    payload = {"device_id": new_id}

    if hostname:
        payload["hostname"] = hostname

    try:
        response = requests.post(url, json=payload, timeout=timeout)
        if not response.ok:
            return False, response.text.strip()
        return True, response.text.strip()
    except requests.RequestException as exc:
        return False, str(exc)


def update_hostname(
    ip: str,
    port: int,
    timeout: float,
    hostname: str,
) -> tuple[bool, str]:
    url = f"http://{ip}:{port}{HOSTNAME_PATH}"
    payload = {"hostname": hostname}

    try:
        response = requests.post(url, json=payload, timeout=timeout)
        if not response.ok:
            return False, response.text.strip()
        return True, response.text.strip()
    except requests.RequestException as exc:
        return False, str(exc)


def scan_subnet(subnet: str, port: int, timeout: float, max_workers: int) -> list[DeviceInfo]:
    network = ipaddress.ip_network(subnet, strict=False)
    hosts = [str(ip) for ip in network.hosts()]
    found: list[DeviceInfo] = []

    with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(fetch_device_info, ip, port, timeout): ip
            for ip in hosts
        }

        for future in concurrent.futures.as_completed(futures):
            info = future.result()
            if info is not None:
                found.append(info)

    found.sort(key=lambda d: ipaddress.ip_address(d.ip))
    return found


def extract_numeric_suffix(device_id: str, prefix: str) -> Optional[int]:
    pattern = rf"^{re.escape(prefix)}(\d+)$"
    match = re.match(pattern, device_id)
    if not match:
        return None
    return int(match.group(1))


def next_free_id(devices: list[DeviceInfo], prefix: str, digits: int) -> str:
    used_numbers = set()

    for device in devices:
        for candidate in (device.device_id, device.effective_id):
            if not candidate:
                continue
            value = extract_numeric_suffix(candidate, prefix)
            if value is not None:
                used_numbers.add(value)

    n = 1
    while n in used_numbers:
        n += 1

    return f"{prefix}{n:0{digits}d}"


def is_valid_hostname(hostname: str) -> bool:
    if not hostname:
        return False
    if len(hostname) > 32:
        return False
    if hostname.startswith("-") or hostname.endswith("-"):
        return False
    return re.fullmatch(r"[A-Za-z0-9-]+", hostname) is not None


def print_devices(devices: list[DeviceInfo]) -> None:
    if not devices:
        print("No ESP32 devices found.")
        return

    print(f"{'IP':<16} {'PROVISIONED':<12} {'DEVICE_ID':<15} {'HOSTNAME':<20} {'HARDWARE_ID':<20}")
    print("-" * 92)
    for d in devices:
        print(
            f"{d.ip:<16} "
            f"{str(d.provisioned):<12} "
            f"{(d.device_id or '-'): <15} "
            f"{(d.effective_hostname or d.hostname or '-'): <20} "
            f"{(d.hardware_id or '-'): <20}"
        )


def choose_target(
    devices: list[DeviceInfo],
    target_ip: Optional[str],
    target_hwid: Optional[str],
    only_unprovisioned: bool,
) -> Optional[DeviceInfo]:
    candidates = devices

    if only_unprovisioned:
        candidates = [d for d in candidates if not d.provisioned or not d.device_id]

    if target_ip:
        candidates = [d for d in candidates if d.ip == target_ip]

    if target_hwid:
        candidates = [d for d in candidates if d.hardware_id == target_hwid]

    if len(candidates) == 1:
        return candidates[0]

    if len(candidates) == 0:
        return None

    print("Multiple matching devices found. Narrow it with --ip or --hwid:")
    print_devices(candidates)
    return None


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Scan ESP32 devices and assign IDs and hostnames."
    )

    parser.add_argument(
        "--subnet",
        required=True,
        help="Subnet to scan, for example 192.168.1.0/24",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=DEFAULT_PORT,
        help=f"HTTP port on device (default: {DEFAULT_PORT})",
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=DEFAULT_TIMEOUT,
        help=f"HTTP timeout in seconds (default: {DEFAULT_TIMEOUT})",
    )
    parser.add_argument(
        "--workers",
        type=int,
        default=64,
        help="Parallel scan workers (default: 64)",
    )
    parser.add_argument(
        "--list",
        action="store_true",
        help="List discovered devices and exit",
    )
    parser.add_argument(
        "--assign-next",
        action="store_true",
        help="Assign the next free ID to a selected device",
    )
    parser.add_argument(
        "--id",
        dest="explicit_id",
        help="Assign a specific ID instead of generating the next free one",
    )
    parser.add_argument(
        "--hostname",
        help="Assign a specific hostname. Default is to use the assigned device ID.",
    )
    parser.add_argument(
        "--ip",
        help="Target a specific device IP",
    )
    parser.add_argument(
        "--hwid",
        help="Target a specific hardware_id",
    )
    parser.add_argument(
        "--prefix",
        default=DEFAULT_PREFIX,
        help=f"ID prefix (default: {DEFAULT_PREFIX})",
    )
    parser.add_argument(
        "--digits",
        type=int,
        default=DEFAULT_DIGITS,
        help=f"Numeric width for generated IDs (default: {DEFAULT_DIGITS})",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Allow selecting already provisioned devices for hostname updates/checks",
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if not args.list and not args.assign_next and not args.explicit_id:
        print("Nothing to do. Use --list, --assign-next, or --id.")
        return 1

    devices = scan_subnet(args.subnet, args.port, args.timeout, args.workers)

    if args.list:
        print_devices(devices)
        if not args.assign_next and not args.explicit_id:
            return 0

    if args.assign_next or args.explicit_id:
        target = choose_target(
            devices=devices,
            target_ip=args.ip,
            target_hwid=args.hwid,
            only_unprovisioned=not args.force,
        )

        if target is None:
            if args.ip:
                matching = [d for d in devices if d.ip == args.ip]
                if matching:
                    d = matching[0]
                    if d.provisioned and d.device_id and not args.force:
                        print(
                            f"Device {d.ip} is already provisioned "
                            f"(device_id={d.device_id}, hostname={d.effective_hostname or d.hostname or '-'}) "
                            "and was skipped. Use --force to target it for hostname checks/updates."
                        )
                        return 2

            print("No unique target device found.")
            return 2

        if args.explicit_id:
            new_id = args.explicit_id
        else:
            new_id = next_free_id(devices, args.prefix, args.digits)

        hostname = args.hostname if args.hostname else new_id

        if not is_valid_hostname(hostname):
            print(f"Invalid hostname: {hostname}")
            return 5

        current_hostname = target.effective_hostname or target.hostname or ""

        if not target.provisioned or not target.device_id:
            print(
                f"Provisioning {target.ip} with id={new_id}, hostname={hostname} "
                f"(hardware_id={target.hardware_id or 'unknown'})"
            )

            ok, detail = provision_device(target.ip, args.port, args.timeout, new_id, hostname)
            if not ok:
                print(f"Provisioning failed. {detail}")
                return 4

            print("Provisioning succeeded.")
            print("Reboot the device to apply Wi-Fi hostname and mDNS name.")
            return 0

        if target.device_id != new_id:
            print(
                f"Device {target.ip} is already provisioned with device_id={target.device_id}. "
                "Changing device_id is not allowed through provisioning."
            )
            return 6

        if current_hostname == hostname:
            print(
                f"Device {target.ip} already has device_id={target.device_id} "
                f"and hostname={current_hostname}. Nothing to do."
            )
            return 0

        print(
            f"Updating hostname on {target.ip} from {current_hostname or '-'} to {hostname} "
            f"(device_id={target.device_id})"
        )

        ok, detail = update_hostname(target.ip, args.port, args.timeout, hostname)
        if not ok:
            print(f"Hostname update failed. {detail}")
            return 7

        print("Hostname update succeeded.")
        print("Reboot the device to apply Wi-Fi hostname and mDNS name.")
        return 0

    return 0


if __name__ == "__main__":
    sys.exit(main())


# python assign_id.py --subnet 192.168.1.0/24 --list
# python assign_id.py --subnet 192.168.1.0/24 --assign-next --ip 192.168.1.55
# python assign_id.py --subnet 192.168.1.0/24 --id esp32-007 --ip 192.168.1.55
# python assign_id.py --subnet 192.168.1.0/24 --id esp32-001 --hostname livingroom-sensor --ip 192.168.1.233 --force