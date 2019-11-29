#include "MultiplierTileCollection.hpp"
#include "BaseMultiplierDSP.hpp"
#include "BaseMultiplierLUT.hpp"
#include "BaseMultiplierXilinx2xk.hpp"
#include "BaseMultiplierIrregularLUTXilinx.hpp"
#include "BaseMultiplierDSPSuperTilesXilinx.hpp"

using namespace std;
namespace flopoco {

    MultiplierTileCollection::MultiplierTileCollection(Target *target, BaseMultiplierCollection *bmc) {
        //cout << bmc->size() << endl;

        MultTileCollection.push_back( new BaseMultiplierDSP(24, 17, 1));
        MultTileCollection.push_back( new BaseMultiplierDSP(17, 24, 1));

        MultTileCollection.push_back( new BaseMultiplierLUT(3, 3));
        MultTileCollection.push_back( new BaseMultiplierLUT(2, 3));
        MultTileCollection.push_back( new BaseMultiplierLUT(3, 2));
        MultTileCollection.push_back( new BaseMultiplierLUT(1, 2));
        MultTileCollection.push_back( new BaseMultiplierLUT(2, 1));
        MultTileCollection.push_back( new BaseMultiplierLUT(1, 1));

        for(int i = 0; i < (int)bmc->size(); i++)
        {
            cout << bmc->getBaseMultiplier(i).getType() << endl;

            if( (bmc->getBaseMultiplier(i).getType().compare("BaseMultiplierDSPSuperTilesXilinx")) == 0){
                for(int i = 1; i <= 12; i++) {
                    MultTileCollection.push_back(
                            new BaseMultiplierDSPSuperTilesXilinx((BaseMultiplierDSPSuperTilesXilinx::TILE_SHAPE) i));
                }
            }

            if( (bmc->getBaseMultiplier(i).getType().compare("BaseMultiplierIrregularLUTXilinx")) == 0){
                for(int i = 1; i <= 8; i++) {
                    MultTileCollection.push_back(
                            new BaseMultiplierIrregularLUTXilinx((BaseMultiplierIrregularLUTXilinx::TILE_SHAPE) i));
                }
            }

        }

//        cout << MultTileCollection.size() << endl;
/*
        for(BaseMultiplierCategory *mult : MultTileCollection)
        {
            cout << mult->cost() << " ";
            cout << mult->getType() << endl;
        }
*/
    }

}