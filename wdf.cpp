/*
wdf.cpp  Test classes for zamvalve
Copyright (C) 2013  Damien Zammit

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "wdf.h"

inline T sanitize_denormal(T value) {
	if (isnan(value)) {
		DUMP(fprintf(stderr,"Broken number ( %e )\n",value));
		return SMALL;
	}
	return value;
}

/////////////////////////////////////////////

int main(){ 
	T Fs = 48000.0;
	T N = Fs/3;
	T gain = 3.0;
	T f0 = 1001.0;
	T input[48000] = { 0.0 };
	int i;
	for (i = 0; i < N; ++i) {
		input[i] = gain*sin(2.0*M_PI*f0/Fs*i);
	}

	// Passive components
	T ci = 0.0000001;	//100nF
	T ck = 0.00001;		//10uF
	T co = 0.00000001;	//10nF
	T ro = 1000000.0;	//1Mohm
	T rp = 100000.0;	//100kohm
	T rg = 20000.0;		//20kohm
	T ri = 1000000.0;	//1Mohm
	T rk = 1000.0;		//1kohm
	T e = 250.0;		//250V

	V Vi = V(0.0,1000.0);	//1kohm internal resistance
	C Ci = C(ci, Fs);
	C Ck = C(ck, Fs);
	C Co = C(co, Fs);
	R Ro = R(ro);
	R Rg = R(rg);
	R Ri = R(ri);
	R Rk = R(rk);
	V E = V(e, rp);

#if 0
//Mod
/*
	//->Gate
	ser S0 = ser(&Ci, &Vi);
	inv I0 = inv(&S0);
	par P0 = par(&I0, &Ri);
	ser S1 = ser(&Rg, &P0);
	inv I1 = inv(&S1);

	//->Cathode
	par I3 = par(&Ck, &Rk);

	//->Plate
	ser S2 = ser(&Co, &Ro);
	inv I4 = inv(&S2);
	inv EE = inv(&E);
	par P2 = par(&I4, &EE);
*/
	ser S0 = ser(&Ci, &Vi);
	//inv I0 = inv(&S0);
	inv RRi = inv(&Ri);
	par P0 = par(&S0, &RRi);
	inv RRg = inv(&Rg);
	ser S1 = ser(&RRg, &P0);
	inv I1 = inv(&S1);

	par S3 = par(&Ck, &Rk);
	inv I3 = inv(&S3);

	ser S2 = ser(&Co, &Ro);
	inv I4 = inv(&S2);
	//inv EE = inv(&E);
	par P2 = par(&I4, &E);

#else
//Official
	//->Gate
	ser S0 = ser(&Ci, &Vi);
	inv I0 = inv(&S0);
	par P0 = par(&I0, &Ri);
	ser S1 = ser(&Rg, &P0);
	inv I1 = inv(&S1);

	//->Cathode
	par I3 = par(&Ck, &Rk);

	//->Plate
	ser S2 = ser(&Co, &Ro);
	inv I4 = inv(&S2);
	par P2 = par(&I4, &E);
#endif	

	// 12AX7 triode model RSD-1
	Triode v;
	v.g = 2.242e-3;
	v.mu = 103.2;
	v.gamma = 1.26;
	v.c = 3.4;
	v.gg = 6.177e-4;
	v.e = 1.314;
	v.cg = 9.901;
	v.ig0 = 8.025e-8;

	printf("0j\t  Vi\t  Ro\t  Vg\t  Vk\t  Vp\t  Ri\t  Rk\t  Rg\t  E\t  Co\t  Ck\t  EA\t  RoA\t  Ig\t  Ik\t  Ip\n");
	
	//Initial voltage guess for triode nodes
	Vi.e = input[0];
	I1.waveUp();
	I3.waveUp();
	P2.waveUp();
	v.G.WD = -I1.WU;
	v.K.WD = -I3.WU;
	v.P.WD = -P2.WU;

	v.vg = v.G.WD;
	v.vk = v.K.WD;
	v.vp = v.P.WD;

	for (int j = 0; j < N; ++j) {
		//Step 1: read input sample as voltage for the source
		Vi.e = input[j];

		//Step 2: propagate waves up to the triode and push values into triode element
		I1.waveUp();
		I3.waveUp();
		P2.waveUp();
		v.G.WD = I1.WU;  //invert polarity for root node 
		v.K.WD = I3.WU;  //invert polarity for root node
		v.P.WD = P2.WU;  //invert polarity for root node
		v.G.PortRes = I1.PortRes;
		v.K.PortRes = I3.PortRes;
		v.P.PortRes = P2.PortRes;

		//Step 3: compute wave reflections inside the triode
		T vg0, vg1, vp0, vp1;

		
		vg0 = v.vg;
		vg1 = vg0 + v.fg(vg0);
		v.vg = v.secantfg(&vg0, &vg1);
		//v.vg = v.newtonfg(&vg0);

		vp0 = v.vp;
		vp1 = vp0 + v.fp(vp0);
		v.vp = v.secantfp(&vp0, &vp1);
		//v.vp = v.newtonfp(&vp0);

		v.vk = v.fk();

		v.G.WU = 2.0*v.vg-v.G.WD;
		v.K.WU = 2.0*v.vk-v.K.WD;
		v.P.WU = 2.0*v.vp-v.P.WD;

//		fprintf(stderr,"%f %f %f :g %f %f :p %f %f :k %f %f\n",v.vg, v.vp, v.vk, v.G.WD, v.G.WU, v.P.WD, v.P.WU, v.K.WD, v.K.WU);

		//Step 4: push new waves down from the triode element
		I1.WD = v.G.WU;  //invert polarity for root node
		I3.WD = v.K.WU;  //invert polarity for root node
		P2.WD = v.P.WU;   //inverting this causes NaN output
		
		I1.setWD(I1.WD);
		I3.setWD(I3.WD);
		P2.setWD(P2.WD); 
	
		
		//Step 5: save triode voltages for next loop - not necessary
	//	v.vg = v.G.Voltage();
	//	v.vk = v.K.Voltage();
	//	v.vp = v.P.Voltage();
		
		//Step 6: output | egrep "^\+" to gnuplot, or egrep "^0" for viewing tabulated data
		printf("%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t: %.4f\t%.4f\t%.4f a:%.2f: %.2f b:%.2f: %.2f\n",j/Fs, input[j], Ro.Voltage(), I1.Voltage(),I3.Voltage(),P2.Voltage(),Ri.Voltage(),Rk.Voltage(),Rg.Voltage(),E.Voltage(),Co.Voltage(), Ck.Voltage(), E.Current(), Ro.Current(), v.G.Current(),v.K.Current(),v.P.Current(),v.P.WD,P2.WU, v.P.WU,P2.WD);
		printf("+%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", j/Fs, input[j], Ro.Voltage(),v.vg,v.vk,v.vp,Ri.Voltage(),Rk.Voltage(),Rg.Voltage(),E.Voltage(),Co.Voltage(), Ck.Voltage(), E.Current(), Ro.Current(), v.G.Current(),v.K.Current(),v.P.Current());
	}
}

