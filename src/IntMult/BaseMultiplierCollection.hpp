#ifndef BaseMultiplierCollection_HPP
#define BaseMultiplierCollection_HPP

#include <string>
#include <iostream>
#include <string>
#include <vector>
#include "Target.hpp"
#include "BaseMultiplier.hpp"

namespace flopoco {


    class BaseMultiplierCollection {

	public:
        BaseMultiplierCollection(Target *target, unsigned int wX, unsigned int wY, bool pipelineDSPs);
        ~BaseMultiplierCollection();

        BaseMultiplier* getBaseMultiplier(int shape);
		
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
