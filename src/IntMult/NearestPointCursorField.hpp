#ifndef FLOPOCO_NEARESTPOINTCURSORFIELD_HPP
#define FLOPOCO_NEARESTPOINTCURSORFIELD_HPP


#include "Field.hpp"

namespace flopoco {
    class NearestPointCursorField : public Field {
    public:
        NearestPointCursorField(unsigned int wX, unsigned int wY, bool signedIO);
        NearestPointCursorField(const Field &copy);
    public:
        void updateCursor() override;
        void resetField(Field& target) override;
        void resetCursorBehaviour() override;

    private:
        unsigned int searchRadius_;
        void checkCircleSegment(unsigned int radius);
        bool checkAction(Cursor& coord, int deltaX, int deltaY, unsigned int diameter);

        struct NextCoord{
            Cursor coord;
            float distance;
        };

        list<NextCoord> nextCoords_;
    };
}


#endif //FLOPOCO_NEARESTPOINTCURSORFIELD_HPP
