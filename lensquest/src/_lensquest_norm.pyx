# coding: utf-8

import numpy as np
cimport numpy as np
from libcpp.string cimport string
from libc.math cimport sqrt, floor, fabs
from libcpp.vector cimport vector
cimport libc
import os
import time
import cython
from libcpp cimport bool as cbool

from _common cimport tsize, arr, PowSpec, ndarray2cl1, ndarray2cl4, ndarray2cl6
								
def cltype(cl):
	if type(cl)==np.ndarray:
		if cl.ndim==1:
			return 1
		else:
			return np.shape(cl)[0]
	elif type(cl) is tuple or list:
		return len(cl)
	else:
		return 0
							   
cdef extern from "_lensquest_cxx.cpp":
	cdef void makeA(PowSpec &wcl, PowSpec &dcl, PowSpec &al, int lmin, int lmax, int lminCMB)
	cdef vector[ vector[double] ] makeA_syst(PowSpec &wcl, PowSpec &dcl, PowSpec &rdcls, PowSpec &al, int lmin, int lmax, int lminCMB, int type)
	cdef vector[ vector[double] ] makeA_dust(PowSpec &wcl, PowSpec &dcl1, PowSpec &dcl2, PowSpec &R1, PowSpec &R2, PowSpec &rdcls, PowSpec &al, int lmin, int lmax, int lminCMB)
	cdef vector[ vector[ vector[ vector[ vector[ vector[double] ] ] ] ] ] makeX_dust(PowSpec &wcl, vector[ vector[ vector[double] ] ] & dcl, vector[ vector[ vector[ vector[double] ] ] ] & rd, vector[ vector[ vector[ vector[double] ] ] ] & al, vector[ vector[ vector[vector[double] ] ] ] & rlnu, int lmin, int lmax, int lminCMB1, int lminCMB2, int lmaxCMB1, int lmaxCMB2)
	cdef vector[ vector[double] ] makeAN(PowSpec &wcl, PowSpec &dcl, PowSpec &ncl, PowSpec &rdcls, PowSpec &al, int lmin, int lmax, int lminCMB1, int lminCMB2, int lmaxCMB1, int lmaxCMB2)
	cdef vector[ vector[double] ] makeAN_RD(PowSpec &wcl, PowSpec &dcl, PowSpec &ncl, PowSpec &rdcls, PowSpec &al, int lmin, int lmax, int lminCMB1, int lminCMB2, int lmaxCMB1, int lmaxCMB2)
	cdef vector[ vector[ vector[double] ] ] makeAN_RD_iterSims(PowSpec &wcl, PowSpec &dcl, PowSpec &ncl, vector[PowSpec *] rdcls, PowSpec &al, int lmin, int lmax, int lminCMB1, int lminCMB2, int lmaxCMB1, int lmaxCMB2, int Nsims)
	cdef vector[ vector[ vector[ vector[double] ] ] ] makeAN_syst(PowSpec &wcl, PowSpec &dcl,  int lmin, int lmax, int lminCMB1, int lminCMB2, int lmaxCMB1, int lmaxCMB2)
	cdef vector[ vector[double] ] makeA_BH(string stype, PowSpec &wcl, PowSpec &dcl, int lmin, int lmax, int lminCMB)
	cdef vector[ vector[double] ] computeKernel(string stype, PowSpec &wcl, PowSpec &dcl, int lminCMB, int L)
	cdef void lensCls(PowSpec& llcl, PowSpec& ulcl,vector[double] &clDD) 
	cdef void systCls(PowSpec& llcl, PowSpec& ulcl,vector[double] &clDD, int type) 
	cdef vector[double] lensBB(vector[double] &clEE, vector[double] &clDD, int lmax_out, cbool even)

def quest_norm(wcl, dcl, ncl=None, lmin=2, lmax=None, lminCMB=2, lmaxCMB=None, lminCMB2=None, lmaxCMB2=None, rdcl=None, bias=False):
	"""Computes the norm of the quadratic estimator.

	Parameters
	----------
	wcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The input power spectra (lensed or unlensed) used in the weights of the normalization a la Okamoto & Hu, either TT only or TT, EE, BB and TE (polarization).
	dcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The (noisy) input power spectra used in the denominators of the normalization (i.e. Wiener filtering), either TT only or TT, EE, BB and TE (polarization).
	ncl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The (noisy) input power spectra used in the denominators of the normalization (i.e. Wiener filtering), either TT only or TT, EE, BB and TE (polarization).
	lmin : int, scalar, optional
	  Minimum l of the normalization. Default: 2
	lmax : int, scalar, optional
	  Maximum l of the normalization. Default: lmaxCMB
	lminCMB : int, scalar, optional
	  Minimum l of the CMB power spectra. Default: 2
	lmaxCMB : int, scalar, optional
	  Maximum l of the CMB power spectra. Default: given by input cl arrays
	bias: bool, scalar, optional
	  Additionally computing the N0 bias. Default: False
	
	Returns
	-------
	AL: array or tuple of arrays
	  Normalization for 1 (TT) or 5 (TT,TE,EE,TB,EB) quadratic estimators.
	"""
	
	cdef int nspec, nspecout

	nspec=cltype(wcl)
	
	if nspec!=cltype(dcl) or nspec<1: raise ValueError("The two power spectra arrays must be of same type and size")
	if rdcl is not None:
		if nspec!=cltype(rdcl): raise ValueError("The third power spectrum array must be of same type and size")
	if nspec!=1 and nspec!=4: raise NotImplementedError("Power spectra must be given in an array of 1 (TT) or 4 (TT,EE,BB,TE) spectra")
	if bias and nspec!=4: raise NotImplementedError("Need polarization spectra for bias computation")
	 
	cdef int lmin_, lmax_, lminCMB1_, lminCMB2_, lmaxCMB_, lmaxCMB1_, lmaxCMB2_
	lmin_=lmin
	lminCMB1_=lminCMB
	if lmaxCMB is None:
		if nspec==1:
			lmaxCMB=len(wcl)-1
		else:
			lmaxCMB=len(wcl[0])-1

	lmaxCMB_=lmaxCMB
	lmaxCMB1_=lmaxCMB
		
	if lmaxCMB2 is None:
		lmaxCMB2_=lmaxCMB_
	else:
		lmaxCMB2_=lmaxCMB2
		lmaxCMB_=np.max([lmaxCMB,lmaxCMB2])
	
	if lminCMB2 is None:
		lminCMB2_=lminCMB1_
	else:
		lminCMB2_=lminCMB2
		
	
	if lmax is None:
		lmax_=lmaxCMB_
	else:
		lmax_=lmax
		
	if nspec==1:
		wcl_c = np.ascontiguousarray(wcl[:lmaxCMB_+1], dtype=np.float64)
		dcl_c = np.ascontiguousarray(dcl[:lmaxCMB_+1], dtype=np.float64)
		nspec=1
	elif nspec==4:
		wcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in wcl]
		dcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in dcl]
		if rdcl is not None and bias: rdcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in rdcl]
		elif bias: rdcl_c=dcl_c
		if ncl is not None and bias: ncl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in ncl]
		elif bias: ncl_c=dcl_c
		nspec=4
	
	nspecout=0

	if nspec==1:
		wcl_=ndarray2cl1(wcl_c, lmaxCMB_)
		dcl_=ndarray2cl1(dcl_c, lmaxCMB_)
		nspecout=1
	elif nspec==4:
		wcl_=ndarray2cl4(wcl_c[0], wcl_c[1], wcl_c[2], wcl_c[3], lmaxCMB_)
		dcl_=ndarray2cl4(dcl_c[0], dcl_c[1], dcl_c[2], dcl_c[3], lmaxCMB_)
		if bias: 
			rdcl_=ndarray2cl4(rdcl_c[0], rdcl_c[1], rdcl_c[2], rdcl_c[3], lmaxCMB_)
			ncl_=ndarray2cl4(ncl_c[0], ncl_c[1], ncl_c[2], ncl_c[3], lmaxCMB_)
		nspecout=6

	cdef PowSpec *al_=new PowSpec(nspecout,lmax_)
	cdef vector[ vector[double] ] bias_

	if bias:
		bias_=makeAN(wcl_[0], dcl_[0], ncl_[0], rdcl_[0], al_[0], lmin_, lmax_, lminCMB1_, lminCMB2_,lmaxCMB1_, lmaxCMB2_)
		del rdcl_
	else:
		makeA(wcl_[0], dcl_[0], al_[0], lmin_, lmax_, lminCMB1_)
	
	del wcl_, dcl_
		
	if nspecout==1:
		al={}
		if bias: nl={}
		al["TT"]= np.zeros(lmax_+1, dtype=np.float64)
		for l in xrange(lmin,lmax_+1):
			al["TT"][l]=al_.tt(l)
		if bias: nl["TTTT"]=al["TT"]
	elif nspecout==6:
		al={}
		if bias: nl={}
		al["TT"] = np.zeros(lmax_+1, dtype=np.float64)
		al["TE"] = np.zeros(lmax_+1, dtype=np.float64)
		al["EE"] = np.zeros(lmax_+1, dtype=np.float64)
		al["TB"] = np.zeros(lmax_+1, dtype=np.float64)
		al["EB"] = np.zeros(lmax_+1, dtype=np.float64)
		if bias: 
			nl["TTTT"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TTTE"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TTEE"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TETE"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TEEE"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["EEEE"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TBTB"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TBEB"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["EBEB"]=np.zeros(lmax_+1, dtype=np.float64)
		for l in xrange(lmin,lmax_+1):
			al["TT"][l]=al_.tt(l)
			al["TE"][l]=al_.tg(l)
			al["EE"][l]=al_.gg(l)
			al["TB"][l]=al_.tc(l)
			al["EB"][l]=al_.gc(l)
			if bias: 
				nl["TTTT"][l]=bias_[0][l]
				nl["TTTE"][l]=bias_[1][l]
				nl["TTEE"][l]=bias_[2][l]
				nl["TETE"][l]=bias_[3][l]
				nl["TEEE"][l]=bias_[4][l]
				nl["EEEE"][l]=bias_[5][l]
				nl["TBTB"][l]=bias_[6][l]
				nl["TBEB"][l]=bias_[7][l]
				nl["EBEB"][l]=bias_[8][l]
				
	del al_
		
	if bias: 
		return al,nl
	else: 
		return al


def quest_norm_RD(wcl, dcl, ncl=None, lmin=2, lmax=None, lminCMB=2, lmaxCMB=None, lminCMB2=None, lmaxCMB2=None, rdcl=None, bias=True):
	"""Computes the analytical realization-dependent N0 of the quadratic estimator.

	Parameters
	----------
	wcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The input power spectra (lensed or unlensed) used in the weights of the normalization a la Okamoto & Hu, either TT only or TT, EE, BB and TE (polarization).
	dcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The mean power spectra from simulations (including foreground residuals), either TT only or TT, EE, BB and TE (polarization).
	ncl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The (noisy) input power spectra used in the denominators of the normalization (i.e. Wiener filtering), either TT only or TT, EE, BB and TE (polarization).
	lmin : int, scalar, optional
	  Minimum l of the normalization. Default: 2
	lmax : int, scalar, optional
	  Maximum l of the normalization. Default: lmaxCMB
	lminCMB : int, scalar, optional
	  Minimum l of the CMB power spectra. Default: 2
	lmaxCMB : int, scalar, optional
	  Maximum l of the CMB power spectra. Default: given by input cl arrays
	bias: bool, scalar, optional
	  Additionally computing the N0 bias. Default: True.
	  It has no effect and it is left just to keep the format of quest_norm function.
	
	Returns
	-------
	AL: array or tuple of arrays
	  Normalization for 1 (TT) or 5 (TT,TE,EE,TB,EB) quadratic estimators.
	"""
	
	bias=True # The bias must be True in order to compute the realization-dependent N0. 
	cdef int nspec, nspecout

	nspec=cltype(wcl)
	
	if nspec!=cltype(dcl) or nspec<1: raise ValueError("The two power spectra arrays must be of same type and size")
	if rdcl is not None:
		if nspec!=cltype(rdcl): raise ValueError("The third power spectrum array must be of same type and size")
	if nspec!=1 and nspec!=4: raise NotImplementedError("Power spectra must be given in an array of 1 (TT) or 4 (TT,EE,BB,TE) spectra")
	if bias and nspec!=4: raise NotImplementedError("Need polarization spectra for bias computation")
	 
	cdef int lmin_, lmax_, lminCMB1_, lminCMB2_, lmaxCMB_, lmaxCMB1_, lmaxCMB2_
	lmin_=lmin
	lminCMB1_=lminCMB
	if lmaxCMB is None:
		if nspec==1:
			lmaxCMB=len(wcl)-1
		else:
			lmaxCMB=len(wcl[0])-1

	lmaxCMB_=lmaxCMB
	lmaxCMB1_=lmaxCMB
		
	if lmaxCMB2 is None:
		lmaxCMB2_=lmaxCMB_
	else:
		lmaxCMB2_=lmaxCMB2
		lmaxCMB_=np.max([lmaxCMB,lmaxCMB2])
	
	if lminCMB2 is None:
		lminCMB2_=lminCMB1_
	else:
		lminCMB2_=lminCMB2
		
	
	if lmax is None:
		lmax_=lmaxCMB_
	else:
		lmax_=lmax
		
	if nspec==1:
		wcl_c = np.ascontiguousarray(wcl[:lmaxCMB_+1], dtype=np.float64)
		dcl_c = np.ascontiguousarray(dcl[:lmaxCMB_+1], dtype=np.float64)
		nspec=1
	elif nspec==4:
		wcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in wcl]
		dcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in dcl]
		if rdcl is not None and bias: rdcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in rdcl]
		elif bias: rdcl_c=dcl_c
		if ncl is not None and bias: ncl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in ncl]
		elif bias: ncl_c=dcl_c
		nspec=4
	
	nspecout=0

	if nspec==1:
		wcl_=ndarray2cl1(wcl_c, lmaxCMB_)
		dcl_=ndarray2cl1(dcl_c, lmaxCMB_)
		nspecout=1
	elif nspec==4:
		wcl_=ndarray2cl4(wcl_c[0], wcl_c[1], wcl_c[2], wcl_c[3], lmaxCMB_)
		dcl_=ndarray2cl4(dcl_c[0], dcl_c[1], dcl_c[2], dcl_c[3], lmaxCMB_)
		if bias: 
			rdcl_=ndarray2cl4(rdcl_c[0], rdcl_c[1], rdcl_c[2], rdcl_c[3], lmaxCMB_)
			ncl_=ndarray2cl4(ncl_c[0], ncl_c[1], ncl_c[2], ncl_c[3], lmaxCMB_)
		nspecout=6

	cdef PowSpec *al_=new PowSpec(nspecout,lmax_)
	cdef vector[ vector[double] ] bias_

	if bias:
		bias_=makeAN_RD(wcl_[0], dcl_[0], ncl_[0], rdcl_[0], al_[0], lmin_, lmax_, lminCMB1_, lminCMB2_,lmaxCMB1_, lmaxCMB2_)
		del rdcl_
	else:
		makeA(wcl_[0], dcl_[0], al_[0], lmin_, lmax_, lminCMB1_)
	
	del wcl_, dcl_
		
	if nspecout==1:
		al={}
		if bias: nl={}
		al["TT"]= np.zeros(lmax_+1, dtype=np.float64)
		for l in xrange(lmin,lmax_+1):
			al["TT"][l]=al_.tt(l)
		if bias: nl["TTTT"]=al["TT"]
	elif nspecout==6:
		al={}
		if bias: nl={}
		al["TT"] = np.zeros(lmax_+1, dtype=np.float64)
		al["TE"] = np.zeros(lmax_+1, dtype=np.float64)
		al["EE"] = np.zeros(lmax_+1, dtype=np.float64)
		al["TB"] = np.zeros(lmax_+1, dtype=np.float64)
		al["EB"] = np.zeros(lmax_+1, dtype=np.float64)
		if bias: 
			nl["TTTT"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TTTE"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TTEE"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TETE"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TEEE"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["EEEE"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TBTB"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["TBEB"]=np.zeros(lmax_+1, dtype=np.float64)
			nl["EBEB"]=np.zeros(lmax_+1, dtype=np.float64)
		for l in xrange(lmin,lmax_+1):
			al["TT"][l]=al_.tt(l)
			al["TE"][l]=al_.tg(l)
			al["EE"][l]=al_.gg(l)
			al["TB"][l]=al_.tc(l)
			al["EB"][l]=al_.gc(l)
			if bias: 
				nl["TTTT"][l]=bias_[0][l]
				nl["TTTE"][l]=bias_[1][l]
				nl["TTEE"][l]=bias_[2][l]
				nl["TETE"][l]=bias_[3][l]
				nl["TEEE"][l]=bias_[4][l]
				nl["EEEE"][l]=bias_[5][l]
				nl["TBTB"][l]=bias_[6][l]
				nl["TBEB"][l]=bias_[7][l]
				nl["EBEB"][l]=bias_[8][l]
				
	del al_
		
	if bias: 
		return al,nl
	else: 
		return al

def quest_norm_RD_iterSims(wcl, dcl, ncl, rdcl, lmin=2, lmax=None, lminCMB=2, lmaxCMB=None, lminCMB2=None, lmaxCMB2=None):
	"""Computes the analytical realization-dependent N0 of the quadratic estimator over all the simulations in a efficient way.

	Parameters
	----------
	wcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The input power spectra (lensed or unlensed) used in the weights of the normalization a la Okamoto & Hu, either TT only or TT, EE, BB and TE (polarization).
	dcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The mean power spectra from simulations (including foreground residuals), either TT only or TT, EE, BB and TE (polarization).
	ncl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The (noisy) input power spectra used in the denominators of the normalization (i.e. Wiener filtering), either TT only or TT, EE, BB and TE (polarization).
	lmin : int, scalar, optional
	  Minimum l of the normalization. Default: 2
	lmax : int, scalar, optional
	  Maximum l of the normalization. Default: lmaxCMB
	lminCMB : int, scalar, optional
	  Minimum l of the CMB power spectra. Default: 2
	lmaxCMB : int, scalar, optional
	  Maximum l of the CMB power spectra. Default: given by input cl arrays
	rdcl : array-like, shape (4, Nsims, lmaxCMB), optional
	  The realization-dependent input power spectra used in the denominators of the normalization (i.e. Wiener filtering),
	   either TT only or TT, EE, BB and TE (polarization).
	
	Returns
	-------
	AL: array or tuple of arrays
	  Normalization for 1 (TT) or 5 (TT,TE,EE,TB,EB) quadratic estimators.
	"""
	
	cdef int nspec, nspecout

	nspec=cltype(wcl)
	
	if nspec!=cltype(dcl) or nspec<1: raise ValueError("The two power spectra arrays must be of same type and size")
	if rdcl is None: raise NotImplementedError("Realization dependent power spectra must be given")
	if rdcl is not None:
		if nspec!=cltype(rdcl): raise ValueError("The third power spectrum array must be of same type and size")
	if nspec!=4: raise NotImplementedError("Power spectra must be given in an array of 4 (TT,EE,BB,TE) spectra")

	# st = time.time()	 
	cdef int lmin_, lmax_, lminCMB1_, lminCMB2_, lmaxCMB_, lmaxCMB1_, lmaxCMB2_, Nsims_
	Nsims_ = rdcl.shape[1]
	cdef vector[PowSpec *] rdcl_
	rdcl_.resize(Nsims_)

	lmin_=lmin
	lminCMB1_=lminCMB
	if lmaxCMB is None:
		lmaxCMB=len(wcl[0])-1

	lmaxCMB_=lmaxCMB
	lmaxCMB1_=lmaxCMB
		
	if lmaxCMB2 is None:
		lmaxCMB2_=lmaxCMB_
	else:
		lmaxCMB2_=lmaxCMB2
		lmaxCMB_=np.max([lmaxCMB,lmaxCMB2])
	
	if lminCMB2 is None:
		lminCMB2_=lminCMB1_
	else:
		lminCMB2_=lminCMB2
		
	
	if lmax is None:
		lmax_=lmaxCMB_
	else:
		lmax_=lmax
	
	wcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in wcl]
	ncl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in ncl]
	dcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in dcl]
	nspec=4
	
	nspecout=0
	# print(f"Time 1:", time.strftime("%H:%M:%S", time.gmtime(time.time() - st)))

	wcl_=ndarray2cl4(wcl_c[0], wcl_c[1], wcl_c[2], wcl_c[3], lmaxCMB_)
	ncl_=ndarray2cl4(ncl_c[0], ncl_c[1], ncl_c[2], ncl_c[3], lmaxCMB_)
	dcl_=ndarray2cl4(dcl_c[0], dcl_c[1], dcl_c[2], dcl_c[3], lmaxCMB_)
	Nsims_ = rdcl.shape[1] 
	for i in xrange(Nsims_):
		rdcl_c = [np.ascontiguousarray(cl[i, :lmaxCMB_+1], dtype=np.float64) for cl in rdcl]
		rdcl_[i]=ndarray2cl4(rdcl_c[0], rdcl_c[1], rdcl_c[2], rdcl_c[3], lmaxCMB_)
	# print(f"Time 2:", time.strftime("%H:%M:%S", time.gmtime(time.time() - st)))
		
	nspecout=6

	cdef PowSpec *al_=new PowSpec(nspecout,lmax_)
	cdef vector[ vector[ vector[double] ] ] bias_

	bias_=makeAN_RD_iterSims(wcl_[0], dcl_[0], ncl_[0], rdcl_, al_[0], lmin_, lmax_, lminCMB1_, lminCMB2_,lmaxCMB1_, lmaxCMB2_, Nsims_)
	
	del wcl_, dcl_
	
	# print(f"Time 3:", time.strftime("%H:%M:%S", time.gmtime(time.time() - st)))

	al={}
	nl={}

	al["TT"] = np.zeros(lmax_+1, dtype=np.float64)
	al["TE"] = np.zeros(lmax_+1, dtype=np.float64)
	al["EE"] = np.zeros(lmax_+1, dtype=np.float64)
	al["TB"] = np.zeros(lmax_+1, dtype=np.float64)
	al["EB"] = np.zeros(lmax_+1, dtype=np.float64)

	nl["TTTT"]=np.zeros((Nsims_, lmax_+1), dtype=np.float64)
	nl["TTTE"]=np.zeros((Nsims_, lmax_+1), dtype=np.float64)
	nl["TTEE"]=np.zeros((Nsims_, lmax_+1), dtype=np.float64)
	nl["TETE"]=np.zeros((Nsims_, lmax_+1), dtype=np.float64)
	nl["TEEE"]=np.zeros((Nsims_, lmax_+1), dtype=np.float64)
	nl["EEEE"]=np.zeros((Nsims_, lmax_+1), dtype=np.float64)
	nl["TBTB"]=np.zeros((Nsims_, lmax_+1), dtype=np.float64)
	nl["TBEB"]=np.zeros((Nsims_, lmax_+1), dtype=np.float64)
	nl["EBEB"]=np.zeros((Nsims_, lmax_+1), dtype=np.float64)

	for l in xrange(lmin,lmax_+1):
		al["TT"][l]=al_.tt(l)
		al["TE"][l]=al_.tg(l)
		al["EE"][l]=al_.gg(l)
		al["TB"][l]=al_.tc(l)
		al["EB"][l]=al_.gc(l)
		for n in range(Nsims_):
			nl["TTTT"][n, l]=bias_[0][n][l]
			nl["TTTE"][n, l]=bias_[1][n][l]
			nl["TTEE"][n, l]=bias_[2][n][l]
			nl["TETE"][n, l]=bias_[3][n][l]
			nl["TEEE"][n, l]=bias_[4][n][l]
			nl["EEEE"][n, l]=bias_[5][n][l]
			nl["TBTB"][n, l]=bias_[6][n][l]
			nl["TBEB"][n, l]=bias_[7][n][l]
			nl["EBEB"][n, l]=bias_[8][n][l]
	# print(f"Time 4:", time.strftime("%H:%M:%S", time.gmtime(time.time() - st)))
				
	del al_
		
	return al, nl


def quest_norm_syst(typenum, wcl, dcl, lmin=2, lmax=None, lminCMB=2, lmaxCMB=None, lminCMB2=None, lmaxCMB2=None, rdcl=None):
	"""Computes the norm of the quadratic estimator.

	Parameters
	----------
	wcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The input power spectra (lensed or unlensed) used in the weights of the normalization a la Okamoto & Hu, either TT only or TT, EE, BB and TE (polarization).
	dcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The (noisy) input power spectra used in the denominators of the normalization (i.e. Wiener filtering), either TT only or TT, EE, BB and TE (polarization).
	lmin : int, scalar, optional
	  Minimum l of the normalization. Default: 2
	lmax : int, scalar, optional
	  Maximum l of the normalization. Default: lmaxCMB
	lminCMB : int, scalar, optional
	  Minimum l of the CMB power spectra. Default: 2
	lmaxCMB : int, scalar, optional
	  Maximum l of the CMB power spectra. Default: given by input cl arrays
	bias: bool, scalar, optional
	  Additionally computing the N0 bias. Default: False
	
	Returns
	-------
	AL: array or tuple of arrays
	  Normalization for 1 (TT) or 5 (TT,TE,EE,TB,EB) quadratic estimators.
	"""
	
	cdef int nspec, nspecout

	nspec=cltype(wcl)
	
	if nspec!=cltype(dcl) or nspec<1: raise ValueError("The two power spectra arrays must be of same type and size")
	if nspec!=1 and nspec!=4: raise NotImplementedError("Power spectra must be given in an array of 1 (TT) or 4 (TT,EE,BB,TE) spectra")

	
	cdef int lmin_, lmax_, lminCMB1_, lminCMB2_, lmaxCMB_, lmaxCMB1_, lmaxCMB2_, type_
	lmin_=lmin
	lminCMB1_=lminCMB
	if lmaxCMB is None:
		if nspec==1:
			lmaxCMB=len(wcl)-1
		else:
			lmaxCMB=len(wcl[0])-1

	lmaxCMB_=lmaxCMB
	lmaxCMB1_=lmaxCMB
		
	if lmaxCMB2 is None:
		lmaxCMB2_=lmaxCMB_
	else:
		lmaxCMB2_=lmaxCMB2
		lmaxCMB_=np.max([lmaxCMB,lmaxCMB2])
	
	if lminCMB2 is None:
		lminCMB2_=lminCMB1_
	else:
		lminCMB2_=lminCMB2
		
	
	if lmax is None:
		lmax_=lmaxCMB_
	else:
		lmax_=lmax
		
	if typenum is None:
		corr=True
	else:
		type_=typenum
		corr=False
		

	wcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in wcl]
	dcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in dcl]
	if rdcl is not None: 
		rdcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in rdcl]
	else:
		rdcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in dcl]
	nspec=4
	

	wcl_=ndarray2cl4(wcl_c[0], wcl_c[1], wcl_c[2], wcl_c[3], lmaxCMB_)
	dcl_=ndarray2cl4(dcl_c[0], dcl_c[1], dcl_c[2], dcl_c[3], lmaxCMB_)
	rdcl_=ndarray2cl4(rdcl_c[0], rdcl_c[1], rdcl_c[2], rdcl_c[3], lmaxCMB_)
	
	nspecout=6

	cdef PowSpec *al_=new PowSpec(nspecout,lmax_)
	cdef vector[ vector[double] ] bias_
	cdef vector[ vector[ vector[ vector[double] ] ] ] corrbias_

	if corr:
		corrbias_=makeAN_syst(wcl_[0], dcl_[0], lmin_, lmax_, lminCMB1_, lminCMB2_,lmaxCMB1_, lmaxCMB2_)
		
		del wcl_, dcl_
		
		nl={}
		dists=['a','o','g1','g2','f1','f2','p1','p2','d1','d2','q']
		
		for i,s1 in enumerate(dists):
			nl[s1]={}
			for j,s2 in enumerate(dists):
				nl[s1][s2]={}
				nl[s1][s2]["TE"]=np.zeros(lmax_+1, dtype=np.float64)
				nl[s1][s2]["EE"]=np.zeros(lmax_+1, dtype=np.float64)
				nl[s1][s2]["TB"]=np.zeros(lmax_+1, dtype=np.float64)
				nl[s1][s2]["EB"]=np.zeros(lmax_+1, dtype=np.float64)
				for l in xrange(lmin,lmax_+1):
					if j>i:
						nl[s1][s2]["TE"][l]=corrbias_[l][j][i][0]
						nl[s1][s2]["EE"][l]=corrbias_[l][j][i][1]
						nl[s1][s2]["TB"][l]=corrbias_[l][j][i][2]
						nl[s1][s2]["EB"][l]=corrbias_[l][j][i][3]
					else:
						nl[s1][s2]["TE"][l]=corrbias_[l][i][j][0]
						nl[s1][s2]["EE"][l]=corrbias_[l][i][j][1]
						nl[s1][s2]["TB"][l]=corrbias_[l][i][j][2]
						nl[s1][s2]["EB"][l]=corrbias_[l][i][j][3]

		
		return nl
	else:
		bias_=makeA_syst(wcl_[0], dcl_[0], rdcl_[0], al_[0], lmin_, lmax_, lminCMB1_, type_)
		
		del wcl_, dcl_, rdcl_

		al={}
		nl={}
		al["TE"] = np.zeros(lmax_+1, dtype=np.float64)
		al["EE"] = np.zeros(lmax_+1, dtype=np.float64)
		al["TB"] = np.zeros(lmax_+1, dtype=np.float64)
		al["EB"] = np.zeros(lmax_+1, dtype=np.float64)
		nl["TETE"]=np.zeros(lmax_+1, dtype=np.float64)
		nl["TEEE"]=np.zeros(lmax_+1, dtype=np.float64)
		nl["EEEE"]=np.zeros(lmax_+1, dtype=np.float64)
		nl["TBTB"]=np.zeros(lmax_+1, dtype=np.float64)
		nl["TBEB"]=np.zeros(lmax_+1, dtype=np.float64)
		nl["EBEB"]=np.zeros(lmax_+1, dtype=np.float64)
		for l in xrange(lmin,lmax_+1):
			al["TE"][l]=al_.tg(l)
			al["EE"][l]=al_.gg(l)
			al["TB"][l]=al_.tc(l)
			al["EB"][l]=al_.gc(l)
			nl["TETE"][l]=bias_[0][l]
			nl["TEEE"][l]=bias_[1][l]
			nl["EEEE"][l]=bias_[2][l]
			nl["TBTB"][l]=bias_[3][l]
			nl["TBEB"][l]=bias_[4][l]
			nl["EBEB"][l]=bias_[5][l]
					
		del al_
			
		return {'A':al,'N':nl}

def quest_norm_dust(wcl, dcl1, dcl2, r1, r2, rdcl, lmin=2, lmax=None, lminCMB=2, lmaxCMB=None, lminCMB2=None, lmaxCMB2=None):
	"""Computes the norm of the quadratic estimator.

	Parameters
	----------
	wcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The input power spectra (lensed or unlensed) used in the weights of the normalization a la Okamoto & Hu, either TT only or TT, EE, BB and TE (polarization).
	dcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The (noisy) input power spectra used in the denominators of the normalization (i.e. Wiener filtering), either TT only or TT, EE, BB and TE (polarization).
	lmin : int, scalar, optional
	  Minimum l of the normalization. Default: 2
	lmax : int, scalar, optional
	  Maximum l of the normalization. Default: lmaxCMB
	lminCMB : int, scalar, optional
	  Minimum l of the CMB power spectra. Default: 2
	lmaxCMB : int, scalar, optional
	  Maximum l of the CMB power spectra. Default: given by input cl arrays
	bias: bool, scalar, optional
	  Additionally computing the N0 bias. Default: False
	
	Returns
	-------
	AL: array or tuple of arrays
	  Normalization for 1 (TT) or 5 (TT,TE,EE,TB,EB) quadratic estimators.
	"""
	
	cdef int nspec, nspecout

	nspec=cltype(wcl)
	
	if nspec!=cltype(dcl1) or nspec<1: raise ValueError("The two power spectra arrays must be of same type and size")
	if nspec!=1 and nspec!=4: raise NotImplementedError("Power spectra must be given in an array of 1 (TT) or 4 (TT,EE,BB,TE) spectra")

	
	cdef int lmin_, lmax_, lminCMB1_, lminCMB2_, lmaxCMB_, lmaxCMB1_, lmaxCMB2_, type_
	lmin_=lmin
	lminCMB1_=lminCMB
	if lmaxCMB is None:
		if nspec==1:
			lmaxCMB=len(wcl)-1
		else:
			lmaxCMB=len(wcl[0])-1

	lmaxCMB_=lmaxCMB
	lmaxCMB1_=lmaxCMB
		
	if lmaxCMB2 is None:
		lmaxCMB2_=lmaxCMB_
	else:
		lmaxCMB2_=lmaxCMB2
		lmaxCMB_=np.max([lmaxCMB,lmaxCMB2])
	
	if lminCMB2 is None:
		lminCMB2_=lminCMB1_
	else:
		lminCMB2_=lminCMB2
		
	
	if lmax is None:
		lmax_=lmaxCMB_
	else:
		lmax_=lmax

		

	wcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in wcl]
	dcl1_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in dcl1]
	dcl2_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in dcl2]
	r1_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in r1]
	r2_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in r2]
	rdcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in rdcl]
	
	nspec=4
	

	wcl_=ndarray2cl4(wcl_c[0], wcl_c[1], wcl_c[2], wcl_c[3], lmaxCMB_)
	dcl1_=ndarray2cl4(dcl1_c[0], dcl1_c[1], dcl1_c[2], dcl1_c[3], lmaxCMB_)
	dcl2_=ndarray2cl4(dcl2_c[0], dcl2_c[1], dcl2_c[2], dcl2_c[3], lmaxCMB_)
	r1_=ndarray2cl4(r1_c[0], r1_c[1], r1_c[2], 0*r1_c[0], lmaxCMB_)
	r2_=ndarray2cl4(r2_c[0], r2_c[1], r2_c[2], 0*r2_c[0], lmaxCMB_)
	rdcl_=ndarray2cl4(rdcl_c[0], rdcl_c[1], rdcl_c[2], rdcl_c[3], lmaxCMB_)
	
	nspecout=6

	cdef PowSpec *al_=new PowSpec(nspecout,lmax_)
	cdef vector[ vector[double] ] bias_

	bias_=makeA_dust(wcl_[0], dcl1_[0], dcl2_[0], r1_[0], r2_[0], rdcl_[0], al_[0], lmin_, lmax_, lminCMB1_)
	
	del wcl_, dcl1_, dcl2_, r1_, r2_, rdcl_

	al={}
	nl={}
	al["TT"] = np.zeros(lmax_+1, dtype=np.float64)
	al["TE"] = np.zeros(lmax_+1, dtype=np.float64)
	al["EE"] = np.zeros(lmax_+1, dtype=np.float64)
	al["TB"] = np.zeros(lmax_+1, dtype=np.float64)
	al["EB"] = np.zeros(lmax_+1, dtype=np.float64)
	al["BB"] = np.zeros(lmax_+1, dtype=np.float64)
	nl["TTTT"]=np.zeros(lmax_+1, dtype=np.float64)
	nl["TTTE"]=np.zeros(lmax_+1, dtype=np.float64)
	nl["TTEE"]=np.zeros(lmax_+1, dtype=np.float64)
	nl["TETE"]=np.zeros(lmax_+1, dtype=np.float64)
	nl["TEEE"]=np.zeros(lmax_+1, dtype=np.float64)
	nl["EEEE"]=np.zeros(lmax_+1, dtype=np.float64)
	nl["TBTB"]=np.zeros(lmax_+1, dtype=np.float64)
	nl["TBEB"]=np.zeros(lmax_+1, dtype=np.float64)
	nl["EBEB"]=np.zeros(lmax_+1, dtype=np.float64)
	nl["BBBB"]=np.zeros(lmax_+1, dtype=np.float64)
	for l in xrange(lmin,lmax_+1):
		al["TT"][l]=al_.tt(l)
		al["TE"][l]=al_.tg(l)
		al["EE"][l]=al_.gg(l)
		al["TB"][l]=al_.tc(l)
		al["EB"][l]=al_.gc(l)
		al["BB"][l]=al_.cc(l)
		nl["TTTT"][l]=bias_[0][l]
		nl["TTTE"][l]=bias_[1][l]
		nl["TTEE"][l]=bias_[2][l]
		nl["TETE"][l]=bias_[3][l]
		nl["TEEE"][l]=bias_[4][l]
		nl["EEEE"][l]=bias_[5][l]
		nl["TBTB"][l]=bias_[6][l]
		nl["TBEB"][l]=bias_[7][l]
		nl["EBEB"][l]=bias_[8][l]
		nl["BBBB"][l]=bias_[9][l]
				
				
	del al_
		
	return {'A':al,'N':nl}

	
def quest_norm_dust_x(wcl, dcl, rd, rlnu, lmin=2, lmax=None, lminCMB=2, lmaxCMB=None, lminCMB2=None, lmaxCMB2=None):
	cdef int nspec, nspecout, nfreq
	
	nspec=cltype(wcl)
	
	nfreq=len(dcl)
	
	if nspec!=1 and nspec!=4: raise NotImplementedError("Power spectra must be given in an array of 1 (TT) or 4 (TT,EE,BB,TE) spectra")
	 
	cdef int lmin_, lmax_, lminCMB1_, lminCMB2_, lmaxCMB_, lmaxCMB1_, lmaxCMB2_
	lmin_=lmin
	lminCMB1_=lminCMB
	if lmaxCMB is None:
		if nspec==1:
			lmaxCMB=len(wcl)-1
		else:
			lmaxCMB=len(wcl[0])-1

	lmaxCMB_=lmaxCMB
	lmaxCMB1_=lmaxCMB
		
	if lmaxCMB2 is None:
		lmaxCMB2_=lmaxCMB_
	else:
		lmaxCMB2_=lmaxCMB2
		lmaxCMB_=np.max([lmaxCMB,lmaxCMB2])
	
	if lminCMB2 is None:
		lminCMB2_=lminCMB1_
	else:
		lminCMB2_=lminCMB2
	
	if lmax is None:
		lmax_=lmaxCMB_
	else:
		lmax_=lmax
		
	if nspec==1:
		wcl_c = np.ascontiguousarray(wcl[:lmaxCMB_+1], dtype=np.float64)
		nspec=1
	elif nspec==4:
		wcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in wcl]
		nspec=4
	
	nspecout=0

	if nspec==1:
		wcl_=ndarray2cl1(wcl_c, lmaxCMB_)
		nspecout=1
	elif nspec==4:
		wcl_=ndarray2cl4(wcl_c[0], wcl_c[1], wcl_c[2], wcl_c[3], lmaxCMB_)
		nspecout=6

	cdef vector[ vector[ vector[ vector[double] ] ] ] al_
	al_=np.zeros((lmax_+1,nfreq,nfreq,6),dtype=np.float64)
	
	cdef vector[ vector[ vector[ vector[ vector[ vector[double] ] ] ] ] ] bias_
	cdef vector[ vector[ vector[ vector[double] ] ] ] rd_
	cdef vector[ vector[ vector[double] ] ] dcl_
	cdef vector[ vector[ vector[ vector[double]] ] ]  rlnu_
	
	rd_=rd
	dcl_=dcl
	rlnu_=rlnu

	bias_=makeX_dust(wcl_[0], dcl_, rd_, al_, rlnu_, lmin_, lmax_, lminCMB1_, lminCMB2_,lmaxCMB1_, lmaxCMB2_)

	del wcl_

	A={}
	A['TT']=np.moveaxis(np.array(al_)[:,:,:,0], 0, -1)
	A['TE']=np.moveaxis(np.array(al_)[:,:,:,1], 0, -1)
	A['EE']=np.moveaxis(np.array(al_)[:,:,:,2], 0, -1)
	A['TB']=np.moveaxis(np.array(al_)[:,:,:,3], 0, -1)
	A['EB']=np.moveaxis(np.array(al_)[:,:,:,4], 0, -1)
	A['BB']=np.moveaxis(np.array(al_)[:,:,:,5], 0, -1)

	N={}
	N['TTTT']=np.moveaxis(np.array(bias_)[:,:,:,:,:,0], 0, -1)
	N['TTTE']=np.moveaxis(np.array(bias_)[:,:,:,:,:,1], 0, -1)
	N['TTEE']=np.moveaxis(np.array(bias_)[:,:,:,:,:,2], 0, -1)
	N['TETE']=np.moveaxis(np.array(bias_)[:,:,:,:,:,3], 0, -1)
	N['TEEE']=np.moveaxis(np.array(bias_)[:,:,:,:,:,4], 0, -1)
	N['EEEE']=np.moveaxis(np.array(bias_)[:,:,:,:,:,5], 0, -1)
	N['TBTB']=np.moveaxis(np.array(bias_)[:,:,:,:,:,6], 0, -1)
	N['TBEB']=np.moveaxis(np.array(bias_)[:,:,:,:,:,7], 0, -1)
	N['EBEB']=np.moveaxis(np.array(bias_)[:,:,:,:,:,8], 0, -1)
	N['BBBB']=np.moveaxis(np.array(bias_)[:,:,:,:,:,9], 0, -1)
	
	return A,N

class quest_kernel:
	def __init__(self, type, wcl, dcl, lminCMB=2, lmaxCMB=None):
		self.type=type
		self.wcl=wcl
		self.dcl=dcl
		self.lminCMB=lminCMB
		self.lmaxCMB=lmaxCMB
		
	def get_kernel(self,L):
		return computekernel(L,self.type,self.wcl,self.dcl,lminCMB=self.lminCMB,lmaxCMB=self.lmaxCMB)
	
	
def computekernel(L,type,wcl,dcl,lminCMB=2,lmaxCMB=None):
	cdef int lminCMB_, L_
	nspec=cltype(wcl)
	
	cdef string type_
	type_=type.encode()
	
	if nspec!=cltype(dcl) or nspec<1: raise ValueError("The two power spectra arrays must be of same type and size")
	if nspec!=1 and nspec!=4: raise NotImplementedError("Power spectra must be given in an array of 1 (TT) or 4 (TT,EE,BB,TE) spectra")
	
	lminCMB_=lminCMB
	if lmaxCMB is None:
		if nspec==1:
			lmaxCMB=len(wcl)-1
		else:
			lmaxCMB=len(wcl[0])-1
	
	if nspec==1:
		wcl_c = np.ascontiguousarray(wcl[:lmaxCMB+1], dtype=np.float64)
		dcl_c = np.ascontiguousarray(dcl[:lmaxCMB+1], dtype=np.float64)
		wcl_=ndarray2cl1(wcl_c, lmaxCMB)
		dcl_=ndarray2cl1(dcl_c, lmaxCMB)
		nspec=1
	elif nspec==4:
		wcl_c = [np.ascontiguousarray(cl[:lmaxCMB+1], dtype=np.float64) for cl in wcl]
		dcl_c = [np.ascontiguousarray(cl[:lmaxCMB+1], dtype=np.float64) for cl in dcl]
		wcl_=ndarray2cl4(wcl_c[0], wcl_c[1], wcl_c[2], wcl_c[3], lmaxCMB)
		dcl_=ndarray2cl4(dcl_c[0], dcl_c[1], dcl_c[2], dcl_c[3], lmaxCMB)
		nspec=4

	cdef vector[ vector[double] ] kernel_
	L_=L
	
	kernel_=computeKernel(type_,wcl_[0], dcl_[0], lminCMB_, L_)
	del wcl_, dcl_
		
	kernel= np.zeros((lmaxCMB+1,lmaxCMB+1), dtype=np.float64)
	for l1 in xrange(lmaxCMB+1):
		for l2 in xrange(lmaxCMB+1):
			kernel[l1][l2]=kernel_[l1][l2]
				
	return kernel
	

def quest_norm_bh(type, wcl, dcl, lmin=2, lmax=None, lminCMB=2, lmaxCMB=None):
	"""Computes the norm of the quadratic estimator.

	Parameters
	----------
	wcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The input power spectra (lensed or unlensed) used in the weights of the normalization a la Okamoto & Hu, either TT only or TT, EE, BB and TE (polarization).
	dcl : array-like, shape (1,lmaxCMB) or (4, lmaxCMB)
	  The (noisy) input power spectra used in the denominators of the normalization (i.e. Wiener filtering), either TT only or TT, EE, BB and TE (polarization).
	lmin : int, scalar, optional
	  Minimum l of the normalization. Default: 2
	lmax : int, scalar, optional
	  Maximum l of the normalization. Default: lmaxCMB
	lminCMB : int, scalar, optional
	  Minimum l of the CMB power spectra. Default: 2
	lmaxCMB : int, scalar, optional
	  Maximum l of the CMB power spectra. Default: given by input cl arrays
	bias: bool, scalar, optional
	  Additionally computing the N0 bias. Default: False
	
	Returns
	-------
	AL: array or tuple of arrays
	  Normalization for 1 (TT) or 5 (TT,TE,EE,TB,EB) quadratic estimators.
	"""
	
	cdef int nspec, nspecout
	cdef string type_
	type_=type.encode()
	
	nspec=cltype(wcl)
	
	if nspec!=cltype(dcl) or nspec<1: raise ValueError("The two power spectra arrays must be of same type and size")
	if nspec!=1 and nspec!=4: raise NotImplementedError("Power spectra must be given in an array of 1 (TT) or 4 (TT,EE,BB,TE) spectra")
	 
	cdef int lmin_, lmax_, lminCMB_, lmaxCMB_
	lmin_=lmin
	lminCMB_=lminCMB
	if lmaxCMB is None:
		if nspec==1:
			lmaxCMB_=len(wcl)-1
		else:
			lmaxCMB_=len(wcl[0])-1
	else:
		lmaxCMB_=lmaxCMB
	if lmax is None:
		lmax_=lmaxCMB_
	else:
		lmax_=lmax
		
	num_bh_types=3

	if nspec==1:
		wcl_c = np.ascontiguousarray(wcl[:lmaxCMB_+1], dtype=np.float64)
		dcl_c = np.ascontiguousarray(dcl[:lmaxCMB_+1], dtype=np.float64)
		nspec=1
	elif nspec==4:
		wcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in wcl]
		dcl_c = [np.ascontiguousarray(cl[:lmaxCMB_+1], dtype=np.float64) for cl in dcl]
		nspec=4
	
	nspecout=0

	if nspec==1:
		wcl_=ndarray2cl1(wcl_c, lmaxCMB_)
		dcl_=ndarray2cl1(dcl_c, lmaxCMB_)
		nspecout=1
	elif nspec==4:
		wcl_=ndarray2cl4(wcl_c[0], wcl_c[1], wcl_c[2], wcl_c[3], lmaxCMB_)
		dcl_=ndarray2cl4(dcl_c[0], dcl_c[1], dcl_c[2], dcl_c[3], lmaxCMB_)		
		nspecout=6

	cdef vector[ vector[double] ] al_

	al_=makeA_BH(type_, wcl_[0], dcl_[0], lmin_, lmax_, lminCMB_)
	
	del wcl_, dcl_
	
	al={}
	al["gg"] = np.zeros(lmax_+1, dtype=np.float64)
	al["gm"] = np.zeros(lmax_+1, dtype=np.float64)
	al["gn"] = np.zeros(lmax_+1, dtype=np.float64)
	al["mm"] = np.zeros(lmax_+1, dtype=np.float64)
	al["mn"] = np.zeros(lmax_+1, dtype=np.float64)
	al["nn"] = np.zeros(lmax_+1, dtype=np.float64)
	for l in xrange(lmin,lmax_+1):
		al["gg"][l]=al_[0][l]
		al["gm"][l]=al_[1][l]
		al["gn"][l]=al_[2][l]
		al["mm"][l]=al_[3][l]
		al["mn"][l]=al_[4][l]
		al["nn"][l]=al_[5][l]
	
	return al
	
def lensbb(clee,clpp,lmax=None,even=False):
	lmaxee=len(clee)-1
	lmaxpp=len(clpp)-1
	if lmax is None:
		lmax=lmaxee
	
	cdef lmax_
	lmax_=lmax
	cdef even_
	even_=even
	
	cdef vector[double] clee_ = vector[double](lmaxee+1,0.)
	cdef vector[double] clpp_ = vector[double](lmaxpp+1,0.)

	for l in range(lmaxee+1):
		clee_[l]=clee[l]
		
	for l in range(lmaxpp+1):
		clpp_[l]=clpp[l]
		
	cdef vector[double] clbb_
	
	clbb_=lensBB(clee_,clpp_,lmax_,even_)
		
	out=np.zeros(lmax_+1, dtype=np.float64)
	for l in range(lmax_+1):
		out[l]=clbb_[l]
		
	return out
	
def lenscls(ucl,clpp):
	cdef int lmax_
	
	lmax_ul=len(ucl[0])-1
	lmaxpp=len(clpp)-1

	ucl_c = [np.ascontiguousarray(cl[:lmax_ul+1], dtype=np.float64) for cl in ucl]
	if len(ucl_c)==6: ucl_=ndarray2cl6(ucl_c[0], ucl_c[1], ucl_c[2], ucl_c[3], ucl_c[4], ucl_c[5], lmax_ul)
	else: raise NotImplementedError('Input spectra should be in an array of length 6: %d given'%len(ucl_c))
		
	
	cdef vector[double] clpp_ = vector[double](lmaxpp+1,0.)
	for l in range(lmaxpp+1):
		clpp_[l]=clpp[l]
		
	lmax_=lmax_ul
	
	cdef PowSpec *lcl_=new PowSpec(6,lmax_)
	
	lensCls(lcl_[0], ucl_[0], clpp_)
	
	l=np.arange(len(clpp))
	R=.5*np.sum(l*(l+1)*(2*l+1)/4./np.pi*clpp)
	
	out=np.zeros((6,lmax_+1), dtype=np.float64)
	for l in xrange(2,lmax_+1):
		out[0][l]=lcl_.tt(l)-l*(l+1)*R*ucl[0][l]
		out[1][l]=lcl_.gg(l)-(l**2+l-4)*R*ucl[1][l]
		out[2][l]=lcl_.cc(l)-(l**2+l-4)*R*ucl[2][l]
		out[3][l]=lcl_.tg(l)-(l**2+l-2)*R*ucl[3][l]
		out[4][l]=lcl_.gc(l)-(l**2+l-4)*R*ucl[4][l]
		out[5][l]=lcl_.tc(l)-(l**2+l-2)*R*ucl[5][l]

	return out
	
def systcls(type,ucl,clpp):
	cdef int lmax_
	
	lmax_ul=len(ucl[0])-1
	lmaxpp=len(clpp)-1

	ucl_c = [np.ascontiguousarray(cl[:lmax_ul+1], dtype=np.float64) for cl in ucl]
	if len(ucl_c)==4: ucl_=ndarray2cl4(ucl_c[0], ucl_c[1], ucl_c[2], ucl_c[3], lmax_ul)
	else: raise NotImplementedError('Input spectra should be in an array of length 4: %d given'%len(ucl_c))
		
	cdef int type_
	
	type_=type
	
	cdef vector[double] clpp_ = vector[double](lmaxpp+1,0.)
	for l in range(lmaxpp+1):
		clpp_[l]=clpp[l]
		
	lmax_=lmax_ul
	
	cdef PowSpec *lcl_=new PowSpec(1,lmax_)
	
	systCls(lcl_[0], ucl_[0], clpp_, type_)
	
	l=np.arange(len(clpp))
	R=.5*np.sum(l*(l+1)*(2*l+1)/4./np.pi*clpp)
	
	out=np.zeros(lmax_+1, dtype=np.float64)
	for l in xrange(2,lmax_+1):
		out[l]=lcl_.tt(l)#-(l**2+l-4)*R*ucl[2][l]

	return out
