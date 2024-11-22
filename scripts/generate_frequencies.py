import numpy as np

from typing import Callable
from textwrap import dedent

# three types of wavetables:
# square, sin and sawtooth (triangle will come, but i'm too lazy to implement it now)

CENTS: bool = True
TABLE_LENGTH: int = 128
CENT_TABLE_LENGTH: int = (TABLE_LENGTH + 1) * 100
ONE_LINE: bool = False

def generate_table(length: int) -> np.ndarray:
    if not CENTS:
        table: np.ndarray = np.zeros((128))
        for i in range(len(table)):
            table[i] = 2 ** ((i - 69) / 12) * 440

            
        return table
    else:
        table: np.ndarray = np.zeros((CENT_TABLE_LENGTH))
        for i in range(CENT_TABLE_LENGTH):
            table[i] = 440 * 2 ** ((i - 6950) / 1200)
        
        return table

def format_table(table: np.ndarray) -> str:
    top: str = f'float FREQUENCIES[] = {{'
    bottom: str = '};'

    values: str = ''
    for sample in table:
        value: str = f'\t{str(sample)},'
        if not ONE_LINE:
            value += ('\n')
        values += value

    output: str = f'{top}\n{values}\n{bottom}\n'
    return output
    

def main() -> None:
    frequencies = generate_table(TABLE_LENGTH)
    print(frequencies[6950])

    with open('frequencies.h', 'w') as f:
        table: str = format_table(frequencies)
        #print(table)
        f.write(table)

if __name__ == '__main__':
    main()
