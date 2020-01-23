import modulePython.dna as dna
from modulePython.read_mtx import *

import numpy as np
from scipy.sparse import *
import scipy.sparse.linalg as spLnal
import time

dampingPath = "matrixFEM/damping(1).mtx"
stiffnessPath = "matrixFEM/stiffness(1).mtx"

dampingPath = "matrixFEM/damping.mtx"
stiffnessPath = "matrixFEM/stiffness.mtx"

dampingPath = "matrixSymDefPos/362_5,786_plat362.mtx"
stiffnessPath = "matrixSymDefPos/362_5,786_plat362.mtx"

# dampingPath = "matrixTest/small.mtx"
# stiffnessPath = "matrixTest/small1.mtx"


S = dna.ReadFromFile(stiffnessPath)
d_S = dna.D_SparseMatrix(S, True)
# d_S = dna.D_SparseMatrix(S.rows, S.cols)

d_S.ConvertMatrixToCSR()
print("Stiffness matrix loaded ...")

D = dna.ReadFromFile(dampingPath)
d_D = dna.D_SparseMatrix(D, True)
d_D.ConvertMatrixToCSR()
print("Dampness matrix loaded ...")

U = np.random.rand(d_S.cols)
# # U = np.ones(d_S.cols)
tau = 0.01
epsilon = 1e-3

d_U = dna.D_Array(len(U))
d_U.Fill(U)


start = time.time()
V1 = dna.DiffusionTest(d_S, d_D, tau, d_U, epsilon)
V1.Print()

solve1Time = time.time() - start

start = time.time()
M = dna.D_SparseMatrix(d_D.rows, d_D.cols)
dna.MatrixSum(d_D, d_S, -tau, M)
# M.Print()
d_DU = d_D.Dot(d_U)
dna.SolveConjugateGradient(M, d_DU, epsilon)

solve2Time = time.time() - start


print("Run Time 1:", solve1Time)
print("Run Time 2:", solve2Time)
