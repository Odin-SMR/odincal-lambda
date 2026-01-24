from pathlib import Path

from .filetypes import aos_dt
from .odinfile import Level0File


class AOSFile(Level0File):
    dtype = aos_dt
    sequence_mask = 0x000F

    def __init__(self, file: Path):
        super().__init__(file)
