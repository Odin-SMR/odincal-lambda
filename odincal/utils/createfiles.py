from sys import argv
from os.path import getsize, splitext


def main():
    origfile = argv[1]
    size = getsize(origfile)
    orig_fp = open(origfile, 'rb')
    test_fp = open('testfile' + splitext(origfile)[1], 'wb')

    # find a block at the middle of the file
    orig_fp.seek(size / 150 / 2 * 150)
    # read at least 3 whole data blocks
    for _ in range(4):
        chunk = orig_fp.read(150 * 13)
        test_fp.write(chunk)
    orig_fp.close()
    test_fp.close()


if __name__ == "__main__" and len(argv) == 2:
    main()
