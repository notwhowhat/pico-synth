#import numpy as np
import numpy as np
from matplotlib.pyplot import plot, savefig
from matplotlib import use

use("Agg")

path = "/mnt/c/Users/jakob/coding-related/micontroller-projects/npicosyn"

raw_arr = np.genfromtxt(
    path + "/build/tests/plot.csv",
    delimiter=",",
    dtype=int
)
arr = np.array([raw_arr[0][0:-1], raw_arr[1][0:-1]])
#print(raw_arr[0][0:-1])
plot(arr[0], arr[1], )
print("generating image...")
savefig(path + "/scripts/plot.png")
print("done!")
