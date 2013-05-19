//tubestage.h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	// Passive components
	V Vi;
	C Ci;
	C Ck;
	C Co;
	R Ro;
	R Rg;
	R Ri;
	R Rk;
	V E;

	T e;

	//Circuit description
	//->Gate
	ser S0;
	inv I0;
	par P0;
	ser S1;
	inv I1;

	//->Cathode
	par I3;

	//->Plate
	ser S2;
	inv I4;
	par P2;

	//Triode element
	Triode v;
} Circuit;


float tubestage(float input, Circuit* c, float tubedrive); 

#ifdef __cplusplus
}
#endif
