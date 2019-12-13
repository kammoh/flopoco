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
    };
}


#endif //FLOPOCO_NEARESTPOINTCURSORFIELD_HPP
