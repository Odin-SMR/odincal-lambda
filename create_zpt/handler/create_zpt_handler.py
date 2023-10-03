"""Create and store ZPT file for input
"""
from typing import Any


def handler(event: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
    scan_ids = []
    for fm in event["ScansInfo"]:
        scan_ids.extend([id for id in fm["ScanID"]])

    return {
        "StatusCode": 201,
        "ScanIDs": scan_ids,
    }
