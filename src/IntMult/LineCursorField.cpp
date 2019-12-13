#include "LineCursorField.hpp"

namespace flopoco {
    LineCursorField::LineCursorField(unsigned int wX, unsigned int wY, bool signedIO) : Field(wX, wY, signedIO) {
    }

    void LineCursorField::resetCursorBehaviour() {
        //nothing to do here
    }

    void LineCursorField::updateCursor() {
        while(cursor_.second != wY_) {
            //try to find next free position in the current line
            for(unsigned int i = 0U; i < wX_; i++) {
                if(!field_[cursor_.second][i]) {
                    cursor_.first = i;
                    return;
                }
            }

            //no free position in this line
            cursor_.second++;
        }

        cursor_.first = 0U;
        cursor_.second = 0U;
    }

    void LineCursorField::resetField(Field& target) {
        //dirty stuff
        LineCursorField& castTarget = static_cast<LineCursorField&>(target);

        //fill up needed space
        if(cursor_.second < castTarget.cursor_.second) {
            for(unsigned int i = cursor_.second; i < castTarget.cursor_.second; i++) {
                field_[i] = castTarget.field_[i];
            }
        }

        //empty upper part
        if(highestLine_ > castTarget.highestLine_) {
            for(unsigned int i = castTarget.highestLine_; i < highestLine_; i++) {
                field_[i] = castTarget.field_[i];
            }
        }

        //copy half filled area
        for(unsigned int i = max(castTarget.cursor_.second - 1, 0); i < castTarget.highestLine_; i++) {
            field_[i] = castTarget.field_[i];
        }
    }
}