from odincal.level0 import stw_correction


def test_correction():
    for file_set in [
            ('017f6e6c.ac2', 0 * 2**32),
            ('11f1eafe.fba', 1 * 2**32),
            ('20683ad2.ac2', 2 * 2**32),
            ('30683ad2.ac2', 3 * 2**32),
        ]:
        assert stw_correction(file_set[0]) == file_set[1]
