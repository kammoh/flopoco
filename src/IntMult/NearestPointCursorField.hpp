#ifndef FLOPOCO_NEARESTPOINTCURSORFIELD_HPP
#define FLOPOCO_NEARESTPOINTCURSORFIELD_HPP


#include "Field.hpp"

namespace flopoco {
    class NearestPointCursorField : public BaseFieldState {
    public:
        NearestPointCursorField();

        void updateCursor() override;
        void setField(Field* field) override;
        void reset(BaseFieldState& baseState) override;
        void reset(Field* field, ID id, unsigned int missing) override;
    private:
        unsigned int searchRadius_;
        unsigned int segmentPos_;

        void checkCircleSegment(unsigned int radius);
        bool checkAction(Cursor& coord, int deltaX, int deltaY, unsigned int diameter);

        struct NextCoord{
            Cursor coord;
            float distance;
        };

        vector<vector<NextCoord>> coordsLUT_;
        list<NextCoord> nextCoords_;
    };
}


#endif //FLOPOCO_NEARESTPOINTCURSORFIELD_HPP
