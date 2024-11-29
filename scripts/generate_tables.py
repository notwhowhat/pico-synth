import numpy as np
import scipy.signal

from matplotlib.pyplot import plot

from typing import Callable
from textwrap import dedent

# three types of wavetables:
# square, sin and sawtooth (triangle will come, but i'm too lazy to implement it now)

WAVETABLE_LENGTH: int = 360
INCTABLE_LENGTH: int = 128
SAMPLE_RATE: int = 44100
ONE_LINE: bool = False

def gen_wavetable(length: int, func: Callable) -> np.ndarray:
    table: np.ndarray = np.zeros(length)
    increment: float = 2.0 * np.pi / length

    for i in range(length):
        table[i] = func(i * increment)

    return table

def gen_inctable(length: int) -> np.ndarray:
    table: np.ndarray = np.zeros(length)

    for i in range(length):
        freq: float = 440.0 * 2.0 ** ((i - 6950.0) / 1200.0)
        table[i] = 360.0 / (SAMPLE_RATE / freq)
        
    return table


def format_table(name: str, table: np.ndarray) -> str:
    out: str = f'float {name}[] = {{'
    if ONE_LINE:
        for e in table:
            out += f'{str(e)},'
    else:
        for e in table:
            out += f'\n{str(e)},'
    out += '};\n'
    return out

def main():
    sin: str = format_table('SIN_TABLE', gen_wavetable(WAVETABLE_LENGTH, np.sin))
    saw: str = format_table('SAW_TABLE', gen_wavetable(WAVETABLE_LENGTH, scipy.signal.sawtooth))
    square: str = format_table('SQUARE_TABLE', gen_wavetable(WAVETABLE_LENGTH, scipy.signal.square))

    increments: str = format_table('INCREMENT_TABLE', gen_inctable(12900))

    with open('tables.h', 'w') as f:
        f.write(sin)
        f.write(saw)
        f.write(square)

        f.write(increments)

if __name__ == '__main__':
    main()
