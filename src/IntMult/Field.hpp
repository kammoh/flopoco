#ifndef FLOPOCO_FIELD_HPP
#define FLOPOCO_FIELD_HPP

#include <utility>
#include <vector>
#include "BaseMultiplierCategory.hpp"

using namespace std;

namespace flopoco {
    class Field {
    public:
        Field(unsigned int wX, unsigned int wY, bool signedIO);
        Field(const Field &copy);

        ~Field();

        void printField();
        //TODO: pass down "updateCursor" as function pointer / virtual function
        pair<unsigned int, unsigned int> placeTileInField(const pair<unsigned int, unsigned int> coord, BaseMultiplierCategory* tile);
        unsigned int checkTilePlacement(const pair<unsigned int, unsigned int> coord, BaseMultiplierCategory* tile);
        pair<unsigned int, unsigned int> checkDSPPlacement(const pair<unsigned int, unsigned int> coord, const BaseMultiplierParametrization& param);
        bool isFull();
        unsigned int getMissing();
        unsigned int getMissingLine();
        unsigned int getMissingHeight();
        pair<unsigned int, unsigned int> getCursor();
        void setCursor(unsigned int x, unsigned int y);
        void setCursor(pair<unsigned int, unsigned int> target);
        void reset();
        void reset(Field& target);

    private:
        void updateCursor();
        vector<vector<bool>> field;
        pair<unsigned int, unsigned int> cursor;
        unsigned int wX;
        unsigned int wY;
        bool signedIO;
        unsigned int missing;
        unsigned int highestLine;
    };
}


#endif
