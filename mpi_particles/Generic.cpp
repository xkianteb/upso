// draw a simple cylinder object

#include "Generic.hpp"
#include "Geometry.hpp"
#include "Textures.hpp"
#include "Vec.inl"
#include "Mat.inl"
#include <stdlib.h>
#include <map>
#include <vector>
 
#define EPSILON 0.01

double dot(Vec<3> a, Vec<3> b){
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double length(Vec<3> v){
    return sqrt(dot(v, v));
}

Vec<3> normalize(Vec<3> v){
    double recip = 1 / length(v);
    Vec<3> n;
    n.x = v.x * recip;
    n.y = v.y * recip;
    n.z = v.z * recip;
    return n;
}

Vec<3> cross(Vec<3> U, Vec<3> V){
    Vec<3> result;
    result.x = U.y * V.z - U.z * V.y;
    result.y = U.z * V.x - U.x * V.z;
    result.z = U.x * V.y - U.y * V.x;
    return result;
}

Vec<3> face_normal(Vec<3> a,Vec<3> b,Vec<3> c){
    
    Vec<3> U = b - a;
    Vec<3> V = c - a;
    
    Vec<3> normal = cross(U, V);
    return normalize(normal);
}

double area(Vec<3> a, Vec<3> b, Vec<3> c){
    return 0.5 * length(cross(b - a, c - a));
}


Vec<3> compute_norm(unsigned int vertex_number, Vec<3> *vertices, unsigned int num_vertices, Vec<9> *faces, unsigned int num_faces, Vec<3> orig_face_normal){

    Vec<3> norm;
    norm.x = 0;
    norm.y = 0;
    norm.z = 0;
    
    Vec<3> this_face_normal;
    double face_area;
    //printf("Looking for vert number %u\n", vertex_number);
    int vert_number_0, vert_number_1, vert_number_2;
    for(unsigned int i = 0; i < num_faces; i++){
        vert_number_0 = (int) faces[i][0];
        vert_number_1 = (int) faces[i][3];
        vert_number_2 = (int) faces[i][6];
        if(vert_number_0 == vertex_number || vert_number_1 == vertex_number || vert_number_2 == vertex_number){
            // found a face that has this vertex
            //printf("Found vert %u at face %u, %i, %i, %i\n", vertex_number, i, vert_number_0, vert_number_1, vert_number_2);
            
            this_face_normal = face_normal(vertices[vert_number_0-1],vertices[vert_number_1-1],vertices[vert_number_2-1]);
            //printf("face normal: %f %f %f\n", this_face_normal.x, this_face_normal.y, this_face_normal.z);
            face_area = area(vertices[vert_number_0 - 1], vertices[vert_number_1 - 1], vertices[vert_number_2 - 1]);
            
            //printf("face area: %lf\n",face_area);
            if(dot(this_face_normal, orig_face_normal) > EPSILON){
                norm = norm + face_area * this_face_normal;
            }
            
        }
        
    }
    
    return normalize(norm);
}

// load the object data
Generic::Generic(
	Geometry &geom, Textures &tex, struct ObjInput input, Vec<3> *vertices, unsigned int num_vertices, Vec<2> *texture_coords, unsigned int num_texture_coords, Vec<3> *normal_lines, unsigned int num_normal_lines, Vec<3> base, float scalar, bool verbose, std::map<char*, struct Materials> material_lookup, char * base_path)
	//Vec<3> *vertices, unsigned int num_vertices, Vec<2> *texture_coords, unsigned int num_texture_coords, Vec<3> *normal_lines, unsigned int num_normal_lines, Vec<9> *faces, unsigned int num_faces, bool normals_present, Vec<3> base, float scalar)
{
    
    Vec<9> face;
	struct Materials *this_material = NULL;
	
	for(std::map<char*, struct Materials>::iterator it = material_lookup.begin(); it != material_lookup.end(); ++it) {
		//printf("did they make it names: %s %s\n", it->first, it->second.file_path);
		if(strcmp(input.mat_name, it->second.name) == 0){
			this_material = &it->second;
		}
	}
	
	//Vec<3> *vertices = input.vertices;
	//unsigned int num_vertices = input.num_vertices;
	//Vec<2> *texture_coords = input.texture_coords;
	//unsigned int num_texture_coords = input.num_texture_coords;
	//Vec<3> *normal_lines = input.normal_lines;
	//unsigned int num_normal_lines = input.num_normal_lines;
	Vec<9> *faces = input.faces;
	unsigned int num_faces = input.num_faces;
	bool normals_present = input.normals_present;
    
    
    // Do an initial scan to put verts into lookup table, then
	std::map<std::vector<int>, unsigned short> lookup;
	std::map<std::vector<int>, unsigned short>::iterator found;
	unsigned short verts_stored = 0;
    std::vector<int> triplet (3);
	
    for(int i = 0; i < num_faces; i++){
        for(int j = 0; j < 9; j+=3){
            face = faces[i];
            
            triplet[0] = (int) face[j+0];
            triplet[1] = (int) face[j+1];
            triplet[2] = (int) face[j+2];
			
            found = lookup.find(triplet);
            if(found == lookup.end()){
				lookup[triplet] = verts_stored;
				if (verbose) {
					printf("Adding triplet %u, %u, %u to %u\n", triplet[0], triplet[1], triplet[2], lookup[triplet]);
				}
                verts_stored++;
            }
        }
    }
	

	
    drawID = geom.addDraw(verts_stored, 3 * num_faces);
    Geometry::Vertex *vert = geom.getVertices(drawID);
    Geometry::Index *tris = geom.getIndices(drawID);

	printf("Size of std::map() lookup table for drawId %i: %lu\n", drawID, lookup.size());
	
    Vec<3> face_norm;
    Vec<3> temp_vec3;
    
    int verts_to_tris = 0;
	verts_stored = 1;
	if (verbose){
		printf("normals present: %i\n", (normals_present ? 1 : 0));
		fflush(stdout);
		printf("Num faces: %u, num_tris: %u\n",num_faces, 3 * num_faces);
	}
	
    unsigned int this_index;
	for(int i = 0; i < num_faces; i++){
		face = faces[i];
		
		if (verbose){
			printf("** got face\n");
			fflush(stdout);
		}
		for(int j = 0; j < 9; j+=3){
			if (verbose){
				printf("i: %i (%i), j: %i\n",i,num_faces,j);
				fflush(stdout);
			}
			
            if(!normals_present){
                face_norm = face_normal(vertices[((int) faces[i][0]) -1],
                                        vertices[((int) faces[i][3]) -1],
                                        vertices[((int) faces[i][6]) -1]);
			}
			if (verbose){
				printf("\t got norm\n");
				fflush(stdout);
			}
			
            triplet[0] = (int) face[j+0];
            triplet[1] = (int) face[j+1];
            triplet[2] = (int) face[j+2];
			
			if (verbose)
				printf("\t got triplet %i, %i, %i\n", triplet[0], triplet[1], triplet[2]);
			fflush(stdout);
            this_index = lookup[triplet];
			
			if (verbose){
				printf("\t got index: %i\n",this_index);
				fflush(stdout);
			}
            vert[this_index].pos = Vec3(vertices[((int) face[j+0]) - 1].x,vertices[((int) face[j+0]) - 1].y,vertices[((int) face[j+0]) - 1].z);
			
			if (verbose){
				printf("\t set pos: %f,%f,%f\n",vert[this_index].pos.x,vert[this_index].pos.y,vert[this_index].pos.z);
				fflush(stdout);
			}
            vert[this_index].uv = Vec2(texture_coords[((int) face[j+1]) - 1].x, texture_coords[((int) face[j+1]) - 1].y);
            
            temp_vec3 =  normals_present ? normal_lines[((int) face[j+2]) - 1] : compute_norm((int) face[j+0], vertices, num_vertices, faces, num_faces, face_norm);
            vert[this_index].norm = Vec3(temp_vec3.x, temp_vec3.y, temp_vec3.z);
            
			
            tris[verts_to_tris] = lookup[triplet];
			if(verbose){
				printf("\tVert %u, norm: %f %f %f\n", this_index, vert[this_index].norm.x,vert[this_index].norm.y,vert[this_index].norm.z);
				printf("\tstoring %u in tris[ %u ]\n",lookup[triplet], verts_to_tris);
			}
            verts_to_tris++;
        }
	}
    
	// initialize per-model data
	Geometry::ModelUniforms &unif = geom.getModelUniforms(drawID);
    unif.modelMats = Xform::translate(base) * Xform::scale(Vec3(scalar,scalar,scalar));
	if(this_material){
		unsigned int new_length = strlen(this_material->file_path) + strlen(base_path) + 5;
		char *new_path = (char *) malloc(new_length);
		memset(new_path, 0, new_length * sizeof(char));
		memcpy(new_path, base_path, strlen(base_path) * sizeof(char));
		memcpy(&new_path[strlen(base_path)], this_material->file_path, strlen(this_material->file_path) * sizeof(char));
		
		unif.KdMap = tex.findOrLoad(new_path);
		unif.KsMap = Vec2(-1,-1);
		unif.Kd = Vec4(this_material->kd.x,this_material->kd.y,this_material->kd.z,1);
		unif.Ks = this_material->ks; // Vec3(0.04f, 0.04f, 0.04f);
		unif.Ns = this_material->ns; // 32.f;
	}else{
		unif.KdMap = tex.findOrLoad("pebbles.pam");
		unif.KsMap = Vec2(-1,-1);
		unif.Kd = Vec4(1,1,1,1);
		unif.Ks = Vec3(0.04f, 0.04f, 0.04f);
		unif.Ns = 32.f;
	}
}
