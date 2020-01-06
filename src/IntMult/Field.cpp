#include "Field.hpp"
#include <iostream>

namespace flopoco {
    Field::Field(unsigned int wX, unsigned int wY, bool signedIO, FieldState& baseState) : wX_(wX), wY_(wY), signedIO_(signedIO), currentStateID_(0U), baseState_{&baseState} {
        field_.resize(wY_);
        for(unsigned int i = 0; i < wY_; i++) {
            field_[i].resize(wX_);
            for (unsigned int j = 0; j < wX_; j++) {
                field_[i][j] = currentStateID_;
            }
        }
        currentStateID_++;

        initFieldState(baseState);
        baseID_ = baseState.getID();
    }

    Field::Field(const Field &copy) {
        wX_ = copy.wX_;
        wY_ = copy.wY_;

        field_.resize(wY_);
        for(unsigned int i = 0U; i < wY_; i++) {
            field_[i].resize(wX_);
            for(unsigned int j = 0; j < wX_; j++) {
                field_[i][j] = copy.field_[i][j];
            }
        }

        signedIO_ = copy.signedIO_;
    }

    Field::~Field() {
        field_.clear();
    }

    void Field::initFieldState(FieldState& fieldState) {
        fieldState.reset(this, currentStateID_++, wX_ * wY_);
        fieldState.setCursor(0U, 0U);
        fieldState.setField(this);
    }

    void Field::updateStateID(Field::FieldState &fieldState) {
        fieldState.setID(currentStateID_++);
    }

    void Field::reset() {
        initFieldState(*baseState_);
        baseID_ = baseState_->getID();
    }

    unsigned int Field::checkTilePlacement(const Cursor coord, BaseMultiplierCategory* tile, FieldState& fieldState) {
        unsigned int sizeX = tile->wX_DSPexpanded(coord.first, coord.second, wX_, wY_, signedIO_);
        unsigned int sizeY = tile->wY_DSPexpanded(coord.first, coord.second, wX_, wY_, signedIO_);
        unsigned int endX = coord.first + sizeX;
        unsigned int endY = coord.second + sizeY;
        unsigned int maxX = std::min(endX, wX_);
        unsigned int maxY = std::min(endY, wY_);

        unsigned int covered = 0;
        ID fieldID = fieldState.getID();

        for(unsigned int i = coord.second; i < maxY; i++) {
            for(unsigned int j = coord.first; j < maxX; j++) {
                //check if tile could cover this area
                if(!tile->shape_contribution(j, i, coord.first, coord.second, wX_, wY_, signedIO_)) {
                    // cout << "coord " << j << " " << i << " is not covered!" << endl;
                    continue;
                }

                if(field_[i][j] == fieldID || field_[i][j] == baseID_) {
                    return 0;
                }

                covered++;
            }
        }

        // cout << "Trying to place tile with size " << sizeX << " " << sizeY << " at position " << coord.first << " " << coord.second << endl;
        // cout << "It would cover " << covered << endl;

        return covered;
    }

    Cursor Field::checkDSPPlacement(const Cursor coord, const BaseMultiplierParametrization& param) {
        unsigned int endX = coord.first + param.getTileXWordSize();
        unsigned int endY = coord.second + param.getTileYWordSize();
        unsigned int maxX = std::min(endX, wX_);
        unsigned int maxY = std::min(endY, wY_);

        Cursor size(param.getTileXWordSize(), 0);

        for(unsigned int i = coord.second; i < maxY; i++) {
            unsigned int currentSize = 0;
            for(unsigned int j = coord.first; j < maxX; j++) {
                if(field_[i][j]) {
                    break;
                }

                currentSize++;
            }

            //seems like it is not possible to use this line
            if(currentSize == 0) {
                break;
            }

            if(size.first > currentSize) {
                size.first = currentSize;
            }

            size.second++;
        }

        // cout << "Trying to place DSPBlock with size " << param.getTileXWordSize() << " " << param.getTileYWordSize() << " at position " << coord.first << " " << coord.second << endl;
        // cout << "Size would be " << size.first << " " << size.second << endl;

        return size;
    }

    Cursor Field::placeTileInField(const Cursor coord, BaseMultiplierCategory* tile, FieldState& fieldState) {
        unsigned int sizeX = tile->wX_DSPexpanded(coord.first, coord.second, wX_, wY_, signedIO_);
        unsigned int sizeY = tile->wY_DSPexpanded(coord.first, coord.second, wX_, wY_, signedIO_);
        unsigned int endX = coord.first + sizeX;
        unsigned int endY = coord.second + sizeY;
        unsigned int maxX = std::min(endX, wX_);
        unsigned int maxY = std::min(endY, wY_);

        ID fieldID = fieldState.getID();
        unsigned int updateMissing = 0U;

        // cout << fieldState.getCursor().first << " " << fieldState.getCursor().second << endl;

        for (unsigned int i = coord.second; i < maxY; i++) {
            for (unsigned int j = coord.first; j < maxX; j++) {
                //check if tile could cover this area and if area is free
                if (tile->shape_contribution(j, i, coord.first, coord.second, wX_, wY_, signedIO_) && field_[i][j] != fieldID && field_[i][j] != baseID_) {
                    field_[i][j] = fieldID;
                    updateMissing++;
                }
            }
        }

        fieldState.decreaseMissing(updateMissing);

        // TODO: update highest line
        /*if (maxY > highestLine_) {
            highestLine_ = maxY;
        }*/

        //printField();
        fieldState.updateCursor();

        // printField();

        // cout << "Placed tile with size " << sizeX << " " << sizeY << " at position " << coord.first << " " << coord.second << " new coord is " << fieldState.getCursor().first << " " << fieldState.getCursor().second << endl;
        // cout << tile->getType() << endl;

        return fieldState.getCursor();
    }

    unsigned int Field::getWidth() {
        return wX_;
    }

    unsigned int Field::getHeight() {
        return wY_;
    }

    void Field::setLine(unsigned int line, vector<ID> &vec) {
        field_[line] = vec;
    }

    unsigned int Field::getMissingLine(FieldState& fieldState) {
        unsigned int missing = 0U;
        Cursor c = fieldState.getCursor();
        ID fieldID = fieldState.getID();

        for(unsigned int i = c.first; i < wX_; i++) {
            if(field_[c.second][i] == fieldID || field_[c.second][i] == baseID_) {
                break;
            }
            missing++;
        }

        return missing;
    }

    unsigned int Field::getMissingHeight(FieldState& fieldState) {
        unsigned int missing = 0U;
        Cursor c = fieldState.getCursor();
        ID fieldID = fieldState.getID();

        for(unsigned int i = c.second; i < wY_; i++) {
            if(field_[i][c.first] == fieldID || field_[i][c.first] == baseID_) {
                break;
            }
            missing++;
        }

        return missing;
    }

    void Field::printField() {
        //TODO: mirror output
        for(auto v: field_) {
            for(auto c: v) {
                cout << c;
            }
            cout << endl;
        }
    }

    bool Field::checkPosition(unsigned int x, unsigned int y, Field::FieldState &fieldState) {
        return (field_[y][x] == fieldState.getID() || field_[y][x] == baseID_);
    }

    void Field::printField(FieldState& fieldState) {
        ID fieldID = fieldState.getID();

        for(auto v: field_) {
            for(auto c: v) {
                cout << (c == fieldID || c == baseID_);
            }
            cout << endl;
        }
    }
}
