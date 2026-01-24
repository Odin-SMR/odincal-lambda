from pathlib import Path

from .filetypes import os_dt
from .odinfile import Level0File


class OSFile(Level0File):
    dtype = os_dt
    sequence_mask = 0

    def __init__(self, file: Path):
        super().__init__(file)
