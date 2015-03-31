// draw a simple sphere object

#include "MultiSphere.hpp"
#include "Geometry.hpp"
#include "Textures.hpp"
#include "Vec.inl"
#include "Mat.inl"

#define F_PI 3.14159265f

// load the object data
MultiSphere::MultiSphere(class Geometry &geom, class Textures &tex,
		   float radius, unsigned int number_of_spheres){
	// polygonal grid resolution
	unsigned int w = 16, h = 8;
	
	num_spheres = number_of_spheres;

	// build vertex, normal and texture coordinate arrays
	num_verts_per_sphere = (w + 1) * (h + 1);
	unsigned int num_verts = num_verts_per_sphere * num_spheres;
	unsigned int numtri = 2*w*h * num_spheres;
	drawID = geom.addDraw(num_verts, 3*numtri);
	Geometry::Vertex *vert = geom.getVertices(drawID);
	Geometry::Index *tris = geom.getIndices(drawID);
	
	unsigned int vert_idx = 0, tri_idx = 0, offset = 0;
	for(unsigned int k=0; k < num_spheres; k++){
		//fprintf(stderr,"Beginning draw for %u/%u\n",k, num_spheres);
		
		for(unsigned int j=0; j <= h;  ++j) {
			for(unsigned int i=0;  i <= w; ++vert_idx, ++i) {
				// 0-1 texture coordinates from grid location
				vert[vert_idx].uv = Vec2(float(i)/float(w), float(j)/float(h));

				// 3D position and normal from spherical coordinates
				float theta = vert[vert_idx].uv.x*(2.f*F_PI);
				float phi   = vert[vert_idx].uv.y*F_PI;
				float cost = cosf(theta), sint = sinf(theta);
				float cosp = cosf(phi),   sinp = sinf(phi);
				vert[vert_idx].norm = Vec3(cost*sinp, sint*sinp, cosp);
				vert[vert_idx].pos = radius * vert[vert_idx].norm;
			}
		}
		

		// build index array linking sets of three vertices into triangles
		// two triangles per square in the grid. Each vertex index is
		// essentially its unfolded grid array position. Be careful that
		// each triangle ends up in counter-clockwise order
		offset = num_verts_per_sphere * k;
		for(unsigned int y=0; y<h; ++y) {
			for(unsigned int x=0; x<w; ++x) {
				tris[tri_idx++] = offset + (w+1) * y + x;
				tris[tri_idx++] = offset + (w+1) * y + x+1;
				tris[tri_idx++] = offset + (w+1) * (y+1) + x+1;

				tris[tri_idx++] = offset + (w+1) * y + x;
				tris[tri_idx++] = offset + (w+1) * (y+1) + x+1;
				tris[tri_idx++] = offset + (w+1) * (y+1) + x;
			}
		}
		
	}

	Vec<3> center = Vec3(0,0,0);
	double scalar = 1;
	// initialize per-model data
	Geometry::ModelUniforms &unif = geom.getModelUniforms(drawID);
	unif.modelMats = Xform::translate(center) * Xform::scale(Vec3(scalar,scalar,scalar));
	unif.KdMap = tex.findOrLoad("pebbles.pam");
	unif.KsMap = Vec2(-1,-1);
	unif.Kd = Vec4(1,1,1,1);
	unif.Ks = Vec3(0.04f, 0.04f, 0.04f);
	unif.Ns = 32.f;
}
