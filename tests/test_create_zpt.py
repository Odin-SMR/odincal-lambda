from create_zpt.handler.create_zpt_handler import get_scan_data

# Dataset from a failed statemachine, is this a split scan?
scans = [
    {"StatusCode": 204, "ScansInfo": []},
    {
        "StatusCode": 200,
        "ScansInfo": [
            {
                "AltStart": 106905.08,
                "AltEnd": 9460.85,
                "LatStart": 71.514114,
                "LatEnd": 61.195534,
                "LonStart": 182.94179,
                "LonEnd": 173.09192,
                "MJDStart": 60244.8251785,
                "MJDEnd": 60244.8267748,
                "ScanID": 14272782480,
                "FreqMode": 21,
                "Backend": "AC1",
            },
        ],
    },
    {"StatusCode": 204, "ScansInfo": []},
    {
        "StatusCode": 200,
        "ScansInfo": [
            {
                "AltStart": 106905.08,
                "AltEnd": 9460.85,
                "LatStart": 71.514114,
                "LatEnd": 61.195534,
                "LonStart": 182.94179,
                "LonEnd": 173.09192,
                "MJDStart": 60244.8251785,
                "MJDEnd": 60244.8267748,
                "ScanID": 14272782480,
                "FreqMode": 21,
                "Backend": "AC1",
            },
        ],
    },
]


def test_get_scan_data():
    data = get_scan_data(scans)
    assert (1,) == data.ScanID.shape
