#ifndef FLOPOCO_FIELD_HPP
#define FLOPOCO_FIELD_HPP

#include <utility>
#include <vector>
#include "BaseMultiplierCategory.hpp"

using namespace std;

namespace flopoco {
    typedef pair<unsigned int, unsigned int> Cursor;

    class Field {
    public:
        Field(unsigned int wX, unsigned int wY, bool signedIO);
        Field(const Field &copy);

        ~Field();

        void printField();
        //TODO: pass down "updateCursor" as function pointer / virtual function
        Cursor placeTileInField(const Cursor coord, BaseMultiplierCategory* tile);
        unsigned int checkTilePlacement(const Cursor coord, BaseMultiplierCategory* tile);
        Cursor checkDSPPlacement(const Cursor coord, const BaseMultiplierParametrization& param);
        bool isFull();
        unsigned int getMissing();
        unsigned int getMissingLine();
        unsigned int getMissingHeight();
        unsigned int getHighestLine();
        unsigned int getWidth();
        unsigned int getHeight();
        bool getCell(Cursor cursor);
        void setLine(unsigned int line, vector<bool>& vec);
        Cursor getCursor();
        void setCursor(unsigned int x, unsigned int y);
        void setCursor(Cursor target);
        void reset();
        void reset(Field& target);

    protected:
        vector<vector<bool>> field_;
        Cursor cursor_;
        unsigned int wX_;
        unsigned int wY_;
        bool signedIO_;
        unsigned int missing_;
        unsigned int highestLine_;
        unsigned int lowestFinishedLine_;

        virtual void updateCursor() = 0;
        virtual void resetField(Field& target) = 0;
        virtual void resetCursorBehaviour() = 0;
    };
}


#endif
