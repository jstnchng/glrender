#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>

#include "bezier_surface.h"

class Parser {
public:
    void read_wavefront_file (const char *file,
    							std::vector< int > &tris,
    							std::vector< float > &verts);

    bool is_obj_file (const char *file);

    void read_bezier_file (const char *file, vector <Bezier_Surface> &surfaces);
};

#endif

