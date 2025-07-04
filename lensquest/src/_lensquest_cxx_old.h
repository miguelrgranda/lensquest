#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream> 
#include <cassert>
#include <omp.h>

#include <healpix_map.h>
#include <alm.h>
#include <alm_healpix_tools.h>
#include <libsharp/sharp_cxx.h>
#include <powspec.h>
#include <datatypes.h>

#include "wignerSymbols-cpp.cpp"
#include "kernels.cpp"

#include <Python.h>




void computef(std::vector< std::vector< std::vector<double> > >& f, size_t L, PowSpec& wcl, size_t lminCMB, size_t lmaxCMB, int num_spec){
	
	std::vector< std::vector<double> > F;
	
	if (num_spec==5) {
		F=std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
		compF_phi(F, L, 2, lmaxCMB+1);
	}
	
	std::vector< std::vector<double> > Fz(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
	compF_phi(Fz, L, 0, lmaxCMB+1);
	
	
	if (num_spec==1) {
		#pragma omp parallel for
		for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
			for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
				if ((l1+L+l3)%2==0) {
					f[tt][l1][l3]=wcl.tt(l1)*Fz[l3][l1]+wcl.tt(l3)*Fz[l1][l3];
				}
				else {
					f[tt][l1][l3]=0.0;
				}
			}
		}
	}
	else if (num_spec==5) {
		#pragma omp parallel for
		for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
			for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
				if ((l1+L+l3)%2==0) {
					f[tt][l1][l3]=wcl.tt(l1)*Fz[l3][l1]+wcl.tt(l3)*Fz[l1][l3];
					f[te][l1][l3]=wcl.tg(l1)*F[l3][l1]+wcl.tg(l3)*Fz[l1][l3];
					f[ee][l1][l3]=wcl.gg(l1)*F[l3][l1]+wcl.gg(l3)*F[l1][l3];
					f[tb][l1][l3]=0.0; 
					f[eb][l1][l3]=0.0;
				}
				else {
					f[tt][l1][l3]=0.0;
					f[te][l1][l3]=0.0; 
					f[ee][l1][l3]=0.0; 
					f[tb][l1][l3]=-wcl.tg(l1)*F[l3][l1];
					f[eb][l1][l3]=-wcl.gg(l1)*F[l3][l1]-wcl.cc(l3)*F[l1][l3];
				}
			}
		}
	}
	else std::cout << "I don't know what to do, yet" << std::endl;
}

void computef(int spec, std::vector< std::vector<double> > & f, size_t L, PowSpec& wcl, size_t lminCMB, size_t lmaxCMB){
	
	std::vector< std::vector<double> > F,Fz;
	
	if (spec>=te) {
		F=std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
		compF_phi(F, L, 2, lmaxCMB+1);
	}
	if (spec<=te) {
		Fz=std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
		compF_phi(Fz, L, 0, lmaxCMB+1);
	}
	
	#pragma omp parallel for
	for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
		for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
			if ((l1+L+l3)%2==0) {
				if (spec==tt) f[l1][l3]=wcl.tt(l1)*Fz[l3][l1]+wcl.tt(l3)*Fz[l1][l3];
				else if (spec==te) f[l1][l3]=wcl.tg(l1)*F[l3][l1]+wcl.tg(l3)*Fz[l1][l3];
				else if (spec==ee) f[l1][l3]=wcl.gg(l1)*F[l3][l1]+wcl.gg(l3)*F[l1][l3];
				else if (spec==tb) f[l1][l3]=0.0; 
				else if (spec==eb) f[l1][l3]=0.0;
			}
			else {
				if (spec==tt) f[l1][l3]=0.0;
				else if (spec==te) f[l1][l3]=0.0; 
				else if (spec==ee) f[l1][l3]=0.0; 
				else if (spec==tb) f[l1][l3]=-wcl.tg(l1)*F[l3][l1];
				else if (spec==eb) f[l1][l3]=-wcl.gg(l1)*F[l3][l1]-wcl.cc(l3)*F[l1][l3];
			}
		}
	}
}

void computef_noise(std::vector< std::vector< std::vector<double> > >& f, size_t L, size_t lminCMB, size_t lmaxCMB, int num_spec){
	std::vector< std::vector<double> > F;
	
	if (num_spec==5) {
		F=std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
		compF_2_noise(F, L, 2, lmaxCMB+1);
	}
	
	std::vector< std::vector<double> > Fz(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
	compF_2_noise(Fz, L, 0, lmaxCMB+1);
	
	
	if (num_spec==1) {
		#pragma omp parallel for
		for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
			for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
				f[tt][l1][l3]=Fz[l1][l3];
			}
		}
	}
	else if (num_spec==5) {
		#pragma omp parallel for
		for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
			for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
				f[tt][l1][l3]=Fz[l1][l3];
				f[te][l1][l3]=0.;
				f[ee][l1][l3]=F[l1][l3];
				f[tb][l1][l3]=0.;
				f[eb][l1][l3]=F[l1][l3];
			}
		}
	}
	else std::cout << "I don't know what to do, yet" << std::endl;
}

void computef_noise(int spec, std::vector< std::vector<double> >& f, size_t L, size_t lminCMB, size_t lmaxCMB){
	std::vector< std::vector<double> > F,Fz;
	
	if (spec>=1) {
		F=std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
		compF_2_noise(F, L, 2, lmaxCMB+1);
	}
	if (spec<=1) {
		Fz=std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
		compF_2_noise(Fz, L, 0, lmaxCMB+1);
	}
	
	#pragma omp parallel for
	for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
		for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
			if (spec==tt) f[l1][l3]=Fz[l1][l3];
			else if (spec==te) f[l1][l3]=0.;
			else if (spec==ee) f[l1][l3]=F[l1][l3];
			else if (spec==tb) f[l1][l3]=0.;
			else if (spec==eb) f[l1][l3]=F[l1][l3];
		}
	}
}

void computef_mask(std::vector< std::vector< std::vector<double> > >& f, size_t L, PowSpec& wcl, size_t lminCMB, size_t lmaxCMB, int num_spec){
	std::vector< std::vector<double> > F;
	
	if (num_spec==5) {
		F=std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
		compF_2_mask(F, L, 2, lmaxCMB+1);
	}
	
	std::vector< std::vector<double> > Fz(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
	compF_2_mask(Fz, L, 0, lmaxCMB+1);
	
	if (num_spec==1) {
		#pragma omp parallel for
		for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
			for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
				if ((l1+L+l3)%2==0) {
					f[tt][l1][l3]=wcl.tt(l1)*Fz[l3][l1]+wcl.tt(l3)*Fz[l1][l3];
				}
				else {
					f[tt][l1][l3]=0.0;
				}
			}
		}
	}
	else if (num_spec==5) {
		#pragma omp parallel for
		for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
			for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
				if ((l1+L+l3)%2==0) {
					f[tt][l1][l3]=wcl.tt(l1)*Fz[l3][l1]+wcl.tt(l3)*Fz[l1][l3];
					f[te][l1][l3]=wcl.tg(l1)*F[l3][l1]+wcl.tg(l3)*Fz[l1][l3];
					f[ee][l1][l3]=wcl.gg(l1)*F[l3][l1]+wcl.gg(l3)*F[l1][l3];
					f[tb][l1][l3]=0.0; 
					f[eb][l1][l3]=0.0;
				}
				else {
					f[tt][l1][l3]=0.0;
					f[te][l1][l3]=0.0; 
					f[ee][l1][l3]=0.0; 
					f[tb][l1][l3]=-wcl.tg(l1)*F[l3][l1];
					f[eb][l1][l3]=-wcl.gg(l1)*F[l3][l1]-wcl.cc(l3)*F[l1][l3];
				}
			}
		}
	}
	else std::cout << "I don't know what to do, yet" << std::endl;
}


void computef_mask(int spec, std::vector< std::vector<double> > & f, size_t L, PowSpec& wcl, size_t lminCMB, size_t lmaxCMB){
	
	std::vector< std::vector<double> > F,Fz;
	
	if (spec>=1) {
		F=std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
		compF_2_mask(F, L, 2, lmaxCMB+1);
	}
	if (spec<=1) {
		Fz=std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
		compF_2_mask(Fz, L, 0, lmaxCMB+1);
	}
	
	#pragma omp parallel for
	for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
		for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
			if ((l1+L+l3)%2==0) {
				if (spec==tt) f[l1][l3]=wcl.tt(l1)*Fz[l3][l1]+wcl.tt(l3)*Fz[l1][l3];
				else if (spec==te) f[l1][l3]=wcl.tg(l1)*F[l3][l1]+wcl.tg(l3)*Fz[l1][l3];
				else if (spec==ee) f[l1][l3]=wcl.gg(l1)*F[l3][l1]+wcl.gg(l3)*F[l1][l3];
				else if (spec==tb) f[l1][l3]=0.0; 
				else if (spec==eb) f[l1][l3]=0.0;
			}
			else {
				if (spec==tt) f[l1][l3]=0.0;
				else if (spec==te) f[l1][l3]=0.0; 
				else if (spec==ee) f[l1][l3]=0.0; 
				else if (spec==tb) f[l1][l3]=-wcl.tg(l1)*F[l3][l1];
				else if (spec==eb) f[l1][l3]=-wcl.gg(l1)*F[l3][l1]-wcl.cc(l3)*F[l1][l3];
			}
		}
	}
}


std::vector< std::vector<double> > computeKernel(std::string stype, PowSpec& wcl, PowSpec& dcl, size_t lminCMB, size_t L) {
	size_t lmaxCMB=dcl.Lmax();
	
	int type = string2esttype(stype);
	
	std::vector< std::vector<double> > f(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));

	std::vector<double> invlcltt(lmaxCMB+1,0.0), invlclee(lmaxCMB+1,0.0), invlclbb(lmaxCMB+1,0.0);
	
	#pragma omp parallel for
	for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
		invlcltt[l1]=1./dcl.tt(l1);
		invlclee[l1]=1./dcl.gg(l1);
		invlclbb[l1]=1./dcl.cc(l1);
	}
	
	std::vector< std::vector<double> > out(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
					
	computef(type,f,L,wcl,2,lmaxCMB);			
	
	if (L>=2) {
		#pragma omp parallel for
		for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
			for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
				     if (type==tt) out[l1][l3]=f[l1][l3]*f[l1][l3]*invlcltt[l1]*invlcltt[l3]*.5/(2.0*L+1.0);
				else if (type==te) out[l1][l3]=f[l1][l3]*f[l1][l3]*invlcltt[l1]*invlclee[l3]/(2.0*L+1.0);
				else if (type==ee) out[l1][l3]=f[l1][l3]*f[l1][l3]*invlclee[l1]*invlclee[l3]*.5/(2.0*L+1.0);
				else if (type==tb) out[l1][l3]=f[l1][l3]*f[l1][l3]*invlcltt[l1]*invlclbb[l3]/(2.0*L+1.0); 
				else if (type==eb) out[l1][l3]=f[l1][l3]*f[l1][l3]*invlclee[l1]*invlclbb[l3]/(2.0*L+1.0);
			}
		}
	}
	
	return out;
}


void makeA(PowSpec& wcl, PowSpec& dcl, PowSpec& al, size_t lmin, size_t lmax, size_t lminCMB) {
	size_t lmaxCMB=dcl.Lmax();
	int num_spec;

	if (wcl.Num_specs()==1) {num_spec=1; assert(al.Num_specs()>=1);}
	if (wcl.Num_specs()==4) {num_spec=5; assert(al.Num_specs()>=6);}
			
	std::vector< std::vector< std::vector<double> > > f(num_spec, std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0)));

	std::vector<double> invlcltt(lmaxCMB+1,0.0), invlclee(lmaxCMB+1,0.0), invlclbb(lmaxCMB+1,0.0);
	
	#pragma omp parallel for
	for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
		invlcltt[l1]=1./dcl.tt(l1);
		if (num_spec==5) {
			invlclee[l1]=1./dcl.gg(l1);
			invlclbb[l1]=1./dcl.cc(l1);
		}
	}
	
	double att, ate, aee, atb, aeb;
	
	for (size_t L=lmin;L<lmax+1;L++) {
		
		// std::cout << " Computing amplitude ... " << (int)(L*100./lmax) << " %\r"; std::cout.flush();
		
		computef(f,L,wcl,lminCMB,lmaxCMB,num_spec);
		att=0.; ate=0.; aee=0.; atb=0.; aeb=0.;
		
		if (L>=lmin) {
			#pragma omp parallel for reduction(+:att, ate, aee, atb, aeb) schedule(dynamic, 25)
			for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
				for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
					att+=f[tt][l1][l3]*f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*.5;
					if (num_spec==5) {
						ate+=f[te][l1][l3]*f[te][l1][l3]*invlcltt[l1]*invlclee[l3];
						aee+=f[ee][l1][l3]*f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*.5;
						atb+=f[tb][l1][l3]*f[tb][l1][l3]*invlcltt[l1]*invlclbb[l3]; 
						aeb+=f[eb][l1][l3]*f[eb][l1][l3]*invlclee[l1]*invlclbb[l3];
					}
				}
			}
		}
		
		al.tt(L) = (att!=0.) ? (2.0*L+1.0)/att : 0.0;
		if (num_spec==5) {
			al.tg(L) = (ate!=0.) ? (2.0*L+1.0)/ate : 0.0;
			al.gg(L) = (aee!=0.) ? (2.0*L+1.0)/aee : 0.0;
			al.tc(L) = (atb!=0.) ? (2.0*L+1.0)/atb : 0.0;
			al.gc(L) = (aeb!=0.) ? (2.0*L+1.0)/aeb : 0.0;
		}
		
		if(PyErr_CheckSignals() == -1) {
			throw invalid_argument( "Keyboard interrupt" );
		}
	}
}

std::vector< std::vector<double> > makeAN(PowSpec& wcl, PowSpec& dcl, PowSpec& ncl, PowSpec& rdcls, PowSpec& al, size_t lmin, size_t lmax, size_t lminCMB1, size_t lminCMB2, size_t lmaxCMB1, size_t lmaxCMB2) {
	int num_spec=5;

	assert(wcl.Num_specs()==4);
	
	size_t lmaxCMB=max(lmaxCMB1,lmaxCMB2);
	size_t lminCMB=min(lminCMB1,lminCMB2);

	std::vector< std::vector<double> > bias(9, std::vector<double>(lmax+1,0.0));
	std::vector< std::vector< std::vector<double> > > f(num_spec, std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0)));

	
	std::vector<double> invlcltt(lmaxCMB+1,0.0), invlclee(lmaxCMB+1,0.0), invlclbb(lmaxCMB+1,0.0);
	
	#pragma omp parallel for
	for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
		invlcltt[l1]=1./ncl.tt(l1);
		invlclee[l1]=1./ncl.gg(l1);
		invlclbb[l1]=1./ncl.cc(l1);
	}

	double ntttt, nttte, nttee, ntete, nteee, neeee, ntbtb, ntbeb, nebeb, att, ate, aee, atb, aeb;
	
	for (size_t L=lmin;L<lmax+1;L++) {
		
		computef(f,L,wcl,lminCMB,lmaxCMB,num_spec);
		ntttt=0.; nttte=0.; nttee=0.; ntete=0.; nteee=0.; neeee=0.; ntbtb=0.; ntbeb=0.; nebeb=0.;
		att=0.; ate=0.; aee=0.; atb=0.; aeb=0.;
		if (L>=lmin) {
			#pragma omp parallel for reduction(+:att, ate, aee, atb, aeb, ntttt, nttte, nttee, ntete, nteee, neeee, ntbtb, ntbeb, nebeb) schedule(dynamic, 25)
			for (size_t l1=lminCMB1;l1<lmaxCMB1+1;l1++) {
				for (size_t l3=lminCMB2;l3<lmaxCMB2+1;l3++) {
					att+=f[tt][l1][l3]*f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*.5;
					ate+=f[te][l1][l3]*f[te][l1][l3]*invlcltt[l1]*invlclee[l3];
					aee+=f[ee][l1][l3]*f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*.5;
					atb+=f[tb][l1][l3]*f[tb][l1][l3]*invlcltt[l1]*invlclbb[l3]; 
					aeb+=f[eb][l1][l3]*f[eb][l1][l3]*invlclee[l1]*invlclbb[l3];

					ntttt+=f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*(f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*rdcls.tt(l1)*rdcls.tt(l3)+sgn(L+l1+l3)*f[tt][l3][l1]*invlcltt[l3]*invlcltt[l1]*rdcls.tt(l1)*rdcls.tt(l3))*.25;
					nttte+=f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*(f[te][l1][l3]*invlcltt[l1]*invlclee[l3]*rdcls.tt(l1)*rdcls.tg(l3)+sgn(L+l1+l3)*f[te][l3][l1]*invlcltt[l3]*invlclee[l1]*rdcls.tg(l1)*rdcls.tt(l3))*.5;
					nttee+=f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*(f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*rdcls.tg(l1)*rdcls.tg(l3)+sgn(L+l1+l3)*f[ee][l3][l1]*invlclee[l3]*invlclee[l1]*rdcls.tg(l1)*rdcls.tg(l3))*.25;
					ntete+=f[te][l1][l3]*invlcltt[l1]*invlclee[l3]*(f[te][l1][l3]*invlcltt[l1]*invlclee[l3]*rdcls.tt(l1)*rdcls.gg(l3)+sgn(L+l1+l3)*f[te][l3][l1]*invlcltt[l3]*invlclee[l1]*rdcls.tg(l1)*rdcls.tg(l3));
					nteee+=f[te][l1][l3]*invlcltt[l1]*invlclee[l3]*(f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*rdcls.tg(l1)*rdcls.gg(l3)+sgn(L+l1+l3)*f[ee][l3][l1]*invlclee[l3]*invlclee[l1]*rdcls.tg(l1)*rdcls.gg(l3))*.5;
					neeee+=f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*(f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*rdcls.gg(l1)*rdcls.gg(l3)+sgn(L+l1+l3)*f[ee][l3][l1]*invlclee[l3]*invlclee[l1]*rdcls.gg(l1)*rdcls.gg(l3))*.25;
					ntbtb+=f[tb][l1][l3]*invlcltt[l1]*invlclbb[l3]*(f[tb][l1][l3]*invlcltt[l1]*invlclbb[l3]*rdcls.tt(l1)*rdcls.cc(l3));
					ntbeb+=f[tb][l1][l3]*invlcltt[l1]*invlclbb[l3]*(f[eb][l1][l3]*invlclee[l1]*invlclbb[l3]*rdcls.tg(l1)*rdcls.cc(l3));
					nebeb+=f[eb][l1][l3]*invlclee[l1]*invlclbb[l3]*(f[eb][l1][l3]*invlclee[l1]*invlclbb[l3]*rdcls.gg(l1)*rdcls.cc(l3));
				}
			}
		}
		
		al.tt(L) = (att!=0.) ? (2.0*L+1.0)/att : 0.0;
		al.tg(L) = (ate!=0.) ? (2.0*L+1.0)/ate : 0.0;
		al.gg(L) = (aee!=0.) ? (2.0*L+1.0)/aee : 0.0;
		al.tc(L) = (atb!=0.) ? (2.0*L+1.0)/atb : 0.0;
		al.gc(L) = (aeb!=0.) ? (2.0*L+1.0)/aeb : 0.0;

		bias[tttt][L]=ntttt*al.tt(L)*al.tt(L)/(2.*L+1.);
		bias[ttte][L]=nttte*al.tt(L)*al.tg(L)/(2.*L+1.);
		bias[ttee][L]=nttee*al.tt(L)*al.gg(L)/(2.*L+1.);
		//=ntttb*att*atb/(2.*L+1.);
		//=ntteb*att*aeb/(2.*L+1.);
		bias[tete][L]=ntete*al.tg(L)*al.tg(L)/(2.*L+1.);
		bias[teee][L]=nteee*al.tg(L)*al.gg(L)/(2.*L+1.);
		//=ntetb*ate*atb/(2.*L+1.);
		//=nteeb*ate*aeb/(2.*L+1.);
		bias[eeee][L]=neeee*al.gg(L)*al.gg(L)/(2.*L+1.);
		//=neetb*aee*atb/(2.*L+1.);
		//=neeeb*aee*aeb/(2.*L+1.);
		bias[tbtb][L]=ntbtb*al.tc(L)*al.tc(L)/(2.*L+1.);
		bias[tbeb][L]=ntbeb*al.tc(L)*al.gc(L)/(2.*L+1.);
		bias[ebeb][L]=nebeb*al.gc(L)*al.gc(L)/(2.*L+1.);

		PyErr_CheckSignals();
	}
	
	return bias;
}

std::vector< std::vector<double> > makeAN_RD(PowSpec& wcl, PowSpec& dcl, PowSpec& ncl, PowSpec& rdcls, PowSpec& al, size_t lmin, size_t lmax, size_t lminCMB1, size_t lminCMB2, size_t lmaxCMB1, size_t lmaxCMB2) {
	// Analytical realization-dependent N0 (eq. 19 of "Full covariance of CMB and lensing reconstruction power spectra"
	// from Julien Peloton et al). Calculates the 2\hat{N}^{(0)} term implemented by Miguel Ruiz-Granda.
	int num_spec=5;

	assert(wcl.Num_specs()==4);
	
	size_t lmaxCMB=max(lmaxCMB1,lmaxCMB2);
	size_t lminCMB=min(lminCMB1,lminCMB2);

	std::vector< std::vector<double> > bias(9, std::vector<double>(lmax+1,0.0));
	std::vector< std::vector< std::vector<double> > > f(num_spec, std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0)));

	
	std::vector<double> invlcltt(lmaxCMB+1,0.0), invlclee(lmaxCMB+1,0.0), invlclbb(lmaxCMB+1,0.0);
	
	#pragma omp parallel for
	for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
		invlcltt[l1]=1./ncl.tt(l1);
		invlclee[l1]=1./ncl.gg(l1);
		invlclbb[l1]=1./ncl.cc(l1);
	}

	double ntttt, nttte, nttee, ntete, nteee, neeee, ntbtb, ntbeb, nebeb, att, ate, aee, atb, aeb;
	
	for (size_t L=lmin;L<lmax+1;L++) {
		
		computef(f,L,wcl,lminCMB,lmaxCMB,num_spec);
		ntttt=0.; nttte=0.; nttee=0.; ntete=0.; nteee=0.; neeee=0.; ntbtb=0.; ntbeb=0.; nebeb=0.;
		att=0.; ate=0.; aee=0.; atb=0.; aeb=0.;
		if (L>=lmin) {
			#pragma omp parallel for reduction(+:att, ate, aee, atb, aeb, ntttt, nttte, nttee, ntete, nteee, neeee, ntbtb, ntbeb, nebeb) schedule(dynamic, 25)
			for (size_t l1=lminCMB1;l1<lmaxCMB1+1;l1++) {
				for (size_t l3=lminCMB2;l3<lmaxCMB2+1;l3++) {
					att+=f[tt][l1][l3]*f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*.5;
					ate+=f[te][l1][l3]*f[te][l1][l3]*invlcltt[l1]*invlclee[l3];
					aee+=f[ee][l1][l3]*f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*.5;
					atb+=f[tb][l1][l3]*f[tb][l1][l3]*invlcltt[l1]*invlclbb[l3]; 
					aeb+=f[eb][l1][l3]*f[eb][l1][l3]*invlclee[l1]*invlclbb[l3];

					ntttt+=f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*(f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*(rdcls.tt(l1)*dcl.tt(l3)+dcl.tt(l1)*rdcls.tt(l3))+sgn(L+l1+l3)*f[tt][l3][l1]*invlcltt[l3]*invlcltt[l1]*(rdcls.tt(l1)*dcl.tt(l3)+dcl.tt(l1)*rdcls.tt(l3)))*.25;
					nttte+=f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*(f[te][l1][l3]*invlcltt[l1]*invlclee[l3]*(rdcls.tt(l1)*dcl.tg(l3)+dcl.tt(l1)*rdcls.tg(l3))+sgn(L+l1+l3)*f[te][l3][l1]*invlcltt[l3]*invlclee[l1]*(rdcls.tg(l1)*dcl.tt(l3)+dcl.tg(l1)*rdcls.tt(l3)))*.5;
					nttee+=f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3]*(f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*(rdcls.tg(l1)*dcl.tg(l3)+dcl.tg(l1)*rdcls.tg(l3))+sgn(L+l1+l3)*f[ee][l3][l1]*invlclee[l3]*invlclee[l1]*(rdcls.tg(l1)*dcl.tg(l3)+dcl.tg(l1)*rdcls.tg(l3)))*.25;
					ntete+=f[te][l1][l3]*invlcltt[l1]*invlclee[l3]*(f[te][l1][l3]*invlcltt[l1]*invlclee[l3]*(rdcls.tt(l1)*dcl.gg(l3)+dcl.tt(l1)*rdcls.gg(l3))+sgn(L+l1+l3)*f[te][l3][l1]*invlcltt[l3]*invlclee[l1]*(rdcls.tg(l1)*dcl.tg(l3)+dcl.tg(l1)*rdcls.tg(l3)));
					nteee+=f[te][l1][l3]*invlcltt[l1]*invlclee[l3]*(f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*(rdcls.tg(l1)*dcl.gg(l3)+dcl.tg(l1)*rdcls.gg(l3))+sgn(L+l1+l3)*f[ee][l3][l1]*invlclee[l3]*invlclee[l1]*(rdcls.tg(l1)*dcl.gg(l3)+dcl.tg(l1)*rdcls.gg(l3)))*.5;
					neeee+=f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*(f[ee][l1][l3]*invlclee[l1]*invlclee[l3]*(rdcls.gg(l1)*dcl.gg(l3)+dcl.gg(l1)*rdcls.gg(l3))+sgn(L+l1+l3)*f[ee][l3][l1]*invlclee[l3]*invlclee[l1]*(rdcls.gg(l1)*dcl.gg(l3)+dcl.gg(l1)*rdcls.gg(l3)))*.25;
					ntbtb+=f[tb][l1][l3]*invlcltt[l1]*invlclbb[l3]*(f[tb][l1][l3]*invlcltt[l1]*invlclbb[l3]*(rdcls.tt(l1)*dcl.cc(l3)+dcl.tt(l1)*rdcls.cc(l3)));
					ntbeb+=f[tb][l1][l3]*invlcltt[l1]*invlclbb[l3]*(f[eb][l1][l3]*invlclee[l1]*invlclbb[l3]*(rdcls.tg(l1)*dcl.cc(l3)+dcl.tg(l1)*rdcls.cc(l3)));
					nebeb+=f[eb][l1][l3]*invlclee[l1]*invlclbb[l3]*(f[eb][l1][l3]*invlclee[l1]*invlclbb[l3]*(rdcls.gg(l1)*dcl.cc(l3)+dcl.gg(l1)*rdcls.cc(l3) + (rdcls.gg(l1)-dcl.gg(l1))*(rdcls.cc(l3)-dcl.cc(l3))));
				}
			}
		}
		
		al.tt(L) = (att!=0.) ? (2.0*L+1.0)/att : 0.0;
		al.tg(L) = (ate!=0.) ? (2.0*L+1.0)/ate : 0.0;
		al.gg(L) = (aee!=0.) ? (2.0*L+1.0)/aee : 0.0;
		al.tc(L) = (atb!=0.) ? (2.0*L+1.0)/atb : 0.0;
		al.gc(L) = (aeb!=0.) ? (2.0*L+1.0)/aeb : 0.0;

		bias[tttt][L]=ntttt*al.tt(L)*al.tt(L)/(2.*L+1.);
		bias[ttte][L]=nttte*al.tt(L)*al.tg(L)/(2.*L+1.);
		bias[ttee][L]=nttee*al.tt(L)*al.gg(L)/(2.*L+1.);
		//=ntttb*att*atb/(2.*L+1.);
		//=ntteb*att*aeb/(2.*L+1.);
		bias[tete][L]=ntete*al.tg(L)*al.tg(L)/(2.*L+1.);
		bias[teee][L]=nteee*al.tg(L)*al.gg(L)/(2.*L+1.);
		//=ntetb*ate*atb/(2.*L+1.);
		//=nteeb*ate*aeb/(2.*L+1.);
		bias[eeee][L]=neeee*al.gg(L)*al.gg(L)/(2.*L+1.);
		//=neetb*aee*atb/(2.*L+1.);
		//=neeeb*aee*aeb/(2.*L+1.);
		bias[tbtb][L]=ntbtb*al.tc(L)*al.tc(L)/(2.*L+1.);
		bias[tbeb][L]=ntbeb*al.tc(L)*al.gc(L)/(2.*L+1.);
		bias[ebeb][L]=nebeb*al.gc(L)*al.gc(L)/(2.*L+1.);

		PyErr_CheckSignals();
	}
	
	return bias;
}

std::vector<std::vector< std::vector<double> > > makeAN_RD_iterSims(PowSpec& wcl, PowSpec& dcl, PowSpec& ncl, std::vector<PowSpec*> & rdcls, PowSpec& al, size_t lmin, size_t lmax, size_t lminCMB1, size_t lminCMB2, size_t lmaxCMB1, size_t lmaxCMB2, size_t Nsims) {
	// Analytical realization-dependent N0 (eq. 19 of "Full covariance of CMB and lensing reconstruction power spectra"
	// from Julien Peloton et al). Calculates the 2\hat{N}^{(0)} term implemented by Miguel Ruiz-Granda. Optimized version.
	int num_spec=5;

	assert(wcl.Num_specs()==4);
	
	size_t lmaxCMB=max(lmaxCMB1,lmaxCMB2);
	size_t lminCMB=min(lminCMB1,lminCMB2);

	std::vector<std::vector<std::vector<double>>> bias(9, std::vector<std::vector<double>>(Nsims, std::vector<double>(lmax + 1, 0.0)));
	std::vector< std::vector< std::vector<double> > > f(num_spec, std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0)));

	
	std::vector<double> invlcltt(lmaxCMB+1,0.0), invlclee(lmaxCMB+1,0.0), invlclbb(lmaxCMB+1,0.0);
	
	#pragma omp parallel for
	for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
		invlcltt[l1]=1./ncl.tt(l1);
		invlclee[l1]=1./ncl.gg(l1);
		invlclbb[l1]=1./ncl.cc(l1);
	}

	double att, ate, aee, atb, aeb;

	for (size_t L=lmin;L<lmax+1;L++) {

		// Print L without newline to check the progress.
		if ( L%10 == 0)
    	   std::cout << "L = " << L << "\r" << std::flush;

		att=0.; ate=0.; aee=0.; atb=0.; aeb=0.;
		double LC=2.*L+1.;
		double ILC=1.0/LC;

		computef(f,L,wcl,lminCMB,lmaxCMB,num_spec);

		std::vector<double> ntttt(Nsims,0.0), nttte(Nsims,0.0), nttee(Nsims,0.0), ntete(Nsims,0.0),
			nteee(Nsims,0.0), neeee(Nsims,0.0), ntbtb(Nsims,0.0), ntbeb(Nsims,0.0), nebeb(Nsims,0.0);

		#pragma omp parallel 
		{
			std::vector<double> ntttt_(Nsims,0.0), nttte_(Nsims,0.0), nttee_(Nsims,0.0), ntete_(Nsims,0.0),
			nteee_(Nsims,0.0), neeee_(Nsims,0.0), ntbtb_(Nsims,0.0), ntbeb_(Nsims,0.0), nebeb_(Nsims,0.0);
			double att_, ate_, aee_, atb_, aeb_;
			att_=0.; ate_=0.; aee_=0.; atb_=0.; aeb_=0.;

			#pragma omp for
			for (size_t l1=lminCMB1;l1<lmaxCMB1+1;l1++) {

				double dcl_tt_l1=dcl.tt(l1);
				double dcl_tg_l1=dcl.tg(l1);
				double dcl_gg_l1=dcl.gg(l1);

				for (size_t l3=lminCMB2;l3<lmaxCMB2+1;l3++) {

					// Getting data from memory
					double dcl_tt_l3=dcl.tt(l3);
					double dcl_tg_l3=dcl.tg(l3);
					double dcl_gg_l3=dcl.gg(l3);
					double dcl_cc_l3=dcl.cc(l3);
					
					// Precalculating multiplications
					double g_tt_l1_l3=f[tt][l1][l3]*invlcltt[l1]*invlcltt[l3];
					double g_te_l1_l3=f[te][l1][l3]*invlcltt[l1]*invlclee[l3];
					double g_ee_l1_l3=f[ee][l1][l3]*invlclee[l1]*invlclee[l3];
					double g_tb_l1_l3=f[tb][l1][l3]*invlcltt[l1]*invlclbb[l3];
					double g_eb_l1_l3=f[eb][l1][l3]*invlclee[l1]*invlclbb[l3];

					int SIGN=(int)sgn(L+l1+l3); // return +1, 0 -1
					double SIG_g_tt_l3_l1=SIGN*f[tt][l3][l1]*invlcltt[l1]*invlcltt[l3];
					double SIG_g_te_l3_l1=SIGN*f[te][l3][l1]*invlcltt[l3]*invlclee[l1];
					double SIG_g_ee_l3_l1=SIGN*f[ee][l3][l1]*invlclee[l3]*invlclee[l1];

					// Normalization factors
					att_ += f[tt][l1][l3]*g_tt_l1_l3;
					ate_ += f[te][l1][l3]*g_te_l1_l3;
					aee_ += f[ee][l1][l3]*g_ee_l1_l3;
					atb_ += f[tb][l1][l3]*g_tb_l1_l3;
					aeb_ += f[eb][l1][l3]*g_eb_l1_l3;
				
					for (size_t i = 0; i < Nsims; i++) {
						// Getting data from memory and store in local variable
						double rdcls_tt_l1=rdcls[i]->tt(l1);
						double rdcls_tt_l3=rdcls[i]->tt(l3);
						double rdcls_tg_l1=rdcls[i]->tg(l1);
						double rdcls_tg_l3=rdcls[i]->tg(l3);
						double rdcls_gg_l1=rdcls[i]->gg(l1);
						double rdcls_gg_l3=rdcls[i]->gg(l3);
						double rdcls_cc_l1=rdcls[i]->cc(l1); 
						double rdcls_cc_l3=rdcls[i]->cc(l3);  

						ntttt_[i] += g_tt_l1_l3*(g_tt_l1_l3 + SIG_g_tt_l3_l1)*(rdcls_tt_l1*dcl_tt_l3+dcl_tt_l1*rdcls_tt_l3);
						nttte_[i] += g_tt_l1_l3*(g_te_l1_l3*(rdcls_tt_l1*dcl_tg_l3 + dcl_tt_l1*rdcls_tg_l3)+SIG_g_te_l3_l1*(rdcls_tg_l1*dcl_tt_l3 + dcl_tg_l1*rdcls_tt_l3));
						nttee_[i] += g_tt_l1_l3*(g_ee_l1_l3*(dcl_tg_l3*rdcls_tg_l1+dcl_tg_l1*rdcls_tg_l3)+SIG_g_ee_l3_l1*(rdcls_tg_l1*dcl_tg_l3 + dcl_tg_l1*rdcls_tg_l3));
						ntete_[i] += g_te_l1_l3*(g_te_l1_l3*(dcl_gg_l3*rdcls_tt_l1+dcl_tt_l1*rdcls_gg_l3)+SIG_g_te_l3_l1*(rdcls_tg_l1*dcl_tg_l3 + dcl_tg_l1*rdcls_tg_l3));
						nteee_[i] += g_te_l1_l3*(g_ee_l1_l3 + SIG_g_ee_l3_l1)*(dcl_gg_l3*rdcls_tg_l1 + dcl_tg_l1*rdcls_gg_l3);
						neeee_[i] += g_ee_l1_l3*(g_ee_l1_l3 + SIG_g_ee_l3_l1)*(dcl_gg_l3*rdcls_gg_l1 + dcl_gg_l1*rdcls_gg_l3);
						ntbtb_[i] += g_tb_l1_l3*(g_tb_l1_l3*(rdcls_tt_l1*dcl_cc_l3 + dcl_tt_l1*rdcls_cc_l3));
						ntbeb_[i] += g_tb_l1_l3*(g_eb_l1_l3*(rdcls_tg_l1*dcl_cc_l3 + dcl_tg_l1*rdcls_cc_l3));
						nebeb_[i] += g_eb_l1_l3*(g_eb_l1_l3*(rdcls_gg_l1*dcl_cc_l3 + dcl_gg_l1*rdcls_cc_l3));// + (rdcls_gg_l1-dcl_gg_l1)*(rdcls_cc_l3-dcl_cc_l3)));
					}   
				}
			}

			// Reduction the variables
			#pragma omp critical 
			{
				att += att_;
				ate += ate_;
				aee += aee_;
				atb += atb_;
				aeb += aeb_;
				
				for (size_t i = 0; i < Nsims; i++) {
					ntttt[i] += ntttt_[i];
					nttte[i] += nttte_[i];
					nttee[i] += nttee_[i];
					ntete[i] += ntete_[i];
					nteee[i] += nteee_[i];
					neeee[i] += neeee_[i];
					ntbtb[i] += ntbtb_[i];
					ntbeb[i] += ntbeb_[i];
					nebeb[i] += nebeb_[i];
				}
			}
		}
		
		al.tt(L) = (att!=0.) ? LC/(att*0.5) : 0.0;
		al.tg(L) = (ate!=0.) ? LC/ate : 0.0;
		al.gg(L) = (aee!=0.) ? LC/(aee*0.5) : 0.0;
		al.tc(L) = (atb!=0.) ? LC/atb : 0.0;
		al.gc(L) = (aeb!=0.) ? LC/aeb : 0.0;

		// Storing data in a local variable
		double al_tt_L=al.tt(L); 
		double al_tg_L=al.tg(L);
		double al_gg_L=al.gg(L);
		double al_gc_L=al.gc(L);
		double al_tc_L=al.tc(L);

		// Iterating over simulations
		for (size_t i = 0; i < Nsims; i++) {
			bias[tttt][i][L]=0.25*ntttt[i]*al_tt_L*al_tt_L*ILC;
			bias[ttte][i][L]=0.5*nttte[i]*al_tt_L*al_tg_L*ILC;
			bias[ttee][i][L]=0.25*nttee[i]*al_tt_L*al_gg_L*ILC;
			bias[tete][i][L]=ntete[i]*al_tg_L*al_tg_L*ILC;
			bias[teee][i][L]=0.5*nteee[i]*al_tg_L*al_gg_L*ILC;
			bias[eeee][i][L]=0.25*neeee[i]*al_gg_L*al_gg_L*ILC;
			bias[tbtb][i][L]=ntbtb[i]*al_tc_L*al_tc_L*ILC;
			bias[tbeb][i][L]=ntbeb[i]*al_tc_L*al_gc_L*ILC;
			bias[ebeb][i][L]=nebeb[i]*al_gc_L*al_gc_L*ILC;
		}

		PyErr_CheckSignals();
	}
	
	return bias;
}


/*
 // VERSION ORIGINAL
std::vector<std::vector< std::vector<double> > > makeAN_RD_iterSims(PowSpec& wcl, PowSpec& ncl, PowSpec& dcl, std::vector<PowSpec*> & rdcls, PowSpec& al, size_t lmin, size_t lmax, size_t lminCMB1, size_t lminCMB2, size_t lmaxCMB1, size_t lmaxCMB2, size_t Nsims) {
	// Analytical realization-dependent N0 (eq. 19 of "Full covariance of CMB and lensing reconstruction power spectra"
	// from Julien Peloton et al). Calculates the 2\hat{N}^{(0)} term implemented by Miguel Ruiz-Granda. Optimized version.
	int num_spec=5;

	assert(wcl.Num_specs()==4);
	
	size_t lmaxCMB=max(lmaxCMB1,lmaxCMB2);
	size_t lminCMB=min(lminCMB1,lminCMB2);

	std::vector<std::vector<std::vector<double>>> bias(9, std::vector<std::vector<double>>(Nsims, std::vector<double>(lmax + 1, 0.0)));
	std::vector< std::vector< std::vector<double> > > f(num_spec, std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0)));

	
	std::vector<double> invlcltt(lmaxCMB+1,0.0), invlclee(lmaxCMB+1,0.0), invlclbb(lmaxCMB+1,0.0);
	
	#pragma omp parallel for
	for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
		invlcltt[l1]=1./ncl.tt(l1);
		invlclee[l1]=1./ncl.gg(l1);
		invlclbb[l1]=1./ncl.cc(l1);
	}

	double att, ate, aee, atb, aeb;

	for (size_t L=lmin;L<lmax+1;L++) {

		// Print L without newline to check the progress.
		if( L%10 == 0)
    	    std::cout << "LM = " << L << "\r" << std::flush;

		att=0.; ate=0.; aee=0.; atb=0.; aeb=0.;
		double LC=double((L<<1)+1);
		double ILC=1.0/LC;

		computef(f,L,wcl,lminCMB,lmaxCMB,num_spec);

		std::vector<double> ntttt(Nsims,0.0), nttte(Nsims,0.0), nttee(Nsims,0.0), ntete(Nsims,0.0),
			nteee(Nsims,0.0), neeee(Nsims,0.0), ntbtb(Nsims,0.0), ntbeb(Nsims,0.0), nebeb(Nsims,0.0);

		#pragma omp parallel 
		{
			std::vector<double> ntttt_(Nsims,0.0), nttte_(Nsims,0.0), nttee_(Nsims,0.0), ntete_(Nsims,0.0),
			nteee_(Nsims,0.0), neeee_(Nsims,0.0), ntbtb_(Nsims,0.0), ntbeb_(Nsims,0.0), nebeb_(Nsims,0.0);
			double att_, ate_, aee_, atb_, aeb_;
			att_=0.; ate_=0.; aee_=0.; atb_=0.; aeb_=0.;

			// #pragma omp parallel for reduction(+:att, ate, aee, atb, aeb) shared(ntttt, nttte, nttee, ntete, nteee, neeee, ntbtb, ntbeb, nebeb, i) schedule(dynamic, 25)
			// #pragma omp parallel for reduction(+:att, ate, aee, atb, aeb, ntttt[0:Nsims], nttte[0:Nsims], nttee[0:Nsims], ntete[0:Nsims], nteee[0:Nsims], neeee[:Nsims], ntbtb[:Nsims], ntbeb[:Nsims], nebeb[:Nsims]) schedule(dynamic, 25)
			#pragma omp for
			for (size_t l1=lminCMB1;l1<lmaxCMB1+1;l1++) {
				double invlcltt_l1=invlcltt[l1];
				double invlclee_l1=invlclee[l1];

				double dcl_tt_l1=dcl.tt(l1);
				double dcl_tg_l1=dcl.tg(l1);
				double dcl_gg_l1=dcl.gg(l1);

				for (size_t l3=lminCMB2;l3<lmaxCMB2+1;l3++) {

					double dcl_tt_l3=dcl.tt(l3);
					double dcl_tg_l3=dcl.tg(l3);
					double dcl_gg_l3=dcl.gg(l3);
					double dcl_cc_l3=dcl.cc(l3);

					double SIG=sgn(L+l1+l3);

					double f_tt_l1_l3=f[tt][l1][l3];
					double f_tt_l3_l1=f[tt][l3][l1];
					double f_te_l1_l3=f[te][l1][l3]; 
					double f_te_l3_l1=f[te][l3][l1]; 
					double f_ee_l1_l3=f[ee][l1][l3];
					double f_ee_l3_l1=f[ee][l3][l1];
					double f_tb_l1_l3=f[tb][l1][l3];
					double f_tb_l3_l1=f[tb][l3][l1];
					double f_eb_l1_l3=f[eb][l1][l3];
					double f_eb_l3_l1=f[eb][l3][l1];

					double invlcltt_l3=invlcltt[l3];
					double invlclee_l3=invlclee[l3];
					double invlclbb_l3=invlclbb[l3];
					
					double f_tt_l1_l3_invlcltt_l1=f_tt_l1_l3*invlcltt_l1;
					double ATT=f_tt_l1_l3*f_tt_l1_l3_invlcltt_l1;
					double f_te_l1_l3_invlcltt_l1=f_te_l1_l3*invlcltt_l1;
					double ATE=f_te_l1_l3*f_te_l1_l3_invlcltt_l1;
					double f_ee_l1_l3_invlclee_l1=f_ee_l1_l3*invlclee_l1;
					double AEE=f_ee_l1_l3*f_ee_l1_l3_invlclee_l1;
					double f_tb_l1_l3_invlcltt_l1=f_tb_l1_l3*invlcltt_l1;
					double ATB=f_tb_l1_l3*f_tb_l1_l3_invlcltt_l1;
					double f_eb_l1_l3_invlclee_l1=f_eb_l1_l3*invlclee_l1;
					double AEB=f_eb_l1_l3*f_eb_l1_l3_invlclee_l1;

					att_+=ATT*invlcltt_l3; // 0.5
					ate_+=ATE*invlclee_l3;
					aee_+=AEE*invlclee_l3; // 0.5;
					atb_+=ATB*invlclbb_l3;
					aeb_+=AEB*invlclbb_l3;

					double f_tb_l1_l3_invlcltt_l1_invlclbb_l3=f_tb_l1_l3_invlcltt_l1*invlclbb_l3;
					double f_tt_l1_l3_invlcltt_l1_invlcltt_l3=f_tt_l1_l3_invlcltt_l1*invlcltt_l3;
					double f_te_l1_l3_invlcltt_l1_invlclee_l3=f_te_l1_l3_invlcltt_l1*invlclee_l3;
					double invlclee_l3_invlclee_l1=invlclee_l3*invlclee_l1;
					double invlcltt_l3_invlclee_l1=invlcltt_l3*invlclee_l1;
					double f_eb_l1_l3_invlclee_l1_invlclbb_l3=f_eb_l1_l3_invlclee_l1*invlclbb_l3; 

					for (size_t i = 0; i < Nsims; i++) {
						double rdcls_tt_l1=rdcls[i]->tt(l1);
						double rdcls_tt_l3=rdcls[i]->tt(l3);
						double rdcls_tg_l1=rdcls[i]->tg(l1);
						double rdcls_tg_l3=rdcls[i]->tg(l3);
						double rdcls_gg_l1=rdcls[i]->gg(l1);
						double rdcls_gg_l3=rdcls[i]->gg(l3);
						double rdcls_cc_l1=rdcls[i]->cc(l1); 
						double rdcls_cc_l3=rdcls[i]->cc(l3);  

						ntttt_[i]+=f_tt_l1_l3_invlcltt_l1_invlcltt_l3*(f_tt_l1_l3_invlcltt_l1_invlcltt_l3+SIG*f_tt_l3_l1*invlcltt_l3*invlcltt_l1)*(rdcls_tt_l1*dcl_tt_l3+dcl_tt_l1*rdcls_tt_l3); // *.25;
						nttte_[i]+=f_tt_l1_l3_invlcltt_l1_invlcltt_l3*(f_te_l1_l3*invlcltt_l1*invlclee_l3*(rdcls_tt_l1*dcl_tg_l3+dcl_tt_l1*rdcls_tg_l3)+SIG*f_te_l3_l1*invlcltt_l3_invlclee_l1*(rdcls_tg_l1*dcl_tt_l3+dcl_tg_l1*rdcls_tt_l3)); // *.5;
						nttee_[i]+=f_tt_l1_l3_invlcltt_l1_invlcltt_l3*(f_ee_l1_l3_invlclee_l1*invlclee_l3*(rdcls_tg_l1*dcl_tg_l3+dcl_tg_l1*rdcls_tg_l3)+SIG*f_ee_l3_l1*invlclee_l3_invlclee_l1*(rdcls_tg_l1*dcl_tg_l3+dcl_tg_l1*rdcls_tg_l3)); // *.25;
						ntete_[i]+=f_te_l1_l3_invlcltt_l1_invlclee_l3*(f_te_l1_l3*invlcltt_l1*invlclee_l3*(rdcls_tt_l1*dcl_gg_l3+dcl_tt_l1*rdcls_gg_l3)+SIG*f_te_l3_l1*invlcltt_l3_invlclee_l1*(rdcls_tg_l1*dcl_tg_l3+dcl_tg_l1*rdcls_tg_l3));
						nteee_[i]+=f_te_l1_l3_invlcltt_l1_invlclee_l3*(f_ee_l1_l3_invlclee_l1*invlclee_l3+SIG*f_ee_l3_l1*invlclee_l3_invlclee_l1)*(rdcls_tg_l1*dcl_gg_l3+dcl_tg_l1*rdcls_gg_l3); //*.5;
						neeee_[i]+=f_ee_l1_l3*invlclee_l1*invlclee_l3*(f_ee_l1_l3_invlclee_l1*invlclee_l3*(rdcls_gg_l1*dcl_gg_l3+dcl_gg_l1*rdcls_gg_l3)+SIG*f_ee_l3_l1*invlclee_l3_invlclee_l1*(rdcls_gg_l1*dcl_gg_l3+dcl_gg_l1*rdcls_gg_l3)); //*.25;
						ntbtb_[i]+=f_tb_l1_l3_invlcltt_l1_invlclbb_l3*(f_tb_l1_l3*invlcltt_l1*invlclbb_l3*(rdcls_tt_l1*dcl_cc_l3+dcl_tt_l1*rdcls_cc_l3));
						ntbeb_[i]+=f_tb_l1_l3_invlcltt_l1_invlclbb_l3*(f_eb_l1_l3_invlclee_l1_invlclbb_l3*(rdcls_tg_l1*dcl_cc_l3+dcl_tg_l1*rdcls_cc_l3));
						nebeb_[i]+=f_eb_l1_l3*invlclee_l1*invlclbb_l3*(f_eb_l1_l3_invlclee_l1_invlclbb_l3*(rdcls_gg_l1*dcl_cc_l3+dcl_gg_l1*rdcls_cc_l3 + (rdcls_gg_l1-dcl_gg_l1)*(rdcls_cc_l3-dcl_cc_l3)));
					}   
				}
			}

			#pragma omp critical 
			{
				att+=att_; // 0.5
				ate+=ate_;
				aee+=aee_; // 0.5;
				atb+=atb_;
				aeb+=aeb_;
				for (size_t i = 0; i < Nsims; i++) {
					ntttt[i]+=ntttt_[i];
					nttte[i]+=nttte_[i];
					nttee[i]+=nttee_[i];
					ntete[i]+=ntete_[i];
					nteee[i]+=nteee_[i];
					neeee[i]+=neeee_[i];
					ntbtb[i]+=ntbtb_[i];
					ntbeb[i]+=ntbeb_[i];
					nebeb[i]+=nebeb_[i];
				}
			}
		}
		
		al.tt(L) = (att!=0.) ? 2.0*LC/att : 0.0;
		al.tg(L) = (ate!=0.) ? LC/ate : 0.0;
		al.gg(L) = (aee!=0.) ? 2.0*LC/aee : 0.0;
		al.tc(L) = (atb!=0.) ? LC/atb : 0.0;
		al.gc(L) = (aeb!=0.) ? LC/aeb : 0.0;

		double al_tt_L=al.tt(L); 
		double al_tg_L=al.tg(L);
		double al_gg_L=al.gg(L);
		double al_gc_L=al.gc(L);
		double al_tc_L=al.tc(L);

		for (size_t i = 0; i < Nsims; i++) {
			bias[tttt][i][L]=0.25*ntttt[i]*al_tt_L*al_tt_L*ILC;
			bias[ttte][i][L]=0.5 *nttte[i]*al_tt_L*al_tg_L*ILC;
			bias[ttee][i][L]=0.25*nttee[i]*al_tt_L*al_gg_L*ILC;
			//=ntttb*att*atb/(2.*L+1.);
			//=ntteb*att*aeb/(2.*L+1.);
			bias[tete][i][L]=     ntete[i]*al_tg_L*al_tg_L*ILC;
			bias[teee][i][L]=0.5 *nteee[i]*al_tg_L*al_gg_L*ILC;
			//=ntetb*ate*atb/(2.*L+1.);
			//=nteeb*ate*aeb/(2.*L+1.);
			bias[eeee][i][L]=0.25*neeee[i]*al_gg_L*al_gg_L*ILC;
			//=neetb*aee*atb/(2.*L+1.);
			//=neeeb*aee*aeb/(2.*L+1.);
			bias[tbtb][i][L]=ntbtb[i]*al_tc_L*al_tc_L*ILC;
			bias[tbeb][i][L]=ntbeb[i]*al_tc_L*al_gc_L*ILC;
			bias[ebeb][i][L]=nebeb[i]*al_gc_L*al_gc_L*ILC;
		}

		PyErr_CheckSignals();
	}
	
	return bias;
}
*/

std::vector< std::vector<double> > makeA_BH(std::string stype, PowSpec& wcl, PowSpec& dcl, size_t lmin, size_t lmax, size_t lminCMB) {
	size_t lmaxCMB=dcl.Lmax();
	int num_spec;
	
	int spec = string2esttype(stype);

	if (wcl.Num_specs()==1) {assert(spec==0);}
	if (wcl.Num_specs()==4) {assert(spec<=5);}

	std::vector< std::vector< std::vector<double> > > f(NUM_BH_TYPE,  std::vector< std::vector<double> >(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0)));
	std::vector< std::vector<double> > out(NUM_BH_TYPE_CROSS, std::vector<double>(lmaxCMB+1,0.0));
	
	std::vector< std::vector<double> > invlcl(lmaxCMB+1, std::vector<double>(lmaxCMB+1,0.0));
	#pragma omp parallel for
	for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
		for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
			if (spec==tt) invlcl[l1][l3]=1./dcl.tt(l1)/dcl.tt(l3)*.5;
			else if (spec==te) invlcl[l1][l3]=1./dcl.tt(l1)/dcl.gg(l3);
			else if (spec==ee) invlcl[l1][l3]=1./dcl.gg(l1)/dcl.gg(l3)*.5;
			else if (spec==tb) invlcl[l1][l3]=1./dcl.tt(l1)/dcl.cc(l3);
			else if (spec==eb) invlcl[l1][l3]=1./dcl.gg(l1)/dcl.cc(l3);
		}
	}
	
	double gg, gm, gn, mm, mn, nn;
	for (size_t L=lmin;L<lmax+1;L++) {
		
		// std::cout << " Computing amplitude ... " << (int)(L*100./lmax) << " %\r"; std::cout.flush();
		computef(spec,f[grad],L,wcl,lminCMB,lmaxCMB);
		computef_mask(spec,f[mask],L,wcl,lminCMB,lmaxCMB);
		computef_noise(spec,f[nois],L,lminCMB,lmaxCMB);
		gg=0.; gm=0.; gn=0.; mm=0.; mn=0.; nn=0.;

		if (L>=lmin) {
			#pragma omp parallel for reduction(+:gg,gm,gn,mm,mn,nn) schedule(dynamic, 25)
			for (size_t l1=lminCMB;l1<lmaxCMB+1;l1++) {
				for (size_t l3=lminCMB;l3<lmaxCMB+1;l3++) {
					gg+=f[grad][l1][l3]*f[grad][l1][l3]*invlcl[l1][l3];
					gm+=f[grad][l1][l3]*f[mask][l1][l3]*invlcl[l1][l3];
					gn+=f[grad][l1][l3]*f[nois][l1][l3]*invlcl[l1][l3];
					mm+=f[mask][l1][l3]*f[mask][l1][l3]*invlcl[l1][l3];
					mn+=f[mask][l1][l3]*f[nois][l1][l3]*invlcl[l1][l3];
					nn+=f[nois][l1][l3]*f[nois][l1][l3]*invlcl[l1][l3];
				}
			}
		}
		
		out[gradgrad][L] = (gg!=0.) ? (2.0*L+1.0)/gg : 0.0;
		out[gradmask][L] = (gm!=0.) ? (2.0*L+1.0)/gm : 0.0;
		out[gradnois][L] = (gn!=0.) ? (2.0*L+1.0)/gn : 0.0;
		out[maskmask][L] = (mm!=0.) ? (2.0*L+1.0)/mm : 0.0;
		out[masknois][L] = (mn!=0.) ? (2.0*L+1.0)/mn : 0.0;
		out[noisnois][L] = (nn!=0.) ? (2.0*L+1.0)/nn : 0.0;

		if(PyErr_CheckSignals() == -1) {
			throw invalid_argument( "Keyboard interrupt" );
		}
	}
	
	return out;
}

void alm2map_libsharp(Alm< xcomplex< double > > & almEin, Alm< xcomplex< double > > & almBin, Healpix_Map<double> & mapQout, Healpix_Map<double> & mapUout, size_t spin, arr<double> &weight) {
	int nside = mapQout.Nside();
	size_t lmax = almEin.Lmax();
	sharp_cxxjob<double> job;
	job.set_weighted_Healpix_geometry (nside, &weight[0]);
	job.set_triangular_alm_info (lmax, lmax);
	job.alm2map_spin(&almEin(0,0),&almBin(0,0),&mapQout[0],&mapUout[0],spin,false);
}

void alm2map_libsharp_sY(Alm< xcomplex< double > > & almin, Healpix_Map<double> & mapRout, Healpix_Map<double> & mapIout, int spin, arr<double> &weight) {
	int nside = mapRout.Nside();
	size_t lmax = almin.Lmax();
	Alm< xcomplex< double > > almZ(lmax,lmax);
	sharp_cxxjob<double> job;
	job.set_weighted_Healpix_geometry (nside, &weight[0]);
	job.set_triangular_alm_info (lmax, lmax);
	job.alm2map_spin(&almin(0,0),&almZ(0,0),&mapRout[0],&mapIout[0],abs(spin),false);
}

void map2alm_libsharp(Healpix_Map<double> & mapQin, Healpix_Map<double> & mapUin, Alm< xcomplex< double > > & almEout, Alm< xcomplex< double > > & almBout, size_t spin, arr<double> &weight) {
	int nside = mapQin.Nside();
	int lmax=almEout.Lmax();
	sharp_cxxjob<double> job;
	job.set_weighted_Healpix_geometry (nside, &weight[0]);
	job.set_triangular_alm_info (lmax, lmax);
	job.map2alm_spin(&mapQin[0],&mapUin[0],&almEout(0,0),&almBout(0,0),spin,false);
}

void map2alm_libsharp_sY(Healpix_Map<double> & mapRin, Healpix_Map<double> & mapIin, Alm< xcomplex< double > > & almout, int spin, arr<double> &weight) {
	int nside = mapRin.Nside();
	int lmax=almout.Lmax();
	Alm< xcomplex< double > > almB(lmax,lmax);
	sharp_cxxjob<double> job;
	job.set_weighted_Healpix_geometry (nside, &weight[0]);
	job.set_triangular_alm_info (lmax, lmax);
	job.map2alm_spin(&mapRin[0],&mapIin[0],&almout(0,0),&almB(0,0),abs(spin),false);
	if (spin>=0) {almB.Scale(complex_i);almout.Add(almB);almout.Scale(-1.);}
	else         {almB.Scale(complex_i);almout.Scale(-1.);almout.Add(almB);}
}

void fast_kernel(Alm< xcomplex< double > > & alm1in, Alm< xcomplex< double > > & alm2in, Alm< xcomplex< double > > & almout, size_t spin, int nside, arr<double> &weight) {
	size_t lmaxCMB1=alm1in.Lmax();
	size_t lmaxCMB2=alm1in.Lmax();
	size_t lmax=almout.Lmax();
	
	Healpix_Map<double> map1Q, map1U, map2Q, map2U;
	map1Q.SetNside(nside,RING);
	map1U.SetNside(nside,RING);
	map2Q.SetNside(nside,RING);
	map2U.SetNside(nside,RING);
	
	size_t lmaxtemp=2*nside-1;
	size_t niter=2;
	
	alm2map_libsharp_sY(alm1in,map1Q,map1U,spin,weight);
	alm2map_libsharp_sY(alm2in,map2Q,map2U,spin,weight);
	
	Healpix_Map<double> mapR;
	mapR.SetNside(nside,RING);
	
	#pragma omp parallel for
	for (int i=0; i< map1Q.Npix(); i++) {
		mapR[i]=(map1Q[i]*map2Q[i]+map1U[i]*map2U[i]);
		map1U[i]=(map2Q[i]*map1U[i]-map2U[i]*map1Q[i]);
		if (mapR[i]!=mapR[i]) mapR[i]=0.0;
		if (map1U[i]!=map1U[i]) map1U[i]=0.0;
	}
	
	alm1in.Set(lmaxtemp,lmaxtemp);
	alm2in.Set(lmaxtemp,lmaxtemp);

	map2alm_iter(mapR, alm1in,niter,weight);
	map2alm_iter(map1U,alm2in,niter,weight);

	alm2in.Scale(complex_i);
	alm1in.Add(alm2in);
	
	for (size_t m=0; m<=lmax; ++m) {
		for (size_t l=m; l<=lmax; ++l) {
			almout(l,m)=alm1in(l,m);
		}
	}
}

void fast_kernel_bl(Alm< xcomplex< double > > & alm1in, Alm< xcomplex< double > > & alm2in, Alm< xcomplex< double > > & almout, int nside, arr<double> &weight) {
	size_t lmaxP=alm1in.Lmax();
	size_t lmaxE=alm2in.Lmax();
	size_t lmax=almout.Lmax();
	
	
	Healpix_Map<double> map1Q, map2Q, map2U;
	map1Q.SetNside(nside,RING);
	map2Q.SetNside(nside,RING);
	map2U.SetNside(nside,RING);
	
	size_t lmaxtemp=lmax;//2*nside-1;
	
	alm2map(alm1in,map1Q);
	alm2map_libsharp_sY(alm2in,map2Q,map2U,2,weight);
	
	Healpix_Map<double> mapR, mapI;
	mapR.SetNside(nside,RING);
	
	#pragma omp parallel for
	for (int i=0; i< map1Q.Npix(); i++) {
		mapR[i]=-map1Q[i]*map2Q[i];
		map2Q[i]=-map2U[i]*map1Q[i];
		if (mapR[i]!=mapR[i]) mapR[i]=0.0;
		if (map2Q[i]!=map2Q[i]) map2Q[i]=0.0;
	}
	
	alm1in.Set(lmaxtemp,lmaxtemp);
	alm2in.Set(lmaxtemp,lmaxtemp);

	sharp_cxxjob<double> job;
	job.set_weighted_Healpix_geometry (nside, &weight[0]);
	job.set_triangular_alm_info (lmaxtemp, lmaxtemp);
	job.map2alm_spin(&mapR[0],&map2Q[0],&alm1in(0,0),&alm2in(0,0),2,false);

	alm1in.Scale(-1.*complex_i);
	alm1in.Add(alm2in);
	
	for (size_t m=0; m<=lmax; ++m) {
		for (size_t l=m; l<=lmax; ++l) {
			almout(l,m)=alm1in(l,m);
		}
	}
	
	if(PyErr_CheckSignals() == -1) {
		throw invalid_argument( "Keyboard interrupt" );
	}
}

void compute_term_bl(size_t termnum, Alm< xcomplex< double > > & almB, Alm< xcomplex< double > > & almE, Alm< xcomplex< double > > & almP, size_t lminE, size_t lminP, int nside, arr<double> &weight) {	
	size_t lmaxB=almB.Lmax();
	size_t lmaxE=almE.Lmax();
	size_t lmaxP=almP.Lmax();
	
	Alm< xcomplex< double > > almE_loc(lmaxE,lmaxE), almP_loc(lmaxP,lmaxP);
	
	arr<double> weightE, weightP, lsqr, sgnL;
	weightE.alloc(lmaxE+1);
	weightP.alloc(lmaxP+1);
	weightE.fill(.0);
	weightP.fill(.0);
	size_t spin=0;
	
	lsqr.alloc(max(lmaxE,lmaxP)+1);
	sgnL.alloc(lmaxB+1);
	
	#pragma omp parallel for
	for (size_t l=0; l<lmaxB+1; l++) {
		lsqr[l]=l*(l+1.);
	}
	#pragma omp parallel for
	for (size_t l=0; l<max(lmaxE,lmaxP)+1; l++) {
		sgnL[l]=sgn(l);
	}

	#pragma omp parallel for
	for (size_t l=lminE; l<lmaxE+1; l++) {
		if      (termnum==1) weightE[l]=1.;//wcl.gg(l)/dcl.gg(l);
		else if (termnum==2) weightE[l]=-1.*(l*(l+1.));//wcl.gg(l)/dcl.gg(l);
		else if (termnum==3) weightE[l]=-1.;//wcl.gg(l)/dcl.gg(l);
		else if (termnum==4) weightE[l]=-1.*sgn(l);//wcl.gg(l)/dcl.gg(l);
		else if (termnum==5) weightE[l]=1.*sgn(l)*(l*(l+1.));//wcl.gg(l)/dcl.gg(l);
		else if (termnum==6) weightE[l]=1.*sgn(l);//wcl.gg(l)/dcl.gg(l);
		
		if (weightE[l]!=weightE[l]) weightE[l]=.0;
	}
	
	#pragma omp parallel for
	for (size_t l=lminP; l<lmaxP+1; l++) {
		if      (termnum==1) weightP[l]=1.;//wcl.tt(l)/dcl.tt(l);
		else if (termnum==2) weightP[l]=1.;//wcl.tt(l)/dcl.tt(l);
		else if (termnum==3) weightP[l]=1.*(l*(l+1.));//wcl.tt(l)/dcl.tt(l);
		else if (termnum==4) weightP[l]=1.*sgn(l);//wcl.tt(l)/dcl.tt(l);
		else if (termnum==5) weightP[l]=1.*sgn(l);//wcl.tt(l)/dcl.tt(l);
		else if (termnum==6) weightP[l]=1.*(l*(l+1.))*sgn(l);//wcl.tt(l)/dcl.tt(l);
		
		if (weightP[l]!=weightP[l]) weightP[l]=.0;
	}
	
	#pragma omp parallel for
	for (size_t m=0; m<=lmaxE; ++m) {
		for (size_t l=m; l<=lmaxE; ++l) {
			almE_loc(l,m)=weightE[l]*almE(l,m);
		}
	} 
	#pragma omp parallel for
	for (size_t m=0; m<=lmaxP; ++m) {
		for (size_t l=m; l<=lmaxP; ++l) {
			almP_loc(l,m)=weightP[l]*almP(l,m);
		}
	} 
	
	Alm< xcomplex< double > > almout(lmaxB,lmaxB);
	fast_kernel_bl(almP_loc,almE_loc,almout,nside,weight);

	if (termnum==4 || termnum==5 || termnum==6) {almout.ScaleL(sgnL);}
	if (termnum==1 || termnum==4) almout.ScaleL(lsqr);
	
	almB.Add(almout);
}


void btemp(Alm< xcomplex< double > > &almB, Alm< xcomplex< double > > &almE, Alm< xcomplex< double > > &almP, int lminB, int lminE, int lminP, int nside) {
	size_t lmaxB=almB.Lmax();

	almB.SetToZero();
	
	arr<double> weight;
	weight.alloc(2*nside);
	weight.fill(1.0);
	
	size_t nterms=6;
		
	for (size_t i=1; i<=nterms; i++) {
		compute_term_bl(i, almB, almE, almP, lminE, lminP, nside, weight);
	}
							
	for (size_t m=0; m<=lmaxB; ++m) {
		for (size_t l=m; l<=lmaxB; ++l) {
			almB(l,m)*=.25;
			if (l<lminB) almB(l,m)=0.;
			if (m==0) almB(l,m)=almB(l,m).real();
		}
	}
}



void compute_term_noise(int type, size_t termnum, Alm< xcomplex< double > > & alm1in, Alm< xcomplex< double > > & alm2in, Alm< xcomplex< double > > & almN, PowSpec& wcl, PowSpec& dcl, size_t lmin, size_t lminCMB, int nside, arr<double> &weight) {	
	size_t lmaxCMB=alm1in.Lmax();
	size_t lmax=almN.Lmax();
	
	Alm< xcomplex< double > > alm1(lmaxCMB,lmaxCMB), alm2(lmaxCMB,lmaxCMB);
	
	arr<double> weight1, weight2, lsqr, sgnL;
	weight1.alloc(lmaxCMB+1);
	weight2.alloc(lmaxCMB+1);	
	weight1.fill(.0);
	weight2.fill(.0);
	size_t spin=0;
	
	lsqr.alloc(lmax+1);
	sgnL.alloc(lmax+1);
	
	#pragma omp parallel 
	for (size_t l=0; l<lmax+1; l++) {
		lsqr[l]=l*(l+1.);
		sgnL[l]=sgn(l);
	}
		
	if (type==tt) { 
		#pragma omp parallel 
		for (size_t l=lminCMB; l<lmaxCMB+1; l++) {	
			weight1[l]=1./dcl.tt(l);
			weight2[l]=1./dcl.tt(l);
		}
		spin=0;
	}

	else if (type==te) { 
		#pragma omp parallel 
		for (size_t l=lminCMB; l<lmaxCMB+1; l++) {	
			weight1[l]=1./dcl.tt(l);
			weight2[l]=1./dcl.gg(l);
		}
		spin=2;
	}

	else if (type==ee) { 
		#pragma omp parallel 
		for (size_t l=lminCMB; l<lmaxCMB+1; l++) {	
			weight1[l]=1./dcl.gg(l);
			weight2[l]=1./dcl.gg(l);
		}
		spin=2;
	}

	else if (type==tb) { 
		#pragma omp parallel 
		for (size_t l=lminCMB; l<lmaxCMB+1; l++) {	
			weight1[l]=1./dcl.tt(l);
			weight2[l]=1./dcl.cc(l);
		}
		spin=2;
	}

	else if (type==eb) { 
		#pragma omp parallel 
		for (size_t l=lminCMB; l<lmaxCMB+1; l++) {	
			weight1[l]=1./dcl.gg(l);
			weight2[l]=1./dcl.cc(l);
		}
		spin=2;
	}

	else if (type==bb) { 
		#pragma omp parallel 
		for (size_t l=lminCMB; l<lmaxCMB+1; l++) {	
			weight1[l]=1./dcl.cc(l);
			weight2[l]=1./dcl.cc(l);
		}
		spin=2;
	}

	alm1=alm1in;
	alm1.ScaleL(weight1);
	alm2=alm2in;
	alm2.ScaleL(weight2);
	
	Alm< xcomplex< double > > almout(lmax,lmax);
	fast_kernel(alm1,alm2,almout,spin,nside,weight);
	
	if (type==tt) almout.Scale(.5);
	else if (type==te) almout.Scale(1.);
	else if (type==ee) almout.Scale(.5);
	else if (type==tb) almout.Scale(1.);
	else if (type==eb) almout.Scale(1.);
	else if (type==bb) almout.Scale(.5);
	
	almN.Add(almout);
	
	if(PyErr_CheckSignals() == -1) {
		throw invalid_argument( "Keyboard interrupt" );
	}
}	


void est_noise(Alm< xcomplex< double > > &alm1, Alm< xcomplex< double > > &alm2, std::string stype, Alm< xcomplex< double > > &almN, PowSpec& wcl, PowSpec& dcl, int lmin, int lminCMB, int nside) {
	size_t lmax=almN.Lmax();

	almN.SetToZero();
	int type = string2esttype(stype);
	
	arr<double> weight;
	weight.alloc(2*nside);
	weight.fill(1.0);
	
	size_t nterms=1;
		
	for (size_t i=1; i<=nterms; i++) {
		compute_term_noise(type, i, alm1, alm2, almN, wcl, dcl, lmin, lminCMB, nside, weight);
	}
							
	for (size_t m=0; m<=lmax; ++m) {
		for (size_t l=m; l<=lmax; ++l) {
			if (l<lmin) almN(l,m)=0.;
			if (m==0) almN(l,m)=almN(l,m).real();
		}
	}
}

void compute_term_mask(int type, size_t termnum, Alm< xcomplex< double > > & alm1in, Alm< xcomplex< double > > & alm2in, Alm< xcomplex< double > > & almM, PowSpec& wcl, PowSpec& dcl, size_t lmin, size_t lminCMB1, size_t lminCMB2, size_t lmaxCMB1, size_t lmaxCMB2, int nside, arr<double> &weight) {	
	size_t lmax=almM.Lmax();
	size_t lmaxCMB=max(lmaxCMB1,lmaxCMB2);
	
	Alm< xcomplex< double > > alm1(lmaxCMB,lmaxCMB), alm2(lmaxCMB,lmaxCMB);
	
	arr<double> weight1, weight2, lsqr, sgnL;
	weight1.alloc(lmaxCMB1+1);
	weight2.alloc(lmaxCMB2+1);
	weight1.fill(.0);
	weight2.fill(.0);
	size_t spin=0;
	
	lsqr.alloc(lmax+1);
	sgnL.alloc(lmax+1);
	
	#pragma omp parallel 
	for (size_t l=0; l<lmax+1; l++) {
		lsqr[l]=l*(l+1.);
		sgnL[l]=sgn(l);
	}

	if (type==tt) {
		#pragma omp parallel 
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if 		(termnum==1) weight1[l]=wcl.tt(l)/dcl.tt(l);
			else if (termnum==2) weight1[l]=sgn(l)/dcl.tt(l);
			
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		
		#pragma omp parallel 
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if 		(termnum==1) weight2[l]=1./dcl.tt(l);
			else if (termnum==2) weight2[l]=wcl.tt(l)*sgn(l)/dcl.tt(l);
			
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}
		
		spin=0;
	}
	
	else if (type==te) {
		#pragma omp parallel 
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if		(termnum==1) weight1[l]=wcl.tg(l)/dcl.tt(l);
			else if (termnum==2) weight1[l]=wcl.tg(l)*sgn(l)/dcl.tt(l);
			else if (termnum==3) weight1[l]=1./dcl.tt(l);
			else if (termnum==4) weight1[l]=sgn(l)/dcl.tt(l);
			
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		
		#pragma omp parallel 
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if 		(termnum==1) weight2[l]=1./dcl.gg(l);
			else if (termnum==2) weight2[l]=sgn(l)/dcl.gg(l);
			else if (termnum==3) weight2[l]=wcl.tg(l)/dcl.gg(l);
			else if (termnum==4) weight2[l]=wcl.tg(l)*sgn(l)/dcl.gg(l);
			
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}
		
		if (termnum<=2) spin=2;
		else spin=0;
	}
	
	else if (type==ee) { 
		#pragma omp parallel 
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if  	(termnum==1) weight1[l]=wcl.gg(l)/dcl.gg(l);
			else if (termnum==2) weight1[l]=sgn(l)*wcl.gg(l)/dcl.gg(l);
			else if (termnum==3) weight1[l]=1./dcl.gg(l);
			else if (termnum==4) weight1[l]=sgn(l)/dcl.gg(l);
			
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		
		#pragma omp parallel 
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if (termnum==1) weight2[l]=1./dcl.gg(l);
			else if (termnum==2) weight2[l]=sgn(l)/dcl.gg(l);
			else if (termnum==3) weight2[l]=wcl.gg(l)/dcl.gg(l);
			else if (termnum==4) weight2[l]=sgn(l)*wcl.gg(l)/dcl.gg(l);
			
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}

		spin=2;
	}
	
	else if (type==tb) {
		#pragma omp parallel 
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if 		(termnum==1) weight1[l]=-wcl.tg(l)/dcl.tt(l);
			else if (termnum==2) weight1[l]=sgn(l)*wcl.tg(l)/dcl.tt(l);
			
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		
		#pragma omp parallel 
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if 		(termnum==1) weight2[l]=1./dcl.cc(l);
			else if (termnum==2) weight2[l]=sgn(l)/dcl.cc(l);
			
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}
		
		spin=2;
	}
	
	else if (type==eb) {
		#pragma omp parallel 
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if 		(termnum==1) weight1[l]=-wcl.gg(l)/dcl.gg(l);
			else if (termnum==2) weight1[l]=sgn(l)*wcl.gg(l)/dcl.gg(l);
			
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		
		#pragma omp parallel 
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if 		(termnum==1) weight2[l]=1./dcl.cc(l);
			else if (termnum==2) weight2[l]=sgn(l)/dcl.cc(l);
			
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}
		
		spin=2;
	}
	
	else if (type==bb) { 
		#pragma omp parallel 
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if 		(termnum==1) weight1[l]=wcl.cc(l)/dcl.cc(l);
			else if (termnum==2) weight1[l]=sgn(l)*wcl.cc(l)/dcl.cc(l);
			else if (termnum==3) weight1[l]=1./dcl.cc(l);
			else if (termnum==4) weight1[l]=sgn(l)/dcl.cc(l);

			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		
		#pragma omp parallel 
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if 		(termnum==1) weight2[l]=1./dcl.cc(l);
			else if (termnum==2) weight2[l]=sgn(l)/dcl.cc(l);
			else if (termnum==3) weight2[l]=wcl.cc(l)/dcl.cc(l);
			else if (termnum==4) weight2[l]=sgn(l)*wcl.cc(l)/dcl.cc(l);

			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}

		spin=2;
	}
	
	#pragma omp parallel for
	for (size_t m=0; m<=lmaxCMB1; ++m) {
		for (size_t l=m; l<=lmaxCMB1; ++l) {
			alm1(l,m)=weight1[l]*alm1in(l,m);
		}
	} 
	#pragma omp parallel for
	for (size_t m=0; m<=lmaxCMB2; ++m) {
		for (size_t l=m; l<=lmaxCMB2; ++l) {
			alm2(l,m)=weight2[l]*alm2in(l,m);
		}
	} 
	
	Alm< xcomplex< double > > almout(lmax,lmax);
	fast_kernel(alm1,alm2,almout,spin,nside,weight);

	if (termnum==2 || termnum==4) almout.ScaleL(sgnL);

	if (type==tt) almout.Scale(1.);
	else if (type==te) almout.Scale(1.);
	else if (type==ee) almout.Scale(.5);
	else if (type==tb) almout.Scale(1.*complex_i);
	else if (type==eb) almout.Scale(1.*complex_i);
	else if (type==bb) almout.Scale(.5);
	
	almM.Add(almout);
	
	if(PyErr_CheckSignals() == -1) {
		throw invalid_argument( "Keyboard interrupt" );
	}
}		

void est_mask(Alm< xcomplex< double > > &alm1, Alm< xcomplex< double > > &alm2, std::string stype, Alm< xcomplex< double > > &almM, PowSpec& wcl, PowSpec& dcl, int lmin, int lminCMB1, int lminCMB2,  int lmaxCMB1, int lmaxCMB2, int nside) {
	size_t lmax=almM.Lmax();

	almM.SetToZero();
	int type = string2esttype(stype);
	
	arr<double> weight;
	weight.alloc(2*nside);
	weight.fill(1.0);
	
	size_t nterms;
	if (type==tt || type==tb || type==eb) nterms=2;
	else nterms=4;		
		
	for (size_t i=1; i<=nterms; i++) {
		compute_term_mask(type, i, alm1, alm2, almM, wcl, dcl, lmin, lminCMB1, lminCMB2, lmaxCMB1, lmaxCMB2, nside, weight);
	}
							
	for (size_t m=0; m<=lmax; ++m) {
		for (size_t l=m; l<=lmax; ++l) {
			almM(l,m)*=.5;
			if (l<lmin) almM(l,m)=0.;
			if (m==0) almM(l,m)=almM(l,m).real();
		}
	}
}

std::vector<double> lensBB(std::vector<double> &clEE, std::vector<double> &clDD, size_t lmax_out, bool even) {
	int lmax_EE=clEE.size()-1;
	int lmax_DD=clDD.size()-1;
	
	std::vector<double> out(lmax_out+1, 0.);
	
	std::vector< std::vector<double> > F(lmax_DD+1, std::vector<double>(lmax_EE+1,0.));
	
	for (size_t l1=0;l1<lmax_out+1;l1++) {
		compF(F, l1, 2, lmax_DD+1, lmax_EE+1);
				
		double Aout=0.;
		if (even==true){
			if (l1>=2) {
				#pragma omp parallel for reduction(+:Aout)
				for (size_t L=2;L<lmax_DD+1;L++) {
					for (size_t l2=2;l2<lmax_EE+1;l2++) {
						if ((l1+L+l2)%2==0) {
							Aout+=F[L][l2]*F[L][l2]*clDD[L]*clEE[l2];
						}
					}
				}
			}
		}
		else {
			if (l1>=2) {
				#pragma omp parallel for reduction(+:Aout)
				for (size_t L=2;L<lmax_DD+1;L++) {
					for (size_t l2=2;l2<lmax_EE+1;l2++) {
						if ((l1+L+l2)%2!=0) {
							Aout+=F[L][l2]*F[L][l2]*clDD[L]*clEE[l2];
						}
					}
				}
			}
		}
		out[l1]=Aout*1./(2.*l1+1.);
						
		if(PyErr_CheckSignals() == -1) {
			throw invalid_argument( "Keyboard interrupt" );
		}
				
	}
	
	return out;
}



void compute_term(int type, size_t termnum, Alm< xcomplex< double > > & alm1in, Alm< xcomplex< double > > & alm2in, Alm< xcomplex< double > > & almP, PowSpec& wcl, PowSpec& dcl, size_t lmin, size_t lminCMB1, size_t lminCMB2, size_t lmaxCMB1, size_t lmaxCMB2, int nside, arr<double> &weight) {	
	size_t lmax=almP.Lmax();
	size_t lmaxCMB=max(lmaxCMB1,lmaxCMB2);
	
	Alm< xcomplex< double > > alm1(lmaxCMB,lmaxCMB), alm2(lmaxCMB,lmaxCMB);
	
	arr<double> weight1, weight2, lsqr, sgnL;
	weight1.alloc(lmaxCMB1+1);
	weight2.alloc(lmaxCMB2+1);
	weight1.fill(.0);
	weight2.fill(.0);
	size_t spin=0;
	
	lsqr.alloc(lmax+1);
	sgnL.alloc(lmax+1);
	
	#pragma omp parallel for
	for (size_t l=0; l<lmax+1; l++) {
		lsqr[l]=l*(l+1.);
		sgnL[l]=sgn(l);
	}

	if (type==tt) {
		#pragma omp parallel for
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if      (termnum==1) weight1[l]=-wcl.tt(l)/dcl.tt(l);
			else if (termnum==2) weight1[l]=wcl.tt(l)/dcl.tt(l);
			else if (termnum==3) weight1[l]=wcl.tt(l)*(l*(l+1.))/dcl.tt(l);
			else if (termnum==4) weight1[l]=-(l*(l+1.))*sgn(l)/dcl.tt(l);
			else if (termnum==5) weight1[l]=1.*sgn(l)/dcl.tt(l);
			else if (termnum==6) weight1[l]=1.*sgn(l)/dcl.tt(l);
			
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		
		#pragma omp parallel for
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if      (termnum==1) weight2[l]=(l*(l+1.))/dcl.tt(l);
			else if (termnum==2) weight2[l]=1./dcl.tt(l);
			else if (termnum==3) weight2[l]=1./dcl.tt(l);
			else if (termnum==4) weight2[l]=wcl.tt(l)*sgn(l)/dcl.tt(l);
			else if (termnum==5) weight2[l]=wcl.tt(l)*sgn(l)/dcl.tt(l);
			else if (termnum==6) weight2[l]=wcl.tt(l)*(l*(l+1.))*sgn(l)/dcl.tt(l);
			
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}
		spin=0;
	}
	
	else if (type==te) { 
		#pragma omp parallel for
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if      (termnum==1)  weight1[l]=-wcl.tg(l)/dcl.tt(l);
			else if (termnum==2)  weight1[l]=wcl.tg(l)/dcl.tt(l);
			else if (termnum==3)  weight1[l]=wcl.tg(l)*(l*(l+1.))/dcl.tt(l);
			else if (termnum==4)  weight1[l]=-wcl.tg(l)*sgn(l)/dcl.tt(l);
			else if (termnum==5)  weight1[l]=wcl.tg(l)*sgn(l)/dcl.tt(l);
			else if (termnum==6)  weight1[l]=wcl.tg(l)*sgn(l)*(l*(l+1.))/dcl.tt(l);
			else if (termnum==7)  weight1[l]=-(l*(l+1.))/dcl.tt(l);
			else if (termnum==8)  weight1[l]=1./dcl.tt(l);
			else if (termnum==9)  weight1[l]=1./dcl.tt(l);
			else if (termnum==10) weight1[l]=-(l*(l+1.))*sgn(l)/dcl.tt(l);
			else if (termnum==11) weight1[l]=sgn(l)/dcl.tt(l);
			else if (termnum==12) weight1[l]=sgn(l)*1./dcl.tt(l);
		
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		#pragma omp parallel for
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if      (termnum==1)  weight2[l]=(l*(l+1.))/dcl.gg(l);
			else if (termnum==2)  weight2[l]=1./dcl.gg(l);
			else if (termnum==3)  weight2[l]=1./dcl.gg(l);
			else if (termnum==4)  weight2[l]=(l*(l+1.))*sgn(l)/dcl.gg(l);
			else if (termnum==5)  weight2[l]=sgn(l)/dcl.gg(l);
			else if (termnum==6)  weight2[l]=1.*sgn(l)/dcl.gg(l);
			else if (termnum==7)  weight2[l]=wcl.tg(l)/dcl.gg(l);
			else if (termnum==8)  weight2[l]=wcl.tg(l)/dcl.gg(l);
			else if (termnum==9)  weight2[l]=(l*(l+1.))*wcl.tg(l)/dcl.gg(l);
			else if (termnum==10) weight2[l]=sgn(l)*wcl.tg(l)/dcl.gg(l);
			else if (termnum==11) weight2[l]=sgn(l)*wcl.tg(l)/dcl.gg(l);
			else if (termnum==12) weight2[l]=(l*(l+1.))*sgn(l)*wcl.tg(l)/dcl.gg(l);
		
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}
		if (termnum<=6) spin=2;
		else spin=0;
	}
	
	else if (type==ee) { 
		#pragma omp parallel for
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if (termnum==1)       weight1[l]=-wcl.gg(l)/dcl.gg(l);
			else if (termnum==2)  weight1[l]=wcl.gg(l)/dcl.gg(l);
			else if (termnum==3)  weight1[l]=wcl.gg(l)*(l*(l+1.))/dcl.gg(l);
			else if (termnum==4)  weight1[l]=-wcl.gg(l)*sgn(l)/dcl.gg(l);
			else if (termnum==5)  weight1[l]=wcl.gg(l)*sgn(l)/dcl.gg(l);
			else if (termnum==6)  weight1[l]=wcl.gg(l)*(l*(l+1.))*sgn(l)/dcl.gg(l);
			else if (termnum==7)  weight1[l]=-(l*(l+1.))/dcl.gg(l);
			else if (termnum==8)  weight1[l]=1./dcl.gg(l);
			else if (termnum==9)  weight1[l]=1./dcl.gg(l);
			else if (termnum==10) weight1[l]=-sgn(l)*(l*(l+1.))/dcl.gg(l);
			else if (termnum==11) weight1[l]=sgn(l)*1./dcl.gg(l);
			else if (termnum==12) weight1[l]=sgn(l)*1./dcl.gg(l);
			
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		#pragma omp parallel for
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if      (termnum==1)  weight2[l]=(l*(l+1.))/dcl.gg(l);
			else if (termnum==2)  weight2[l]=1./dcl.gg(l);
			else if (termnum==3)  weight2[l]=1./dcl.gg(l);
			else if (termnum==4)  weight2[l]=(l*(l+1.))*sgn(l)/dcl.gg(l);
			else if (termnum==5)  weight2[l]=sgn(l)*1./dcl.gg(l);
			else if (termnum==6)  weight2[l]=sgn(l)*1./dcl.gg(l);
			else if (termnum==7)  weight2[l]=wcl.gg(l)/dcl.gg(l);
			else if (termnum==8)  weight2[l]=wcl.gg(l)/dcl.gg(l);
			else if (termnum==9)  weight2[l]=wcl.gg(l)*(l*(l+1.))/dcl.gg(l);
			else if (termnum==10) weight2[l]=wcl.gg(l)*sgn(l)/dcl.gg(l);
			else if (termnum==11) weight2[l]=wcl.gg(l)*sgn(l)/dcl.gg(l);
			else if (termnum==12) weight2[l]=wcl.gg(l)*(l*(l+1.))*sgn(l)/dcl.gg(l);
			
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}
		spin=2;
	}
	
	else if (type==tb) { 
		#pragma omp parallel for
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if      (termnum==1) weight1[l]=wcl.tg(l)/dcl.tt(l);
			else if (termnum==2) weight1[l]=-wcl.tg(l)/dcl.tt(l);
			else if (termnum==3) weight1[l]=-wcl.tg(l)*(l*(l+1.))/dcl.tt(l);
			else if (termnum==4) weight1[l]=-wcl.tg(l)*sgn(l)/dcl.tt(l);
			else if (termnum==5) weight1[l]=wcl.tg(l)*sgn(l)/dcl.tt(l);
			else if (termnum==6) weight1[l]=wcl.tg(l)*(l*(l+1.))*sgn(l)/dcl.tt(l);
			
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		#pragma omp parallel for
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if      (termnum==1) weight2[l]=(l*(l+1.))/dcl.cc(l);
			else if (termnum==2) weight2[l]=1./dcl.cc(l);
			else if (termnum==3) weight2[l]=1./dcl.cc(l);
			else if (termnum==4) weight2[l]=(l*(l+1.))*sgn(l)/dcl.cc(l);
			else if (termnum==5) weight2[l]=sgn(l)*1./dcl.cc(l);
			else if (termnum==6) weight2[l]=sgn(l)*1./dcl.cc(l);
			
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}
		spin=2;
	}
	
	else if (type==eb) { 
		#pragma omp parallel for
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if      (termnum==1) weight1[l]=wcl.gg(l)/dcl.gg(l);
			else if (termnum==2) weight1[l]=-wcl.gg(l)/dcl.gg(l);
			else if (termnum==3) weight1[l]=-wcl.gg(l)*(l*(l+1.))/dcl.gg(l);
			else if (termnum==4) weight1[l]=-wcl.gg(l)*sgn(l)/dcl.gg(l);
			else if (termnum==5) weight1[l]=wcl.gg(l)*sgn(l)/dcl.gg(l);
			else if (termnum==6) weight1[l]=wcl.gg(l)*(l*(l+1.))*sgn(l)/dcl.gg(l);
			
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		#pragma omp parallel for
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if      (termnum==1) weight2[l]=(l*(l+1.))/dcl.cc(l);
			else if (termnum==2) weight2[l]=1./dcl.cc(l);
			else if (termnum==3) weight2[l]=1./dcl.cc(l);
			else if (termnum==4) weight2[l]=(l*(l+1.))*sgn(l)/dcl.cc(l);
			else if (termnum==5) weight2[l]=sgn(l)*1./dcl.cc(l);
			else if (termnum==6) weight2[l]=sgn(l)*1./dcl.cc(l);
			
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}
		spin=2;
	}
	
	else if (type==bb) {
		#pragma omp parallel for
		for (size_t l=lminCMB1; l<lmaxCMB1+1; l++) {
			if      (termnum==1)  weight1[l]=-wcl.cc(l)/dcl.cc(l);
			else if (termnum==2)  weight1[l]=wcl.cc(l)/dcl.cc(l);
			else if (termnum==3)  weight1[l]=wcl.cc(l)*(l*(l+1.))/dcl.cc(l);
			else if (termnum==4)  weight1[l]=-wcl.cc(l)*sgn(l)/dcl.cc(l);
			else if (termnum==5)  weight1[l]=wcl.cc(l)*sgn(l)/dcl.cc(l);
			else if (termnum==6)  weight1[l]=wcl.cc(l)*(l*(l+1.))*sgn(l)/dcl.cc(l);
			else if (termnum==7)  weight1[l]=-(l*(l+1.))/dcl.cc(l);
			else if (termnum==8)  weight1[l]=1./dcl.cc(l);
			else if (termnum==9)  weight1[l]=1./dcl.cc(l);
			else if (termnum==10) weight1[l]=-sgn(l)*(l*(l+1.))/dcl.cc(l);
			else if (termnum==11) weight1[l]=sgn(l)*1./dcl.cc(l);
			else if (termnum==12) weight1[l]=sgn(l)*1./dcl.cc(l);
			
			if (weight1[l]!=weight1[l]) weight1[l]=.0;
		}
		#pragma omp parallel for
		for (size_t l=lminCMB2; l<lmaxCMB2+1; l++) {
			if      (termnum==1)  weight2[l]=(l*(l+1.))/dcl.cc(l);
			else if (termnum==2)  weight2[l]=1./dcl.cc(l);
			else if (termnum==3)  weight2[l]=1./dcl.cc(l);
			else if (termnum==4)  weight2[l]=(l*(l+1.))*sgn(l)/dcl.cc(l);
			else if (termnum==5)  weight2[l]=sgn(l)*1./dcl.cc(l);
			else if (termnum==6)  weight2[l]=sgn(l)*1./dcl.cc(l);
			else if (termnum==7)  weight2[l]=wcl.cc(l)/dcl.cc(l);
			else if (termnum==8)  weight2[l]=wcl.cc(l)/dcl.cc(l);
			else if (termnum==9)  weight2[l]=wcl.cc(l)*(l*(l+1.))/dcl.cc(l);
			else if (termnum==10) weight2[l]=wcl.cc(l)*sgn(l)/dcl.cc(l);
			else if (termnum==11) weight2[l]=wcl.cc(l)*sgn(l)/dcl.cc(l);
			else if (termnum==12) weight2[l]=wcl.cc(l)*(l*(l+1.))*sgn(l)/dcl.cc(l);
			
			if (weight2[l]!=weight2[l]) weight2[l]=.0;
		}
		spin=2;
	}
	
	#pragma omp parallel for
	for (size_t m=0; m<=lmaxCMB1; ++m) {
		for (size_t l=m; l<=lmaxCMB1; ++l) {
			alm1(l,m)=weight1[l]*alm1in(l,m);
		}
	} 
	#pragma omp parallel for
	for (size_t m=0; m<=lmaxCMB2; ++m) {
		for (size_t l=m; l<=lmaxCMB2; ++l) {
			alm2(l,m)=weight2[l]*alm2in(l,m);
		}
	} 
	
	Alm< xcomplex< double > > almout(lmax,lmax);
	fast_kernel(alm1,alm2,almout,spin,nside,weight);

	if (termnum==2 || termnum==5 || termnum==8 || termnum==11) almout.ScaleL(lsqr);
	if (termnum==4 || termnum==5 || termnum==6 || termnum==10 || termnum==11 || termnum==12) almout.ScaleL(sgnL);

	if (type==tt) almout.Scale(1.);
	else if (type==te) almout.Scale(1.);
	else if (type==ee) almout.Scale(.5);
	else if (type==tb) almout.Scale(1.*complex_i);
	else if (type==eb) almout.Scale(1.*complex_i);
	else if (type==bb) almout.Scale(.5);
	
	almP.Add(almout);
	
	if(PyErr_CheckSignals() == -1) {
		throw invalid_argument( "Keyboard interrupt" );
	}
}


