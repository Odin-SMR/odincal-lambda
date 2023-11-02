"""Extract just the scan IDs and frequency modes from input
"""
from typing import Any
from .log_configuration import logconfig

logconfig()


def handler(event: dict[str, Any], context: dict[str, Any]) -> dict[str, Any]:
    scans: set[tuple[int, int]] = set()
    for fm in event["ScansInfo"]:
        scans = scans.union(
            {(scan["ScanID"], scan["FreqMode"]) for scan in fm["ScansInfo"]}
        )

    if len(scans) == 0:
        return {
            "StatusCode": 204,
            "ScanIDs": [],
        }
    return {
        "StatusCode": 200,
        "ScanIDs": [{"ScanID": s[0], "FreqMode": s[1]} for s in scans],
    }
