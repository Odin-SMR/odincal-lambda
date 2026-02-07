from datetime import UTC, datetime
from pathlib import Path
from typing import Any

import numpy as np


class Level0File:
    dtype: np.dtype[Any] | None = None
    sequence_mask: int | None = None

    def __init__(self, file: Path):
        self._created = datetime.now(UTC)
        self._file = file
        if self.dtype is None or self.sequence_mask is None:
            raise NotImplementedError("parameters must be defined in subclass")
        self._file = file
        overflow = int(file.stem, 16) >> 28  # last 4 cut off in filename
        full_file_data = np.fromfile(file, dtype=self.dtype)
        block_id: np.ndarray[tuple[int], np.dtype[np.uint16]] = (
            full_file_data["user"] & self.sequence_mask
        )
        start_idx, end_idx = self._find_full_blocks(block_id)
        self.data = full_file_data[start_idx : end_idx + 1]
        block_idx = np.where(self.data["user"] & self.sequence_mask == 0)[0]
        self._stw: np.ndarray[tuple[int], np.dtype[np.uint64]] = self.data["stw"][
            block_idx
        ].astype(np.uint64) + (overflow << 32)
        self._backend: np.ndarray[tuple[int], np.dtype[np.uint16]] = self.data["user"][
            block_idx
        ]
        self._words = self.data["words"].reshape(
            (-1, np.max(block_id) + 1, self.data["words"].shape[1])
        )

    def _find_full_blocks(
        self,
        block_id: np.ndarray[tuple[int], np.dtype[np.uint16]],
    ) -> tuple[int, int]:
        start_idx = np.where(block_id == 0)[0]
        end_idx = np.where(block_id == np.max(block_id))[0]

        return start_idx[0], end_idx[-1]

    @property
    def words(self) -> np.ndarray[tuple[int, int, int], np.dtype[np.uint16]]:
        return self._words

    @property
    def backend(self) -> np.ndarray[tuple[int], np.dtype[np.uint16]]:
        return self._backend

    @property
    def stw(self) -> np.ndarray[tuple[int], np.dtype[np.uint64]]:
        return self._stw
