"""Extract just the scan IDs from input
"""
from typing import Any


def handler(event: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
    scan_ids: set[int] = set()
    for fm in event["ScansInfo"]:
        scan_ids = scan_ids.union({scan["ScanID"] for scan in fm["ScansInfo"]})

    if len(scan_ids) == 0:
        return {
            "StatusCode": 204,
            "ScanIDs": [],
        }
    return {
        "StatusCode": 200,
        "ScanIDs": list(scan_ids),
    }
