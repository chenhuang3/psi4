import psi4
import numpy as np
import random

mol = psi4.geometry("""
C
C 1 20
""")

primary = psi4.core.BasisSet.build(mol, "ORBITAL", "cc-pVDZ")
aux = psi4.core.BasisSet.build(mol, "ORBITAL", "cc-pVDZ-jkfit")

nbf = primary.nbf()
naux = aux.nbf()

print ("nbf:  ", nbf)
print ("naux: ", naux)

# form metric
mints = psi4.core.MintsHelper(primary)
zero_bas = psi4.core.BasisSet.zero_ao_basis_set()
Jmetric = np.squeeze(mints.ao_eri(zero_bas, aux, zero_bas, aux))

# form inverse metric
Jmetric_inv = mints.ao_eri(zero_bas, aux, zero_bas, aux)
Jmetric_inv.power(-0.5, 1.e-12)
Jmetric_inv = np.squeeze(Jmetric_inv)

# form Qpq
Qpq = np.squeeze(mints.ao_eri(aux, zero_bas, primary, primary))
Qpq = Jmetric_inv.dot(Qpq.reshape(naux, -1))
Qpq = Qpq.reshape(naux, nbf, -1) 
pQq = np.einsum("Qpq->pQq", Qpq)

# space sizes
c1 = 16
c2 = 16
c3 = 20
c4 = 20  
c5 = 24

# space initiations
C1 = psi4.core.Matrix(nbf,c1) 
C2 = psi4.core.Matrix(nbf,c2) 
C3 = psi4.core.Matrix(nbf,c3)
C4 = psi4.core.Matrix(nbf,c4)
C5 = psi4.core.Matrix(nbf,c5)

# get random numpy arrays
C1.np[:] = 1 #np.random.random()
C2.np[:] = 2 #np.random.random()
C3.np[:] = 3 #np.random.random()
C4.np[:] = 4 #np.random.random()
C5.np[:] = 5 #np.random.random()

# left sieve
Cleft = []
Cleft.append(C1)
Cleft.append(C1)
Cleft.append(C2)

Cleft.append(C3)
Cleft.append(C3)

Cleft.append(C5)

# right sieve
Cright = []
Cright.append(C1)
Cright.append(C2)
Cright.append(C2)

Cright.append(C3)
Cright.append(C4)

Cright.append(C5)

# build J
J = []
Qs = []
for i in range(len(Cright)):
    D = np.dot(np.asarray(Cleft[i]), np.asarray(Cright[i]).T)
    Q = np.dot(Qpq.reshape(naux, -1), D.ravel())
    Qs.append(Q)
    J.append(np.dot(Q, Qpq.reshape(-1, nbf*nbf)))
    J[i] = J[i].reshape(nbf, nbf)
    
# build K
K = []
for i in range(len(Cright)):
    Ktmp1 = np.zeros((nbf, naux, np.shape(Cleft[i])[1]))
    Ktmp2 = np.zeros((nbf, naux, np.shape(Cleft[i])[1]))
    for j in range(nbf):
        Ktmp1[j] = np.dot(pQq[j], Cleft[i])
    if(Cleft[i] != Cright[i]):
        for j in range(nbf):
            Ktmp2[j] = np.dot(pQq[j], Cright[i])
    else:
        Ktmp2 = Ktmp1
    K.append(np.dot(Ktmp1.reshape(nbf,-1), Ktmp2.reshape(nbf, -1).T))

# now compare to DF_Helper - test STORE -----------------------------------------
dfh = psi4.core.DF_Helper(primary, aux)

# tweak options
dfh.set_method("STORE")
dfh.set_memory(200000)
dfh.set_on_core(False)
dfh.set_MO_hint(24)

# build
dfh.initialize()

# allocate J
Js = []
Js.append(psi4.core.Matrix(nbf,nbf))
Js.append(psi4.core.Matrix(nbf,nbf))
Js.append(psi4.core.Matrix(nbf,nbf))
Js.append(psi4.core.Matrix(nbf,nbf))
Js.append(psi4.core.Matrix(nbf,nbf))
Js.append(psi4.core.Matrix(nbf,nbf))

# allocate K
Ks = []
Ks.append(psi4.core.Matrix(nbf,nbf))
Ks.append(psi4.core.Matrix(nbf,nbf))
Ks.append(psi4.core.Matrix(nbf,nbf))
Ks.append(psi4.core.Matrix(nbf,nbf))
Ks.append(psi4.core.Matrix(nbf,nbf))
Ks.append(psi4.core.Matrix(nbf,nbf))

# invoke J/K build
dfh.build_JK(Cleft, Cright, Js, Ks)

# practice J
Q = np.zeros((naux))
D = np.dot(np.asarray(C1), np.asarray(C1).T)

# am i right?
for i in range(len(Cright)):
    psi4.compare_arrays(np.asarray(Ks[i]), K[i], 9, "K builds - STORE" )     #TEST
#for i in range(len(Cright)):
#    print (np.linalg.norm(np.asarray(Js[i]).ravel()[:naux]-Qs[i]))     #TEST

Ds = []
for i in range(len(Cright)):
    Ds.append(np.dot(np.asarray(Cleft[i]), np.asarray(Cright[i]).T))
    print (np.linalg.norm(np.asarray(Js[i]) -Ds[i]))

#for i in range(naux):
#    if (i == 80):
#        print ("YUP")
#    print (np.linalg.norm(np.asarray(Js[0]).ravel()[i] -Qs[0][i]))
#psi4.compare_arrays(np.asarray(Js[i]), J[i], 9, "J builds - STORE" )     #TEST


