import numpy as np

from typing import Callable
from textwrap import dedent

# three types of wavetables:
# square, sin and sawtooth (triangle will come, but i'm too lazy to implement it now)

TABLE_LENGTH: int = 128
ONE_LINE: bool = False

def generate_table(length: int) -> np.ndarray:
    table: np.ndarray = np.zeros((128))
    for i in range(len(table)):
        table[i] = 2 ** ((i - 69) / 12) * 440

        
    return table

def format_table(table: np.ndarray) -> str:
    top: str = f'float frequencies[] {{'
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
    frequencies = generate_table(TABLE_LENGTH)
    print(frequencies[69])

    with open('frequencies.h', 'w') as f:
        table: str = format_table(frequencies)
        print(table)
        f.write(table)

if __name__ == '__main__':
    main()
