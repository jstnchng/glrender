#include <cstring>
#include <cstdlib>
#include "parser.h"

void Parser::read_wavefront_file(const char *file, 
    std::vector < int > &tris,
    std::vector< float > &verts){

    tris.clear ();
    verts.clear ();

    ifstream in(file);
    char buffer[1025];
    string cmd;
    
    for (int line=1; in.good(); line++) {
        in.getline(buffer,1024);
        buffer[in.gcount()]=0;
                
        cmd="";
        
        istringstream iss (buffer);
        
        iss >> cmd;
        
        if (cmd[0]=='#' or cmd.empty()) {
            // ignore comments or blank lines
            continue;
        } 
        else if (cmd=="v") {
            // got a vertex:
            
            // read in the parameters:
            double pa, pb, pc;
            iss >> pa >> pb >> pc;
 
            verts.push_back (pa);
            verts.push_back (pb);
            verts.push_back (pc);
         } 
        else if (cmd=="f") {
            // got a face (triangle)
            
            // read in the parameters:
            int i, j, k;
            iss >> i >> j >> k;
            
            // vertex numbers in OBJ files start with 1, but in C++ array
            // indices start with 0, so we're shifting everything down by
            // 1
            tris.push_back (i-1);
            tris.push_back (j-1);
            tris.push_back (k-1);
        } 
        else {
            std::cerr << "Parser error: invalid command at line " << line << std::endl;
        }
        
     }
    
    in.close();
    
    std::cout << "found this many tris, verts: " << tris.size () / 3.0 << "  " 
              << verts.size () / 3.0 << std::endl;
}

bool Parser::is_obj_file(const char *file){
	int len = strlen(file);
	if (len <= 4){
		return false;
	} else {
		if ( file[len-4] == '.'
			&& file[len-3] == 'o'
			&& file[len-2] == 'b'  
			&& file[len-1] == 'j' ){
			return true;
		} else {
			return false;
		}
	}
}

void Parser::read_bezier_file (const char *file, vector <Bezier_Surface> &surfaces){
	ifstream in(file);
    char buffer[1025];
    string cmd;

    int num_surf;
	in.getline(buffer,1024);
	buffer[in.gcount()]=0;

	istringstream iss (buffer);
	iss >> num_surf;

	std::cout << "number of bezier surfaces read: " << num_surf << std::endl;

	vector < vector<vec4> > control_points;

	for (int i=0; i<num_surf; i++){
		control_points.clear();
		int u_deg, v_deg;

		in.getline(buffer,1024);
        buffer[in.gcount()]=0;
        istringstream iss (buffer);
        iss >> u_deg >> v_deg;

        for (int v=0; v<=v_deg; v++){
        	in.getline(buffer,1024);
    	    buffer[in.gcount()]=0;
	        istringstream iss (buffer);
            std::vector<vec4> row;

	        for (int u=0; u<=u_deg; u++){
	        	double x,y,z;
	        	iss >> x >> y >> z;
                vec4 v(x,y,z,1.0);
                row.push_back(v);
	        }
            control_points.push_back(row);
        }
        surfaces.push_back( Bezier_Surface(control_points, u_deg, v_deg) );

        std::cout << "for surface " << i << " {" << std::endl;
        std::cout << "   u_deg: " << u_deg << std::endl;
        std::cout << "   v_deg: " << v_deg << std::endl;
        std::cout << "   control_points: " << control_points.size() << " x " << control_points[0].size() << std::endl;
        std::cout << "}" << std::endl;
	}

	in.close();
}

