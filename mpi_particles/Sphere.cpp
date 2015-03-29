// draw a simple sphere object

#include "Sphere.hpp"
#include "Geometry.hpp"
#include "Textures.hpp"
#include "Vec.inl"
#include "Mat.inl"

#define F_PI 3.14159265f

// load the object data
Sphere::Sphere(
	Geometry &geom, Textures &tex,
	float radius, Vec<3> center, float scalar)
{
	// polygonal grid resolution
	unsigned int w = 16, h = 8;

	// build vertex, normal and texture coordinate arrays
	num_verts = (w + 1) * (h + 1);
	unsigned int numtri = 2*w*h;
	drawID = geom.addDraw(num_verts, 3*numtri);
	Geometry::Vertex *vert = geom.getVertices(drawID);
	Geometry::Index *tris = geom.getIndices(drawID);

	for(unsigned int j=0, idx=0; j <= h;  ++j) {
		for(unsigned int i=0;  i <= w; ++idx, ++i) {
			// 0-1 texture coordinates from grid location
			vert[idx].uv = Vec2(float(i)/float(w), float(j)/float(h));

			// 3D position and normal from spherical coordinates
			float theta = vert[idx].uv.x*(2.f*F_PI);
			float phi   = vert[idx].uv.y*F_PI;
			float cost = cosf(theta), sint = sinf(theta);
			float cosp = cosf(phi),   sinp = sinf(phi);
			vert[idx].norm = Vec3(cost*sinp, sint*sinp, cosp);
			vert[idx].pos = radius * vert[idx].norm;
		}
	}

	// build index array linking sets of three vertices into triangles
	// two triangles per square in the grid. Each vertex index is
	// essentially its unfolded grid array position. Be careful that
	// each triangle ends up in counter-clockwise order
	for(unsigned int y=0, idx=0; y<h; ++y) {
		for(unsigned int x=0; x<w; ++x) {
			tris[idx++] = (w+1)* y	  + x;
			tris[idx++] = (w+1)* y	  + x+1;
			tris[idx++] = (w+1)*(y+1) + x+1;

			tris[idx++] = (w+1)* y	  + x;
			tris[idx++] = (w+1)*(y+1) + x+1;
			tris[idx++] = (w+1)*(y+1) + x;
		}
	}

	// initialize per-model data
	Geometry::ModelUniforms &unif = geom.getModelUniforms(drawID);
	unif.modelMats = Xform::translate(center) * Xform::scale(Vec3(scalar,scalar,scalar));
	unif.KdMap = tex.findOrLoad("pebbles.pam");
	unif.KsMap = Vec2(-1,-1);
	unif.Kd = Vec4(1,1,1,1);
	unif.Ks = Vec3(0.04f, 0.04f, 0.04f);
	unif.Ns = 32.f;
}
