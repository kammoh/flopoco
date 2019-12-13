#include "NearestPointCursorField.hpp"

namespace flopoco {
    NearestPointCursorField::NearestPointCursorField(unsigned int wX, unsigned int wY, bool signedIO) : Field(wX, wY, signedIO), searchRadius_{0} {
    }

    void NearestPointCursorField::resetCursorBehaviour() {
        searchRadius_ = 0;
    }

    void NearestPointCursorField::updateCursor() {
        if(missing_ == 0) {
            setCursor(0,0);
            return;
        }

        cout << "New cursor test " << missing_ << endl;
        printField();

        double bestCost = DBL_MAX;
        Cursor best(0, 0);
        bool found = false;

        searchRadius_ = 0;

        //until the highest edge
        while(searchRadius_ != std::max(wX_, wY_)) {
            unsigned int searchRadiusX = std::min(searchRadius_, wX_);
            unsigned int searchRadiusY = std::min(searchRadius_, wY_);

            double maxDistance = std::sqrt(std::pow(searchRadiusX, 2) + std::pow(searchRadiusY, 2));

            cout << "SEARCH CHECK " << searchRadiusX << " " << searchRadiusY << " " << maxDistance << endl;

            for(unsigned int i = 0; i < searchRadiusY; i++) {
                for(unsigned int j = 0; j < searchRadiusX; j++) {
                    double cost = std::sqrt(std::pow(j, 2) + std::pow(i, 2));
                    if(i == 48 && j == 48) {
                        cout << "48 hit " << cost << " " << field_[i][j] << endl;
                    }

                    if(cost > maxDistance) {
                        continue;
                    }

                    if(!field_[i][j]) {
                        found = true;
                        if(cost < bestCost) {
                            bestCost = cost;
                            best = Cursor(j, i);
                        }

                        cout << "Checking pos " << i << " " << j << " " << cost << endl;
                    }
                }
            }

            if(found) {
                setCursor(best);
                //cout << best.first << " " << best.second << endl;
                //cout << bestCost << endl;
                //exit(0);
                return;
            }

            searchRadius_++;
        }

        cout << "Couldn't find any place " << searchRadius_ << endl;
        setCursor(0, 0);

        exit(0);
    }

    void NearestPointCursorField::resetField(Field& target) {
        //dirty stuff
        NearestPointCursorField& castTarget = static_cast<NearestPointCursorField&>(target);

        //just copy everything for now
        for(int i = 0; i < wY_; i++) {
            field_[i] = castTarget.field_[i];
        }
    }
}