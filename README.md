# sonic-dx-se-unpacker
Sonic DX SE Sound Unpacker

## Build
```bash
make
```

## How to use
**warning**: Destination folder must exist
```bash
# Just read DAT file header
# ./bin/se-unpack.exe read_header <source_file>
./bin/se-unpack.exe read_header /c/Games/SonicDX/system/sounddata/se/CART_BANK01.dat

# Unpack WAVE data from DAT files
# ./bin/se-unpack.exe unpack <source_file> <destination_folder>
./bin/se-unpack.exe unpack /c/Games/SonicDX/system/sounddata/se/CART_BANK01.dat ~/sonic-se-banks
```

Windows CMD command to unpack all banks
```
for %i in (C:\Games\SonicDX\system\sounddata\se\*.dat) do (
    C:\git\sonic-dx-se-unpacker\bin\se-unpack.exe unpack %i C:\Users\User\sonic-se-banks
)
```