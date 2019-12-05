#include "MultiplierTileCollection.hpp"
#include "BaseMultiplierDSP.hpp"
#include "BaseMultiplierLUT.hpp"
#include "BaseMultiplierXilinx2xk.hpp"
#include "BaseMultiplierIrregularLUTXilinx.hpp"
#include "BaseMultiplierDSPSuperTilesXilinx.hpp"

using namespace std;
namespace flopoco {

    MultiplierTileCollection::MultiplierTileCollection(Target *target, BaseMultiplierCollection *bmc, int wX, int wY, bool superTile, bool use2xk, bool useirregular) {
        //cout << bmc->size() << endl;

        MultTileCollection.push_back( new BaseMultiplierDSP(24, 17, 1));
        MultTileCollection.push_back( new BaseMultiplierDSP(17, 24, 1));

        MultTileCollection.push_back( new BaseMultiplierLUT(3, 3));
        MultTileCollection.push_back( new BaseMultiplierLUT(2, 3));
        MultTileCollection.push_back( new BaseMultiplierLUT(3, 2));
        MultTileCollection.push_back( new BaseMultiplierLUT(1, 2));
        MultTileCollection.push_back( new BaseMultiplierLUT(2, 1));
        MultTileCollection.push_back( new BaseMultiplierLUT(1, 1));

        if(superTile){
            for(int i = 1; i <= 12; i++) {
                MultTileCollection.push_back(
                        new BaseMultiplierDSPSuperTilesXilinx((BaseMultiplierDSPSuperTilesXilinx::TILE_SHAPE) i));
            }
        }

        if(use2xk){
            for(int x = 4; x <= wX; x++) {
                MultTileCollection.push_back(
                        new BaseMultiplierXilinx2xk(x,2));
            }
            for(int y = 4; y <= wX; y++) {
                MultTileCollection.push_back(
                        new BaseMultiplierXilinx2xk(2,y));
            }
        }

        if(useirregular){
            for(int i = 1; i <= 8; i++) {
                MultTileCollection.push_back(
                        new BaseMultiplierIrregularLUTXilinx((BaseMultiplierIrregularLUTXilinx::TILE_SHAPE) i));
            }
        }

/*        for(int i = 0; i < (int)bmc->size(); i++)
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
*/
//        cout << MultTileCollection.size() << endl;

        for(BaseMultiplierCategory *mult : MultTileCollection)
        {
            cout << mult->cost() << " ";
            cout << mult->getType() << endl;
        }

    }

    BaseMultiplierCategory* MultiplierTileCollection::superTileSubtitution(vector<BaseMultiplierCategory*> mtc, int rx1, int ry1, int lx1, int ly1, int rx2, int ry2, int lx2, int ly2){
        for(int i = 0; i < (int)mtc.size(); i++)
        {
            if(mtc[i]->getDSPCost(1,1) == 2){
                int id = mtc[i]->isSuperTile(rx1, ry1, lx1, ly1, rx2, ry2, lx2, ly2);
                if(id){
                    return mtc[i+id];
                }
            }
        }
        return nullptr;
    }

}