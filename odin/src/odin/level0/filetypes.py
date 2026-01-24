import numpy as np

ac_dt = np.dtype(
    [
        ("sync", np.uint16),
        ("stw", np.uint32),
        ("user", np.uint16),
        ("words", np.uint16, (64,)),
        ("index", np.uint16),
        ("fill", np.uint16, (6,)),
    ]
)
ac1_dt = ac_dt
ac2_dt = ac_dt

shk_dt = np.dtype(
    [
        ("sync", np.uint16),
        ("stw", np.uint32),
        ("user", np.uint16),
        ("words", np.uint16, (1 + 4 + 3 + 9 + 8 + 8 + 6 + 6 + 20)),
        ("index", np.uint16),
        ("fill", np.uint16, (5,)),
    ]
)

os_dt = np.dtype(
    [
        ("sync", np.uint16),
        ("stw", np.uint32),
        ("user", np.uint16),
        ("words", np.uint16, (156,)),
        ("index", np.uint16),
        ("fill", np.uint16, (4,)),
    ]
)
aos_dt = np.dtype(
    [
        ("sync", np.uint16),
        ("stw", np.uint32),
        ("user", np.uint16),
        ("words", np.uint16, (112,)),
        ("index", np.uint16),
        ("fill", np.uint16, (3,)),
    ]
)

fba_dt = np.dtype(
    [
        ("sync", np.uint16),
        ("stw", np.uint32),
        ("user", np.uint16),
        ("words", np.uint16, (7,)),
        ("index", np.uint16),
        ("fill", np.uint16, (3,)),
    ]
)

att_ut = np.dtype(
    [
        ("date", np.uint64),
        ("hour", np.uint32),
        ("minute", np.uint32),
        ("second", np.float32),
    ]
)

att_att = np.dtype(
    [
        ("q0", np.float64),
        ("q1", np.float64),
        ("q2", np.float64),
        ("q3", np.float64),
    ]
)
att_arcs = np.dtype(
    [
        ("dx", np.float32),
        ("dy", np.float32),
        ("dz", np.float32),
    ]
)
att_gps = np.dtype(
    dtype=[
        ("x", np.float64),
        ("y", np.float64),
        ("z", np.float64),
        ("vx", np.float32),
        ("vy", np.float32),
        ("vz", np.float32),
    ]
)
att_pos = np.dtype(
    [
        ("lat", np.float32),
        ("lon", np.float32),
        ("alt", np.float32),
    ]
)
att_dt = np.dtype(
    [
        ("ut", att_ut),  # 0-3
        ("stw", np.uint64),  # 4
        ("orbit", np.float64),  # 5
        ("qt", att_att),  # 6-9
        ("qa", att_att),  # 10-13
        ("qe", att_arcs),  # 14-16
        ("gps", att_gps),  # 17-22
        ("pos_os", att_pos),  # 23-25
        ("pos_smr", att_pos),  # 26-28
        ("quality", np.uint16),  # 29
        ("sci", np.uint16),  # 30
        ("placs", att_pos),  # 31-33
        ("mode", np.uint16),  # 34
        ("st_sep", np.float64),  # 35
        ("acs", np.float64),  # 36
    ]
)
