#include "bezier_surface.h"

using namespace std;
typedef amath::vec4  point4;

Bezier_Surface::Bezier_Surface(vector< vector<point4> > &pts, int u, int v){
	u_deg = u;
	v_deg = v;
	control_points = pts;
}

vector<point4> Bezier_Surface::get_control_col(int u){
	vector<point4> cc;
	for (int i=0; i<=v_deg; i++){
		cc.push_back(control_points[i][u]);
	}
	return cc;
}

vector<point4> Bezier_Surface::get_control_row(int v){
	return control_points[v];
}

void Bezier_Surface::eval_bez (const std::vector<point4> controlpoints, int degree, const double t, point4 &pnt, vec4 &tangent){
	point4 *temp = new point4[degree+1];
	for(int i=0; i<= degree; i++){
		temp[i] = controlpoints[i];
	}

	point4 curr, next;
	int i = degree;
	while (i != 0){
		for (int j=0; j<i; j++){
			curr = temp[j];
			next = temp[j+1];

			temp[j] = curr*(1-t) + next*(t);
		}
		i--;
	}
	pnt = temp[0];
	tangent = next - curr;
	tangent *= degree;
	delete [] temp;
}	

void Bezier_Surface::evaluate (double u, double v, point4 &pnt, vec4 &norm){
	vector<point4> u_curve;
	vec4 tmp_tan;
	for (int i=0; i<=v_deg; i++){
		point4 u_controlpt;
		eval_bez(get_control_row(i), u_deg, u, u_controlpt, tmp_tan);
		u_curve.push_back(u_controlpt);
	}

	vector<point4> v_curve;
	for (int i=0; i<=u_deg; i++){
		point4 v_controlpt;
		eval_bez(get_control_col(i), v_deg, 1-v, v_controlpt, tmp_tan);
		v_curve.push_back(v_controlpt);
	}

	point4 v_pt;
	vec4 v_tan;
	eval_bez(u_curve, v_deg, 1-v, v_pt, v_tan);

	point4 u_pt;
	vec4 u_tan;
	eval_bez(v_curve, u_deg, u, u_pt, u_tan);

	vec3 n = normalize(cross(u_tan, v_tan));
	norm = vec4(n.x, n.y, n.z, 0.0);
	pnt = v_pt;
}

void Bezier_Surface::sample (int samples, vector<vec4> &points, vector<vec4> &norms){
	double u_samples = get_u_samples(samples) * 1.0;
	double v_samples = get_v_samples(samples) * 1.0;

	vector<vec4> pts;
	vector<vec4> nrms;

	for(int v=0; v<v_samples; v++){
		for(int u=0; u<u_samples; u++){
			point4 point;
			vec4 norm;
			evaluate(u/(u_samples-1), v/(v_samples-1), point, norm);
			pts.push_back(point);
			nrms.push_back(norm);
		}
	}

	for (int v=0; v < v_samples-1; v++){
        for (int u=0; u < u_samples-1; u++){
            points.push_back( pts[(u) + (v)*u_samples]);
            norms.push_back(  nrms[(u) + (v)*u_samples]);
            points.push_back( pts[(u+1) + (v)*u_samples]);
            norms.push_back(  nrms[(u+1) + (v)*u_samples]);
            points.push_back( pts[(u) + (v+1)*u_samples]);
            norms.push_back(  nrms[(u) + (v+1)*u_samples]);

            points.push_back( pts[(u+1) + (v)*u_samples]);
            norms.push_back(  nrms[(u+1) + (v)*u_samples]);
            points.push_back( pts[(u) + (v+1)*u_samples]);
            norms.push_back(  nrms[(u) + (v+1)*u_samples]);
            points.push_back( pts[(u+1) + (v+1)*u_samples]);
            norms.push_back( nrms[(u+1) + (v+1)*u_samples]);
        }
    }

}




