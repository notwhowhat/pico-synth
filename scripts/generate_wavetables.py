import numpy as np
from scipy.signal import square, sawtooth

from typing import Callable
from textwrap import dedent

# three types of wavetables:
# square, sin and sawtooth (triangle will come, but i'm too lazy to implement it now)

TABLE_LENGTH: int = 360
TABLE_AMPLITUDE: float = 1.0
ONE_LINE: bool = False

def generate_table(waveform: Callable, length: int, amplitude: float) -> np.ndarray:
    table: np.ndarray = np.zeros((length))
    increment: float = 2.0 * np.pi / length

    for i in range(length):
        table[i] = waveform(i * increment)

    return table * amplitude

def sin_wave(sample: float) -> float:
    return np.sin(sample)

def square_wave(sample: float) -> float:
    return square(sample)

def sawtooth_wave(sample: float) -> float:
    return sawtooth(sample)

def format_table(name: str, table: np.ndarray) -> str:
    top: str = f'float {name}_table {{'
    bottom: str = '};'

    values: str = ''
    for sample in table:
        value: str = f'\t{str(sample)},'
        if not ONE_LINE:
            value.join('\n')
        values += value

    output: str = f'{top}\n{values}\n{bottom}\n'
    return output
    

def main() -> None:
    sin_table = generate_table(sin_wave, TABLE_LENGTH, TABLE_AMPLITUDE)
    square_table = generate_table(square_wave, TABLE_LENGTH, TABLE_AMPLITUDE)
    sawtooth_table = generate_table(sawtooth_wave, TABLE_LENGTH, TABLE_AMPLITUDE)

    print(format_table('sin', sin_table))
    print(format_table('square', square_table))
    print(format_table('sawtooth', sawtooth_table))

    with open('wavetables.h', 'w') as f:
        table: str = format_table('sin', sin_table)
        print(table)
        f.write(table)
        
        table: str = format_table('square', square_table)
        print(table)
        f.write(table)
        
        table: str = format_table('sawtooth', sawtooth_table)
        print(table)
        f.write(table)

if __name__ == '__main__':
    main()
