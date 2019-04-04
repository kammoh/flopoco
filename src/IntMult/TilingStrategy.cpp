#include "IntMult/TilingStrategy.hpp"

namespace flopoco {


TilingStrategy::TilingStrategy(int wX, int wY, int wOut, bool signedIO, BaseMultiplierCollection *baseMultiplierCollection) :
	wX(wX), wY(wY), wOut(wOut), signedIO(signedIO), baseMultiplierCollection(baseMultiplierCollection)
{
}

void TilingStrategy::printSolution(ofstream &outstream)
{
    outstream << "\\documentclass{standalone}\n\\usepackage{tikz}\n\n\\begin{document}\n\\begin{tikzpicture}\n";
    outstream << "\\draw[thick] (0, 0) rectangle (" << wX << ", " << wY << ");\n";
    for (size_t i = 0 ; i < static_cast<size_t>(wY) ; ++i) {
        outstream << "\\draw[dotted, very thin, gray] (0, " << i <<") -- (" << wX << ", " << i << ");\n";
    }
    for (size_t i = 0 ; i < static_cast<size_t>(wX) ; ++i) {
        outstream << "\\draw[dotted, very thin, gray] ("<< i <<", 0) -- (" << i << ", " << wY << ");\n";
    }
    for (auto& tile : solution) {
        auto& parametrization = tile.first;
        auto& coordinates = tile.second;
        int xstart = coordinates.first;
        int ystart = coordinates.second;
        int xend = xstart + static_cast<int>(parametrization.getXWordSize());
        int yend = ystart + static_cast<int>(parametrization.getYWordSize());
        outstream << "\\draw (" << xstart << ", " << ystart << ") rectangle (" <<
                     xend << ", " << yend << ");\n";
    }
    outstream << "\\end{tikzpicture}\n\\end{document}\n";
}
}   //end namespace flopoco
