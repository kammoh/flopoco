#include "Operator.hpp"

class IntMultiplierLUT : public Operator
{
public:
	IntMultiplierLUT(Operator *parentOp, Target* target, bool isSignedX, bool isSignedY, int wX, int wY, bool flipXY=false);
private:
	int wX, wY;
	bool isSignedX, isSignedY;
	bool flipXY;
	mpz_class function(int x);
};
