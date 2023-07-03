from ctypes import sizeof
from odincal.data_structures import AC, ACData, HK


def test_len_acd():
    assert sizeof(AC) == 150


def test_len_shkd():
    assert sizeof(HK) == 150


def test_len_acdata():
    assert sizeof(ACData) == 150
