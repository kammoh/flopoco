#include "Field.hpp"
#include <iostream>

namespace flopoco {
    Field::Field(unsigned int wX, unsigned int wY, bool signedIO) : wX_(wX), wY_(wY), signedIO_(signedIO), missing_(wX * wY), highestLine_(0U), lowestFinishedLine_(0U) {
        field_.resize(wY_);
        for(unsigned int i = 0; i < wY_; i++) {
            field_[i].resize(wX_);
            for(unsigned int j = 0; j < wX_; j++) {
                field_[i][j] = false;
            }
        }

        cout << signedIO_ << " TEST IO" << endl;
        setCursor(0U, 0U);
    }

    Field::Field(const Field &copy) {
        wX_ = copy.wX_;
        wY_ = copy.wY_;
        missing_ = copy.missing_;
        cursor_ =  Cursor(copy.cursor_);

        field_.resize(wY_);
        for(unsigned int i = 0U; i < wY_; i++) {
            field_[i].resize(wX_);
            for(unsigned int j = 0; j < wX_; j++) {
                field_[i][j] = copy.field_[i][j];
            }
        }

        signedIO_ = copy.signedIO_;
        highestLine_ = copy.highestLine_;
    }

    Field::~Field() {
        field_.clear();
    }

    void Field::reset() {
        if(missing_ == wX_ * wY_) {
            return;
        }

        for(unsigned int i = 0U; i < wY_; i++) {
            for(unsigned int j = 0U; j < wX_; j++) {
                field_[i][j] = false;
            }
        }

        setCursor(0, 0);
        missing_ = wX_ * wY_;

        highestLine_ = 0;

        resetCursorBehaviour();
    }

    void Field::reset(Field& target) {
        if(target.wX_ != wX_ || target.wY_ != wY_) {
            return;
        }

        cout << "Before reset" << endl;
        //target.printField();
        cout << "State" << endl;
        //printField();

        cout << highestLine_ << endl;

        missing_ = target.missing_;

        resetField(target);

        highestLine_ = target.highestLine_;
        setCursor(target.cursor_);

        cout << "After reset" << endl;
        //printField();
    }

    unsigned int Field::checkTilePlacement(const Cursor coord, BaseMultiplierCategory* tile) {
        unsigned int sizeX = tile->wX_DSPexpanded(coord.first, coord.second, wX_, wY_, signedIO_);
        unsigned int sizeY = tile->wY_DSPexpanded(coord.first, coord.second, wX_, wY_, signedIO_);
        unsigned int endX = coord.first + sizeX;
        unsigned int endY = coord.second + sizeY;
        unsigned int maxX = std::min(endX, wX_);
        unsigned int maxY = std::min(endY, wY_);

        unsigned int covered = 0;

        for(unsigned int i = coord.second; i < maxY; i++) {
            for(unsigned int j = coord.first; j < maxX; j++) {
                //check if tile could cover this area
                if(!tile->shape_contribution(j, i, coord.first, coord.second, wX_, wY_, signedIO_)) {
                    cout << "coord " << j << " " << i << " is not covered!" << endl;
                    continue;
                }

                if(field_[i][j]) {
                    return 0;
                }

                covered++;
            }
        }

        cout << "Trying to place tile with size " << sizeX << " " << sizeY << " at position " << coord.first << " " << coord.second << endl;
        cout << "It would cover " << covered << endl;

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

        cout << "Trying to place DSPBlock with size " << param.getTileXWordSize() << " " << param.getTileYWordSize() << " at position " << coord.first << " " << coord.second << endl;
        cout << "Size would be " << size.first << " " << size.second << endl;

        return size;
    }

    Cursor Field::placeTileInField(const Cursor coord, BaseMultiplierCategory* tile) {
        unsigned int sizeX = tile->wX_DSPexpanded(coord.first, coord.second, wX_, wY_, signedIO_);
        unsigned int sizeY = tile->wY_DSPexpanded(coord.first, coord.second, wX_, wY_, signedIO_);
        unsigned int endX = coord.first + sizeX;
        unsigned int endY = coord.second + sizeY;
        unsigned int maxX = std::min(endX, wX_);
        unsigned int maxY = std::min(endY, wY_);

        for (unsigned int i = coord.second; i < maxY; i++) {
            for (unsigned int j = coord.first; j < maxX; j++) {
                //check if tile could cover this area and if area is free
                if (tile->shape_contribution(j, i, coord.first, coord.second, wX_, wY_, signedIO_) && !field_[i][j]) {
                    missing_--;
                    field_[i][j] = true;
                }
            }
        }

        if (maxY > highestLine_) {
            highestLine_ = maxY;
        }


        //printField();
        updateCursor();

        cout << "Placed tile with size " << sizeX << " " << sizeY << " at position " << coord.first << " "
             << coord.second << " new coord is " << cursor_.first << " " << cursor_.second << endl;
        cout << tile->getType() << endl;

        return cursor_;
    }

    bool Field::isFull() {
        return missing_ == 0U;
    }

    unsigned int Field::getMissing() {
        return missing_;
    }

    unsigned int Field::getHighestLine() {
        return highestLine_;
    }

    unsigned int Field::getWidth() {
        return wX_;
    }

    unsigned int Field::getHeight() {
        return wY_;
    }

    bool Field::getCell(Cursor cursor) {
        return field_[cursor.second][cursor.first];
    }

    void Field::setLine(unsigned int line, vector<bool> &vec) {
        field_[line] = vec;
    }

    unsigned int Field::getMissingLine() {
        unsigned int missing_ = 0U;

        for(unsigned int i = cursor_.first; i < wX_; i++) {
            if(field_[cursor_.second][i]) {
                break;
            }
            missing_++;
        }

        return missing_;
    }

    unsigned int Field::getMissingHeight() {
        unsigned int missing_ = 0U;

        for(unsigned int i = cursor_.second; i < wY_; i++) {
            if(field_[i][cursor_.first]) {
                break;
            }
            missing_++;
        }

        return missing_;
    }

    Cursor Field::getCursor() {
        return cursor_;
    }

    void Field::setCursor(unsigned int x, unsigned int y)  {
        if(x > wX_) {
            x = 0U;
        }

        if(y > wY_) {
            y = 0U;
        }

        cursor_.first = x;
        cursor_.second = y;
    }

    void Field::setCursor(Cursor target) {
        setCursor(target.first, target.second);
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
}
