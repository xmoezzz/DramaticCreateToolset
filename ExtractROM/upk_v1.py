from sys import argv
import struct
import os


def hexdump(src, length=16, sep='.'):
    FILTER = ''.join([(len(repr(chr(x))) == 3) and chr(x) or sep for x in range(256)])
    lines = []
    for c in xrange(0, len(src), length):
        chars = src[c:c+length]
        hex = ' '.join(["%02x" % ord(x) for x in chars])
        if len(hex) > 24:
            hex = "%s %s" % (hex[:24], hex[24:])
        printable = ''.join(["%s" % ((ord(x) <= 127 and FILTER[ord(x)]) or sep) for x in chars])
        lines.append("%08x:  %-*s  |%s|\n" % (c, length*3, hex, printable))
    print ''.join(lines)

def u8(b, off=0):
    return ord(b[off])

def u16(b, off=0):
    return struct.unpack("<H", b[off:off+2])[0]

def u32(b, off=0):
    return struct.unpack("<I", b[off:off+4])[0]

def c_str(b, off=0):
    part = b[off:]
    term = part.find("\x00")
    if term == -1:
        print "full dump:"
        hexdump(b)
        print "part dump (offset 0x{:x})".format(off)
        hexdump(part)
        raise Exception("failed to find C-string here")
    return part[:term]


folder_map = dict()


class File():

    def __init__(self, data, parent_data):
        name_off = u32(data[0:3] + "\x00")
        self.name = c_str(parent_data, name_off)
        self.flags = u8(data, 3)
        self.offset = u32(data, 4)
        self.size = u32(data, 8)


class Info():

    def __init__(self, data):
        self.num_files = u32(data, 0)

        self.files = []
        for x in range(self.num_files):
            start = 4 + x * 0xC
            file_info = data[start:start+0xC]
            self.files.append(File(file_info, data))

        for file in self.files:
            if file.name == ".":
                folder_map[file.offset] = self
            elif file.name == "..":
                self.parent = folder_map[file.offset]

fin = None


def extract_folder(folder, path):
    os.mkdir(path)
    for file in folder.files:
        if file.name in [".", ".."]:
            continue
        print "{}/{}{}".format(path, file.name, "/" if file.flags & 0x80 else "")
        if file.flags & 0x80:
            extract_folder(folder_map[file.offset], os.path.join(path, file.name))
        else:
            fin.seek(file.offset * 0x800)
            fout = open(os.path.join(path, file.name), "wb")
            fout.write(fin.read(file.size))
            fout.close()


def main():
    global fin
    fin = open(argv[1], "rb")
    assert fin.read(4) == "ROM "
    assert u16(fin.read(2)) == 1
    assert u16(fin.read(2)) == 2
    header_size = u32(fin.read(4))
    unk1 = fin.read(4)

    folders = []

    while fin.tell() < header_size:
        start = fin.tell()
        fin.seek(start + 0xC)
        info_size = u32(fin.read(4))
        fin.seek(start)
        data = fin.read(info_size)
        # hexdump(data)
        info = Info(data)
        folders.append(info)
        
        end = start + info_size
        # align to 0x10
        end = (end + 0xF) & ~0xF
        fin.seek(end)

    extract_folder(folders[0], path="extract")


if __name__ == "__main__":
    main()
