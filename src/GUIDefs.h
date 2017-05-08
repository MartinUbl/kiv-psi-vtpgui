/********************************
 * KIV/PSI: VTP node            *
 * Semestral work               *
 *                              *
 * Author: Martin UBL           *
 *         A16N0026P            *
 *         ublm@students.zcu.cz *
 ********************************/

#ifndef GUIDEFS_H
#define GUIDEFS_H

#include <vector>

/*
 * Container structure for frame dump contents
 */
struct FrameDumpContents
{
    // hex string of contents
    std::string contents;

    // parsed rows
    std::vector<std::pair<std::string, std::string> > tableRows;
    // parsed byte positions of row contents
    std::vector<std::pair<size_t, size_t> > dumpStartEnds;
};

#endif
