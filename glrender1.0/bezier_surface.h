#ifndef __BEZIER_SURFACE_H__
#define __BEZIER_SURFACE_H__

#include <vector>
#include "amath.h"
using namespace std;

typedef amath::vec4 point4;

class Bezier_Surface{
public:
	vector < vector<point4> > control_points;
	int u_deg;
	int v_deg;

public:
	vector<point4> get_control_col(int u);
	vector<point4> get_control_row(int v);

	Bezier_Surface(vector <vector<point4> > &pts, int u, int v);

	int get_u_samples (int samples){
		return (u_deg+1) * samples;
	};
	int get_v_samples (int samples){
		return (v_deg+1) * samples;
	};

	void eval_bez (const std::vector<point4> controlpoints, int degree, const double t, point4 &pnt, vec4 &tangent);
	void evaluate (double u, double v, point4 &pnt, vec4 &norm);
	void sample (int samples, vector<vec4> &points, vector<vec4> &norms);
};

#endif