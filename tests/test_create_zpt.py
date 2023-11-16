import pytest
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

# Another error collected from logs
scans2 = [
    {
        "StatusCode": 200,
        "ScansInfo": [
            {
                "AltStart": 101198.2,
                "AltEnd": 71997.71,
                "LatStart": -80.69511,
                "LatEnd": -74.56122,
                "LonStart": 255.60405,
                "LonEnd": 155.68942,
                "MJDStart": 60223.4118582,
                "MJDEnd": 60223.4152584,
                "ScanID": 14243177578,
                "FreqMode": 2,
                "Backend": "AC1",
            },
            {
                "AltStart": 101198.2,
                "AltEnd": 71997.71,
                "LatStart": -80.69511,
                "LatEnd": -74.56122,
                "LonStart": 255.60405,
                "LonEnd": 155.68942,
                "MJDStart": 60223.4118582,
                "MJDEnd": 60223.4152584,
                "ScanID": 14243177578,
                "FreqMode": 2,
                "Backend": "AC1",
            },
            {
                "AltStart": 101198.2,
                "AltEnd": 62083.902,
                "LatStart": -80.69511,
                "LatEnd": -82.37251,
                "LonStart": 255.60405,
                "LonEnd": 205.98912,
                "MJDStart": 60223.4118582,
                "MJDEnd": 60223.4130148,
                "ScanID": 14243177578,
                "FreqMode": 21,
                "Backend": "AC1",
            },
            {
                "AltStart": 101198.2,
                "AltEnd": 62083.902,
                "LatStart": -80.69511,
                "LatEnd": -82.37251,
                "LonStart": 255.60405,
                "LonEnd": 205.98912,
                "MJDStart": 60223.4118582,
                "MJDEnd": 60223.4130148,
                "ScanID": 14243177578,
                "FreqMode": 21,
                "Backend": "AC1",
            },
        ],
    }
]


@pytest.mark.parametrize("test_input,expected", [(scans, (1,)), (scans2, (1,))])
def test_get_scan_data(test_input, expected):
    data = get_scan_data(test_input)
    assert expected == data.ScanID.shape
