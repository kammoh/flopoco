#include "NearestPointCursorField.hpp"

namespace flopoco {
    NearestPointCursorField::NearestPointCursorField(unsigned int wX, unsigned int wY, bool signedIO) : Field(wX, wY, signedIO), searchRadius_{0} {
    }

    void NearestPointCursorField::resetCursorBehaviour() {
        searchRadius_ = 0;
        nextCoords_.clear();
    }

    void NearestPointCursorField::updateCursor() {
        if(missing_ == 0) {
            setCursor(0,0);
            return;
        }

        while(true) {
            //check if cursor vector is empty
            if(nextCoords_.size() == 0) {
                // cout << "Requesting new segment" << endl;
                checkCircleSegment(searchRadius_);
                searchRadius_++;
                // cout << "Segmentsize " << nextCoords_.size() << endl;
            }

            for(unsigned int i = 0; i < nextCoords_.size(); i++) {
                NextCoord& next = nextCoords_.front();
                Cursor coord(next.coord);
                nextCoords_.pop_front();
                if(!field_[coord.second][coord.first]) {
                    setCursor(coord);
                    // cout << "Updated cursor to " << coord.first << " " << coord.second << endl;
                    //exit(0);
                    return;
                }
            }
        }
    }

    void NearestPointCursorField::checkCircleSegment(unsigned int radius) {
        Cursor coord(0, radius);
        unsigned int diameter = radius * radius;

        // printField();

        while(coord.first <= radius && coord.second >= 0) {
            //cout << coord.first << " " << coord.second << endl;

            if(coord.second < field_.size() && coord.first < field_[1].size()) {
                //cout << "Is in matrix" << endl;
                float distance = std::sqrt((float)((coord.first * coord.first) + (coord.second * coord.second)));
                if(((distance == 0  && radius == 0) || distance > (radius - 1)) && !field_[coord.second][coord.first]) {
                    // cout << "Added to list" << endl;
                    NextCoord nextCoord;
                    nextCoord.distance = distance;
                    nextCoord.coord = Cursor(coord);
                    nextCoords_.push_back(nextCoord);
                }
            }

            if(checkAction(coord, 1, 0, diameter)) {
                continue;
            }

            if(checkAction(coord, 0, -1, diameter)) {
                continue;
            }

            if(coord.first == radius && coord.second == 0) {
                break;
            }

            cout << "Seems like we couldn't find a new direction" << endl;
            cout << cursor_.first << " " << cursor_.second << endl;
            exit(0);
        }

        nextCoords_.sort([](const NextCoord& a, const NextCoord& b) -> bool { return a.distance < b.distance; });

        /*for(NextCoord& c: nextCoords_) {
            cout << c.coord.first << " " << c.coord.second << " " << c.distance << endl;
        }*/
    }

    bool NearestPointCursorField::checkAction(Cursor& coord, int deltaX, int deltaY, unsigned int diameter) {
        unsigned int x = (unsigned int) ((int)coord.first + deltaX);
        unsigned int y = (unsigned int) ((int)coord.second + deltaY);
        if((x * x) + (y * y) <= diameter) {
            coord.first = x;
            coord.second = y;
            return true;
        }
        return false;
    }

    void NearestPointCursorField::resetField(Field& target) {
        //dirty stuff
        NearestPointCursorField& castTarget = static_cast<NearestPointCursorField&>(target);

        //just copy everything for now
        for(unsigned int i = 0; i < wY_; i++) {
            field_[i] = castTarget.field_[i];
        }

        searchRadius_ = castTarget.searchRadius_;
        nextCoords_.clear();
        nextCoords_ = castTarget.nextCoords_;
    }
}