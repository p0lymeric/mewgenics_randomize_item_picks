from collections import namedtuple
import re
import hashlib
import pefile

MEWGENICS_EXE_PATH = r'C:\Games\Steam\steamapps\common\Mewgenics\Mewgenics.exe'

'''
Run this script to find symbol addresses after a game update.

polymeric 2026
'''

# Signature patterns were made using the Sigga script for Ghidra and hand-adjusted as needed,
# with an acceptable size range of [32, 128] B and preferring to anchor at function start.

DirectSig = namedtuple('DirectSig', ['pattern', 'offset'])
IndirectSig = namedtuple('IndirectSig', ['pattern', 'offset', 'length', 'signed', 'rip_relative'])

# Desired variable name, location descriptor pair.
signatures = {
    # Functions are located by anchoring on starting bytes if possible, but offsets and indirect references may be used if needed
    'ADDRESS_glaiel__MewDirector__always_update': DirectSig('48 8B 05 ?? ?? ?? ?? F2 0F 10 05 ?? ?? ?? ?? 48 FF 81 30 05 00 00 F2 0F 5E 80 C8 0D 00 00 F2 0F 58 81 38 05 00 00', 0),
    'ADDRESS_glaiel__InventoryItemBox__click__lambda_1__Do_call_posttrampoline': DirectSig('40 55 53 56 57 41 54 41 55 41 56 41 57 48 8D 6C 24 E1 48 81 EC E8 00 00 00 4C 8B E9 C7 45 67 00 00 00 00', 0),
    'ADDRESS_SDL_PollEvent': DirectSig('48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 48 89 7C 24 20 41 56 48 81 EC B0 00 00 00 48 85 D2', 0),

    # Data/TLS references are located through references within functions
    'DATAOFF_glaiel__MewDirector__p_singleton': IndirectSig(
        '48 89 5C 24 10 48 89 4C 24 08 57 48 83 EC 40 48 8B CA 48 8B 05 ?? ?? ?? ?? 48 8B B8 A8 05 00 00',
        21, 4, True, True
    ),
}

def hex_pattern_to_bytes_regex(pattern):
    pattern_norm = ''.join(pattern.split())
    bytes_str = b''
    for i in range(0, len(pattern_norm), 2):
        pattern_byte = pattern_norm[i:i+2]
        if pattern_byte == '??':
            bytes_str += b'.'
        else:
            bytes_str += re.escape(bytes([int(pattern_byte, 16)]))
    return re.compile(bytes_str, re.DOTALL)

def main():
    pe = pefile.PE(MEWGENICS_EXE_PATH, fast_load=True)

    with open(MEWGENICS_EXE_PATH, 'rb') as f:
        hash = hashlib.sha256(f.read()).hexdigest()
    print(f'inline constexpr Hash256Bit EXE_SHA256 = c_str_to_hash256bit("{hash}");')

    for search_varname, search_descriptor in signatures.items():
        regexp = hex_pattern_to_bytes_regex(search_descriptor.pattern)
        results = list(re.finditer(regexp, pe.get_memory_mapped_image()))
        if len(results) == 1:
            result_cva = results[0].start()
            result_buffer = results[0].group(0)
            if type(search_descriptor) is DirectSig:
                target_rva = result_cva + search_descriptor.offset
                print(f'inline constexpr uintptr_t {search_varname} = {hex(target_rva)}; // {search_descriptor}')
            else:
                if search_descriptor.offset + search_descriptor.length > len(result_buffer):
                    print(f'// WARNING: indexed past match buffer')
                target_bytes = pe.get_memory_mapped_image()[result_cva+search_descriptor.offset:result_cva+search_descriptor.offset+search_descriptor.length]
                target = int.from_bytes(target_bytes, byteorder='little', signed=search_descriptor.signed)
                if search_descriptor.rip_relative:
                    target += result_cva + search_descriptor.offset + search_descriptor.length
                print(f'inline constexpr uintptr_t {search_varname} = {hex(target)}; // {search_descriptor}')
        elif len(results) > 1:
            print(f'inline constexpr uintptr_t {search_varname} = <MULTIPLE MATCHES>; // {search_descriptor}')
        else:
            print(f'inline constexpr uintptr_t {search_varname} = <NOT FOUND>; // {search_descriptor}')

if __name__ == '__main__':
    main()
