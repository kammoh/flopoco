#include "NearestPointCursorField.hpp"

namespace flopoco {
    NearestPointCursorField::NearestPointCursorField() : BaseFieldState(), searchRadius_{0}, segmentPos_{0} {
    }

    void NearestPointCursorField::setField(Field *field) {
        BaseFieldState::setField(field);

        unsigned int wX = field->getWidth();
        unsigned int wY = field->getHeight();

        unsigned int maxDistance = ((int) std::sqrt((float)((wX * wX) + (wY * wY)))) + 1;
        coordsLUT_.resize(maxDistance);

        for(unsigned int i = 0; i < maxDistance; i++) {
            //create LUT for this range
            Cursor coord(0, i);
            unsigned int diameter = i * i;

            while(coord.first <= i && coord.second >= 0) {
                if(coord.second < wY && coord.first < wX) {
                    float distance = std::sqrt((float)((coord.first * coord.first) + (coord.second * coord.second)));
                    if((distance == 0  && i == 0) || distance > (i - 1)) {
                        // cout << "Added to list" << endl;
                        NextCoord nextCoord;
                        nextCoord.distance = distance;
                        nextCoord.coord = Cursor(coord);
                        coordsLUT_[i].push_back(nextCoord);
                    }
                }

                if(checkAction(coord, 1, 0, diameter)) {
                    continue;
                }

                if(checkAction(coord, 0, -1, diameter)) {
                    continue;
                }

                if(coord.first == i && coord.second == 0) {
                    break;
                }

                cout << "Seems like we couldn't find a new direction" << endl;
                cout << cursor_.first << " " << cursor_.second << endl;
                exit(0);
            }

            std::sort(coordsLUT_[i].begin(), coordsLUT_[i].end(), [](const NextCoord& a, const NextCoord& b) -> bool { return a.distance < b.distance; });
        }

        /*for(vector<NextCoord>& list : coordsLUT_) {
            for(NextCoord& c: list) {
                cout << c.coord.first << ", " << c.coord.second << " = " << c.distance << " ";
            }
            cout << endl;
        }*/
    }

    void NearestPointCursorField::reset(Field *field, ID id, unsigned int missing) {
        BaseFieldState::reset(field, id, missing);

        searchRadius_ = 0U;
        segmentPos_ = 0U;
    }

    void NearestPointCursorField::reset(BaseFieldState &baseState) {
        BaseFieldState::reset(baseState);

        searchRadius_ = 0U;
        segmentPos_ = 0U;

        NearestPointCursorField* cpy = dynamic_cast<NearestPointCursorField*>(&baseState);
        if(cpy != nullptr) {
            searchRadius_ = cpy->searchRadius_;
            segmentPos_ = cpy->segmentPos_;
        }
    }

    void NearestPointCursorField::updateCursor() {
        if(missing_ == 0) {
            setCursor(0,0);
            return;
        }

        // cout << "Requesting new position " << " " << missing_ << endl;

        // field_->printField();

        while(true) {
            for(unsigned int i = segmentPos_; i < coordsLUT_[searchRadius_].size(); i++) {
                NextCoord& next = coordsLUT_[searchRadius_][i];
                if(!field_->checkPosition(next.coord.first, next.coord.second, *this)) {
                    setCursor(next.coord);
                    segmentPos_ = i;
                    return;
                }
                else {
                    // cout << next.coord.first << ", " << next.coord.second << " already set" << endl;
                }
            }

            searchRadius_++;
            segmentPos_ = 0;

            /*cout << "Updated search radius " << searchRadius_ << endl;
            for(NextCoord& c: coordsLUT_[searchRadius_]) {
                cout << c.coord.first << ", " << c.coord.second << " = " << c.distance << " ";
            }
            cout << endl; */

            if(searchRadius_ == coordsLUT_.size()) {
                cout << "REACHED MAX SEARCH RADIUS" << endl;
                break;
            }
        }
    }

    void NearestPointCursorField::checkCircleSegment(unsigned int radius) {
        Cursor coord(0, radius);
        unsigned int diameter = radius * radius;

        // printField();

        unsigned int wX = field_->getWidth();
        unsigned int wY = field_->getHeight();

        while(coord.first <= radius && coord.second >= 0) {
            //cout << coord.first << " " << coord.second << endl;

            if(coord.second < wY && coord.first < wX) {
                //cout << "Is in matrix" << endl;
                float distance = std::sqrt((float)((coord.first * coord.first) + (coord.second * coord.second)));
                if(((distance == 0  && radius == 0) || distance > (radius - 1)) && !field_->checkPosition(coord.first, coord.second, *this)) {
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
}