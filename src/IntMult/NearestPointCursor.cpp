#include "NearestPointCursor.hpp"

namespace flopoco {
    NearestPointCursor::NearestPointCursor() : BaseFieldState(), searchRadius_{0}, segmentPos_{0} {
    }

    void NearestPointCursor::setField(Field *field) {
        BaseFieldState::setField(field);

        coordsLUT_.clear();
        // cout << "Rebuilding LUTS" << endl;

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
                cout << c.coord.first << "," << c.coord.second << " = " << c.distance << " ";
            }
            cout << endl;
        }*/
    }

    void NearestPointCursor::reset(Field *field, ID id, unsigned int missing) {
        BaseFieldState::reset(field, id, missing);

        searchRadius_ = 0U;
        segmentPos_ = 0U;
    }

    void NearestPointCursor::reset(BaseFieldState &baseState) {
        BaseFieldState::reset(baseState);

        searchRadius_ = 0U;
        segmentPos_ = 0U;

        NearestPointCursor* cpy = dynamic_cast<NearestPointCursor*>(&baseState);
        if(cpy != nullptr) {
            searchRadius_ = cpy->searchRadius_;
            segmentPos_ = cpy->segmentPos_;
        }
    }

    void NearestPointCursor::updateCursor() {
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

            // cout << "Updated search radius " << searchRadius_ << endl;

            if(searchRadius_ == coordsLUT_.size()) {
                cout << "REACHED MAX SEARCH RADIUS" << endl;
                field_->printField();
                break;
            }
            else {
                /* for(NextCoord& c: coordsLUT_[searchRadius_]) {
                    cout << c.coord.first << "," << c.coord.second << " = " << c.distance << " ";
                }
                cout << endl; */
            }
        }
    }

    bool NearestPointCursor::checkAction(Cursor& coord, int deltaX, int deltaY, unsigned int diameter) {
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