#include "BaseMultiplierCollection.hpp"
#include "BaseMultiplierLUT.hpp"
#include "BaseMultiplier2xk.hpp"
#include "BaseMultiplierDSP.hpp"
#include "BaseMultiplierDSPSuperTilesXilinx.hpp"

namespace flopoco {

BaseMultiplierCollection::BaseMultiplierCollection(Target* target, unsigned int wX, unsigned int wY, bool pipelineDSPs){
    srcFileName = "BaseMultiplierCollection";
    uniqueName_ = "BaseMultiplierCollection";
	
    this->target = target;
    this->wX = wX;
    this->wY = wY;


    //create DSP-based multipliers:
	baseMultipliers.push_back(new BaseMultiplierDSP(false, false, 17, 24, pipelineDSPs));
    baseMultipliers.push_back(new BaseMultiplierDSP(false, false, 24, 17, pipelineDSPs));

    //create DSP-based super tiles: TODO make correct ordering
    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_I, pipelineDSPs)); //_2 = I
    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_G, pipelineDSPs)); //_3 = G

    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_K, pipelineDSPs)); //_4 = K
    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_E, pipelineDSPs)); //_5 = E

    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_H, pipelineDSPs)); //_6 = H
    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_J, pipelineDSPs)); //_7 = J

    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_A, pipelineDSPs)); //_8 = A
    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_D, pipelineDSPs)); //_9 = D

    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_C, pipelineDSPs)); //_10 = C
    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_B, pipelineDSPs)); //_11 = B

    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_L, pipelineDSPs)); //_12 = L
    baseMultipliers.push_back(new BaseMultiplierDSPSuperTilesXilinx(false, false, BaseMultiplierDSPSuperTilesXilinx::SHAPE_F, pipelineDSPs)); //_13 = F

    for(unsigned int width = 4; width <= 17; width++){
        baseMultipliers.push_back(new BaseMultiplierDSP(false, false, width, width, pipelineDSPs));
    }

    //create logic-based multipliers:
    //baseMultipliers.push_back(new BaseMultiplierLUT(false,false,3,3)); //3x3 LUT-based multiplier (six LUT6)
    //in old solution files the 3x3 is missing. In problemfiles from thursday 3x3 should be there

    baseMultipliers.push_back(new BaseMultiplierLUT(false,false,2,3)); //2x3 LUT-based multiplier (three LUT6)
    baseMultipliers.push_back(new BaseMultiplierLUT(false,false,3,2)); //3x2 LUT-based multiplier (three LUT6)
    baseMultipliers.push_back(new BaseMultiplierLUT(false,false,1,2)); //1x2 LUT-based multiplier (two AND gates)
    baseMultipliers.push_back(new BaseMultiplierLUT(false,false,2,1)); //2x1 LUT-based multiplier (two AND gates)
    baseMultipliers.push_back(new BaseMultiplierLUT(false,false,2,2)); //2x2 LUT-based multiplier (two LUT6) not in new versions
    baseMultipliers.push_back(new BaseMultiplierLUT(false,false,1,1)); //1x1 LUT-based multiplier (an AND gate)


    unsigned int maxWidth = (wX > wY ? wX : wY);
    for(int k=4; k <= maxWidth; k++) //ToDo: adjust limits
    {
        baseMultipliers.push_back(new BaseMultiplier2xk(false, false, k, false)); //2xk LUT/carry-chain-based multiplier
        baseMultipliers.push_back(new BaseMultiplier2xk(false, false, k, true));  //kx2 LUT/carry-chain-based multiplier
    }




    int i=0;
    cout << "The following multiplier shapes were generated:" << endl;

    for(BaseMultiplier* bm : baseMultipliers)
    {
        cout << "shape " << i++ << ": " << bm->getXWordSize() << "x" << bm->getYWordSize() << " of type " << bm->getName() << endl;
    }
}

BaseMultiplier* BaseMultiplierCollection::getBaseMultiplier(int shape)
{
    if(shape < ((int) baseMultipliers.size()))
        return baseMultipliers[shape];
    else
        return nullptr;
}

BaseMultiplierCollection::~BaseMultiplierCollection()
{
    for(BaseMultiplier* bm : baseMultipliers)
    {
        delete bm;
    }
}

	
}   //end namespace flopoco

