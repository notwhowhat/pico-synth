import numpy as np
import scipy.signal

#from matplotlib.pyplot import plot

from typing import Callable
from textwrap import dedent

# three types of wavetables:
# square, sin and sawtooth (triangle will come, but i'm too lazy to implement it now)

WAVETABLE_LENGTH: int = 1024
NOTE_COUNT: int = 128
INCTABLE_LENGTH: int = (NOTE_COUNT + 1) * 100
SAMPLE_RATE: int = 44100
ONE_LINE: bool = False

def triangle_wave(x: float):
    return np.arcsin(np.sin(2.0 * np.pi * x)) * 2.0 / np.pi

def gen_wavetable(length: int, func: Callable) -> np.ndarray:
    table: np.ndarray = np.zeros(length, dtype=np.int32)
    increment: float = 2.0 * np.pi / length

    for i in range(length):
        table[i] = int(func(i * increment) * 2147483647) # INT_MAX

    return table

def gen_inctable(length: int) -> np.ndarray:
    table: np.ndarray = np.zeros(length, dtype=np.uint32)

    for i in range(length):
        freq: float = 440.0 * 2.0 ** ((i - 6950) / 1200.0)
        table[i] = int(4294967295 * (freq / SAMPLE_RATE)) # UINT_MAX
        
    return table

def gen_keytrack(length: int) -> np.ndarray:
    table: np.ndarray = np.zeros(length, dtype=np.int32)

    for i in range(length):
        table[i] = (440 * 2 ** ((i - 57) / 12)) / SAMPLE_RATE
        
    return table

def format_table(table_type: str, name: str, table: np.ndarray) -> str:
    out: str = f'{table_type} {name}[] = {{'
    if ONE_LINE:
        for e in table:
            out += f'{str(e)},'
    else:
        for e in table:
            out += f'\n{str(e)},'
    out += '};\n'
    return out

def main():
    sin: str = format_table(
        'fixed',
        'SIN_TABLE', 
        gen_wavetable(WAVETABLE_LENGTH, np.sin)
    )
    tri: str = format_table(
        'fixed',
        'TRI_TABLE', 
        gen_wavetable(WAVETABLE_LENGTH, triangle_wave)
    )
    saw: str = format_table(
        'fixed',
        'SAW_TABLE', 
        gen_wavetable(WAVETABLE_LENGTH, scipy.signal.sawtooth)
    )
    square: str = format_table(
        'fixed',
        'SQUARE_TABLE', 
        gen_wavetable(WAVETABLE_LENGTH, scipy.signal.square)
    )

    increments: str = format_table(
        'uint32_t',
        'INCREMENT_TABLE', 
        gen_inctable(INCTABLE_LENGTH)
    )
    keytrack: str = format_table(
        'fixed',
        'KEYTRACK_TABLE', 
        gen_keytrack(128)
    )

    with open('tables.c', 'w') as f:
        f.write('#include "picosyn.h"\n')
        f.write(sin)
        f.write(tri)
        f.write(saw)
        f.write(square)

        f.write(increments)
        f.write(keytrack)

if __name__ == '__main__':
    main()
