#ifndef FLOPOCO_LINECURSORFIELD_HPP
#define FLOPOCO_LINECURSORFIELD_HPP

#include "Field.hpp"

namespace flopoco {
    class LineCursorField : public Field {
    public:
        LineCursorField(unsigned int wX, unsigned int wY, bool signedIO);
        LineCursorField(const Field &copy);
    public:
        void updateCursor() override;
        void resetField(Field& target) override;
        void resetCursorBehaviour() override;
    };
}


#endif //FLOPOCO_LINECURSORFIELD_HPP
