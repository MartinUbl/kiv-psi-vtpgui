#ifndef GUIDEFS_H
#define GUIDEFS_H

#include <vector>

struct FrameDumpContents
{
    std::string contents;

    std::vector<std::pair<std::string, std::string> > tableRows;
    std::vector<std::pair<size_t, size_t> > dumpStartEnds;
};

#endif
