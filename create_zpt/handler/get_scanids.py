"""Extract just the scan IDs from input
"""
from typing import Any


def handler(event: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
    scan_ids: set[int] = set()
    for fm in event["ScansInfo"]:
        scan_ids = scan_ids.union({scan["ScanID"] for scan in fm["ScanInfo"]})

    return {
        "StatusCode": 200,
        "ScanIDs": list(scan_ids),
    }
