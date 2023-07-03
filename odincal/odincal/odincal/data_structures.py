'''
Describing datastructures.

HK -     is the structure HouseKeeping format.
AC -     ac1 and ac2 file format.
ACData - the autocorelator information.

These structures are used to create the database tables. Changing theese
definitions will make you responsible to change the database structure.
'''
from ctypes import LittleEndianStructure, c_ushort, c_uint, c_short


class HK(LittleEndianStructure):
    _pack_ = 2
    _fields_ = [
        ('sync', c_ushort),
        ('stw', c_uint),
        ('backend', c_ushort),
        ('ps2', c_ushort),  # HK47 (2,3)
        ('aos', (c_ushort * 4)),  # (2,(4,5,6,7))
        ('os', (c_ushort * 3)),  # (5,(5,6,7))
        ('pyrod2', c_ushort),  # HK46 (2,2)
        ('pyro1', c_ushort),  # HK4C (11,3)
        ('pyro1d', c_ushort),  # HK48 (11,1)
        ('ps3', c_ushort),  # HK4D (11,4)
        ('ps1', c_ushort),  # HK49 (11,2)
        ('temp1', c_ushort),  # HK40 (11,7)
        ('temp2', c_ushort),  # HK41 (11,8)
        ('rwss', c_uint),  # HK42 (11,(9,10))
        ('plla', (c_ushort * 8)),  # (11,11)
        ('pllb', (c_ushort * 8)),  # (11,(19)
        ('mecha', (c_ushort * 6)),  # (11,27)
        ('mechb', (c_ushort * 6)),  # (11,33)
        ('mecha_sync', c_ushort),  # ACDC sync word (0,3)
        ('mechb_sync', c_ushort),  # ACS avail (0,4)
        ('acdc1_sync', c_ushort),  # HK pyro d5 (0,1)
        ('acdc2_sync', c_ushort),  # HK4A (0,2)
        ('ghz119_sync', c_ushort),  # (0,5)
        ('subcom', c_ushort),  # HK20 (9,(1,2,3))
        ('oscreg', c_uint),  # HK21,HK22
        ('magxz', c_ushort),  # HK44 (12,(1,2)
        ('magyr', c_ushort),  # HK45
        ('acdc1_slow', c_ushort),  # ACDC slow (12,15)
        ('acdc2_slow', c_ushort),  # ACDC slow (12,16)
        ('ocs_peek', c_ushort),  # HK30  (13,(1,2,3)
        ('hk31', c_ushort),  # HK31
        ('stw_comp', c_ushort),  # HK32
        ('Gyro1', c_ushort),  # gyro 1 (13,4)
        ('Gyro2', c_ushort),  # gyro 2 (13,10
        ('Gyro3', c_ushort),  # gyro 3 (13,16
        ('ibat1', c_ushort),  # HK4E (14,39)
        ('ibat2', c_ushort),  # HK4F (14,40)
        ('nodata', (c_ushort * 2)),
        ('index', c_short, 8),
        ('acdc', c_short, 4),
        ('nospare', c_short, 2),
        ('astro', c_short, 1),
        ('valid', c_short, 1),
        ('nodummy', (c_ushort * 3)),
    ]


class AC(LittleEndianStructure):
    _pack_ = 2
    _fields_ = [
        ('sync', c_ushort),
        ('stw', c_uint),
        ('backend', c_ushort),
        # data starts
        ('nodata0', (c_ushort * 2)),
        ('counter', c_ushort),
        ('nodata0', (c_ushort * 5)),
        ('chopper_pos', c_ushort),
        ('nodata2', (c_ushort * 3)),
        ('int_time', c_ushort),
        ('nodata3', (c_ushort * 3)),
        ('acd_mon', (c_ushort * 16)),
        # config copy starts
        # 08
        ('mask3', c_short, 8),
        ('mask2', c_short, 8),
        # 09
        ('mask5', c_short, 8),
        ('mask4', c_short, 8),
        # 0A
        ('mask6', c_ushort),
        # 0B
        ('inttime', c_short, 8),
        ('ccmxset', c_short, 8),
        # 0C
        ('switch', c_short, 1),
        ('sim', c_short, 1),
        ('nop', c_short, 6),
        ('mxm', c_short, 4),
        ('nop2', c_short, 4),
        # 0D
        ('ssb1_att', c_ushort),
        # 0E
        ('ssb2_att', c_ushort),
        # 0F
        ('ssb3_att', c_ushort),
        # 10
        ('ssb4_att', c_ushort),
        # 11
        ('ssb4_fq', c_ushort),
        # 12
        ('ssb3_fq', c_ushort),
        # 13
        ('ssb2_fq', c_ushort),
        # 14
        ('ssb1_fq', c_ushort),
        # 15
        ('useraddress', c_ushort),
        # 16
        ('r', c_short, 8),
        ('s', c_short, 8),
        # ('noconfig',(c_ushort*17)),
        ('noconfig', (c_ushort * 2)),
        ('prescaler', c_ushort),
        ('lags', (c_short * 8)),
        ('noconfig2', (c_ushort * 6)),
        ('index', c_short, 8),
        ('acdc', c_short, 4),
        ('nospare', c_short, 2),
        ('astro', c_short, 1),
        ('valid', c_short, 1),
        ('nodummy', (c_ushort * 6)),
    ]


class ACData(LittleEndianStructure):
    _pack_ = 2
    _fields_ = [
        ('sync', c_ushort),
        ('stw', c_uint),
        ('user', c_ushort),
        ('userdata', (c_short * 64)),
        ('index', c_short, 8),
        ('acdc', c_short, 4),
        ('nospare', c_short, 2),
        ('astro', c_short, 1),
        ('valid', c_short, 1),
        ('dummy', (c_ushort * 6)),
    ]
