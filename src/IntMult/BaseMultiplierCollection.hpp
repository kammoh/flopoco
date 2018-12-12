#ifndef BaseMultiplierCollection_HPP
#define BaseMultiplierCollection_HPP

#include <string>
#include <iostream>
#include <string>
#include <vector>
#include "Target.hpp"
#include "BaseMultiplier.hpp"

namespace flopoco {
	typedef base_multiplier_id_t size_t;
    class BaseMultiplierCollection {

	public:
		BaseMultiplierCollection(Target *target, unsigned int wX, unsigned int wY, bool pipelineDSPs=false);
        ~BaseMultiplierCollection();

        BaseMultiplier* getBaseMultiplier(base_multiplier_id_t multRef);

		vector<base_multiplier_id_t> getMultipliersIDByArea();
		
        string getName(){ return uniqueName_; }
		
    private:

	
        Target* target;

        string srcFileName; //for debug outputs

        string uniqueName_; /**< useful only to enable same kind of reporting as for FloPoCo operators. */

        unsigned int wX;
        unsigned int wY;
	
        vector<BaseMultiplier*> baseMultipliers; //the list of base mutlipliers, index is identical to shape
	};
}
#endif
