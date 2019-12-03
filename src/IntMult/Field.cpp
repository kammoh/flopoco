#include "Field.hpp"
#include <iostream>

namespace flopoco {
    Field::Field(unsigned int wX_, unsigned int wY_, bool signedIO_) : wX(wX_), wY(wY_), signedIO(signedIO_), missing(wX_ * wY_), highestLine(0) {
        field.resize(wY);
        for(unsigned int i = 0; i < wY; i++) {
            field[i].resize(wX);
            for(unsigned int j = 0; j < wX; j++) {
                field[i][j] = false;
            }
        }

        setCursor(0, 0);
    }

    Field::Field(const Field &copy) {
        wX = copy.wX;
        wY = copy.wY;
        missing = copy.missing;
        cursor =  pair<unsigned int, unsigned int>(copy.cursor);

        field.resize(wY);
        for(unsigned int i = 0; i < wY; i++) {
            field[i].resize(wX);
            for(unsigned int j = 0; j < wX; j++) {
                field[i][j] = copy.field[i][j];
            }
        }

        highestLine = copy.highestLine;
    }

    Field::~Field() {
        field.clear();
    }

    void Field::reset() {
        for(unsigned int i = 0; i < wY; i++) {
            for(unsigned int j = 0; j < wX; j++) {
                field[i][j] = false;
            }
        }

        setCursor(0, 0);
        missing = wX * wY;

        highestLine = 0;
    }

    void Field::reset(Field& target) {
        if(target.wX != wX || target.wY != wY) {
            return;
        }

        cout << "Before reset" << endl;
        target.printField();
        cout << "State" << endl;
        printField();

        cout << highestLine << endl;

        missing = target.missing;

        //fill up needed space
        if(cursor.second < target.cursor.second) {
            for(unsigned int i = cursor.second; i < target.cursor.second; i++) {
                field[i] = target.field[i];
            }
        }

        //empty upper part
        if(highestLine > target.highestLine) {
            for(unsigned int i = target.highestLine; i < highestLine; i++) {
                field[i] = target.field[i];
            }
        }

        //copy half filled area
        for(unsigned int i = max(target.cursor.second - 1, 0); i < target.highestLine; i++) {
            field[i] = target.field[i];
        }

        highestLine = target.highestLine;
        setCursor(target.cursor);

        cout << "After reset" << endl;
        printField();
    }

    unsigned int Field::checkTilePlacement(const pair<unsigned int, unsigned int> coord, BaseMultiplierCategory* tile) {
        unsigned int sizeX = tile->wX_DSPexpanded(coord.first, coord.second, wX, wY, signedIO);
        unsigned int sizeY = tile->wY_DSPexpanded(coord.first, coord.second, wX, wY, signedIO);
        unsigned int endX = coord.first + sizeX;
        unsigned int endY = coord.second + sizeY;
        unsigned int maxX = endX > wX ? wX : endX;
        unsigned int maxY = endY > wY ? wY : endY;

        unsigned int covered = 0;

        for(unsigned int i = coord.second; i < maxY; i++) {
            for(unsigned int j = coord.first; j < maxX; j++) {
                //check if tile could cover this area
                if(!tile->shape_contribution(j, i, coord.first, coord.second, wX, wY, signedIO)) {
                    cout << "coord " << j << " " << i << " is not covered!" << endl;
                    continue;
                }

                if(field[i][j]) {
                    return 0;
                }

                covered++;
            }
        }

        cout << "Trying to place tile with size " << sizeX << " " << sizeY << " at position " << coord.first << " " << coord.second << endl;
        cout << "It would cover " << covered << endl;

        return covered;
    }

    pair<unsigned int, unsigned int> Field::checkDSPPlacement(const pair<unsigned int, unsigned int> coord, const BaseMultiplierParametrization& param) {
        unsigned int endX = coord.first + param.getTileXWordSize();
        unsigned int endY = coord.second + param.getTileYWordSize();
        unsigned int maxX = endX > wX ? wX : endX;
        unsigned int maxY = endY > wY ? wY : endY;

        pair<unsigned int, unsigned int> size(param.getTileXWordSize(), 0);

        for(unsigned int i = coord.second; i < maxY; i++) {
            unsigned int currentSize = 0;
            for(unsigned int j = coord.first; j < maxX; j++) {
                if(field[i][j]) {
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

    pair<unsigned int, unsigned int> Field::placeTileInField(const pair<unsigned int, unsigned int> coord, BaseMultiplierCategory* tile) {
        unsigned int sizeX = tile->wX_DSPexpanded(coord.first, coord.second, wX, wY, signedIO);
        unsigned int sizeY = tile->wY_DSPexpanded(coord.first, coord.second, wX, wY, signedIO);
        unsigned int endX = coord.first + sizeX;
        unsigned int endY = coord.second + sizeY;
        unsigned int maxX = endX > wX ? wX : endX;
        unsigned int maxY = endY > wY ? wY : endY;

        for(unsigned int i = coord.second; i < maxY; i++) {
            for (unsigned int j = coord.first; j < maxX; j++) {
                //check if tile could cover this area and if area is free
                if (tile->shape_contribution(j, i, coord.first, coord.second, wX, wY, signedIO) && !field[i][j]) {
                    missing--;
                    field[i][j] = true;
                }
            }
        }

        if(maxY > highestLine) {
            highestLine = maxY;
        }


        printField();
        updateCursor();

        cout << "Placed tile with size " << sizeX << " " << sizeY << " at position " << coord.first << " " << coord.second << " new coord is " << cursor.first << " " << cursor.second << endl;
        cout << tile->getType() << endl;

        return cursor;
    }

    void Field::updateCursor() {
        while(cursor.second != wY) {
            //try to find next free position in the current line
            for(unsigned int i = 0; i < wX; i++) {
                if(!field[cursor.second][i]) {
                    cursor.first = i;
                    return;
                }
            }

            //no free position in this line
            cursor.second++;
        }

        cursor.first = 0;
        cursor.second = 0;
    }

    bool Field::isFull() {
        return missing == 0;
    }

    unsigned int Field::getMissing() {
        return missing;
    }

    unsigned int Field::getMissingLine() {
        unsigned int missing = 0;

        for(unsigned int i = cursor.first; i < wX; i++) {
            if(field[cursor.second][i]) {
                break;
            }
            missing++;
        }

        return missing;
    }

    unsigned int Field::getMissingHeight() {
        unsigned int missing = 0;

        for(unsigned int i = cursor.second; i < wY; i++) {
            if(field[i][cursor.first]) {
                break;
            }
            missing++;
        }

        return missing;
    }

    pair<unsigned int, unsigned int> Field::getCursor() {
        return cursor;
    }

    void Field::setCursor(unsigned int x, unsigned int y)  {
        if(x > wX) {
            x = 0;
        }

        if(y > wY) {
            y = 0;
        }

        cursor.first = x;
        cursor.second = y;
    }

    void Field::setCursor(pair<unsigned int, unsigned int> target) {
        setCursor(target.first, target.second);
    }

    void Field::printField() {
        //TODO: mirror output
        for(auto v: field) {
            for(auto c: v) {
                cout << c;
            }
            cout << endl;
        }
    }
}
