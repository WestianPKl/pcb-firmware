import struct

INFO_ADDR = 0x080FF800
INFO_MAGIC = 0x494E464F

serial = 0x12345678
hw_major = 1
hw_minor = 0
prod_date = b"2026-01-29"

blob = bytearray([0xFF] * 2048)

data = struct.pack(
    "<IIBB2s10s", INFO_MAGIC, serial, hw_major, hw_minor, b"\x00\x00", prod_date
)

blob[0 : len(data)] = data

with open("info_page.bin", "wb") as f:
    f.write(blob)

print("Wrote info_page.bin (2KB)")
