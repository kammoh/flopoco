#include "TargetBaseMultSpecialisation.hpp"
#include "Targets/Kintex7.hpp"
#include "XilinxBaseMultiplier2xk.hpp"

using namespace std;

namespace flopoco{
	/*
	 * @param bmVec the vector to populate with the target specific
	 * baseMultipliers
	 */
	vector<BaseMultiplierCategory*> TargetSpecificBaseMultiplier(
			Target const* target
		)
	{
		vector<BaseMultiplierCategory*> ret;
		if (typeid(*target) == typeid(Kintex7)) {
			ret.push_back(new XilinxBaseMultiplier2xk(128));
		}
		return ret;
	}
}
