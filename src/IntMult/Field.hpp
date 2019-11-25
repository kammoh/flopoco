#ifndef FLOPOCO_FIELD_HPP
#define FLOPOCO_FIELD_HPP

#include <utility>
#include <vector>
#include "BaseMultiplierCategory.hpp"

using namespace std;

namespace flopoco {
    class Field {
    public:
        Field(unsigned int wX, unsigned int wY);
        Field(const Field &copy);

        ~Field();

        void printField();
        pair<unsigned int, unsigned int> placeTileInField(const pair<unsigned int, unsigned int> coord, const BaseMultiplierParametrization& tile);
        unsigned int checkTilePlacement(const pair<unsigned int, unsigned int> coord, const BaseMultiplierParametrization& tile);
        bool isFull();
        unsigned int getMissing();
        unsigned int getMissingLine();
        unsigned int getMissingHeight();
        pair<unsigned int, unsigned int> getCursor();
        void setCursor(unsigned int x, unsigned int y);
        void reset();

    private:
        void updatePosition();
        vector<vector<bool>> field;
        pair<unsigned int, unsigned int> cursor;
        unsigned int wX;
        unsigned int wY;
        unsigned int missing;
    };
}


#endif
