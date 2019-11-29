#ifndef FLOPOCO_MULTIPLIERTILECOLLECTION_HPP
#define FLOPOCO_MULTIPLIERTILECOLLECTION_HPP
#include "BaseMultiplierCategory.hpp"
#include "BaseMultiplierCollection.hpp"

namespace flopoco {
    class MultiplierTileCollection {

    public:
        MultiplierTileCollection(Target *target, BaseMultiplierCollection* bmc);
        vector<BaseMultiplierCategory*> MultTileCollection;

    };
}
#endif //FLOPOCO_MULTIPLIERTILECOLLECTION_HPP
